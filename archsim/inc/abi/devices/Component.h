/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   Component.h
 * Author: s0457958
 *
 * Created on 13 December 2013, 16:02
 */

#ifndef COMPONENT_H
#define	COMPONENT_H

#include "abi/Address.h"
#include "define.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace archsim
{
	namespace core
	{
		namespace thread
		{
			class ThreadInstance;
		}
	}

	namespace abi
	{

		class EmulationModel;

		namespace devices
		{
			class Component;
			class MemoryComponent;

			class IRQLine;
			class ConsoleSerialPort;
			class ConsoleOutputSerialPort;
			namespace generic
			{
				namespace ps2
				{
					class PS2KeyboardDevice;
					class PS2MouseDevice;
				}
			}
			namespace gfx
			{
				class VirtualScreen;
			}

			enum ComponentParameterType {
				ComponentParameter_Unknown,
				ComponentParameter_String,
				ComponentParameter_U64,
				ComponentParameter_Component,
				ComponentParameter_Thread
			};

			class ComponentParameterDescriptor
			{
			public:
				ComponentParameterDescriptor(ComponentParameterType type);

				ComponentParameterType GetType() const
				{
					return type_;
				}
				uint32_t GetIndex() const
				{
					return index_;
				}
				void SetIndex(uint32_t index)
				{
					index_ = index;
				}
			private:
				ComponentParameterType type_;
				uint32_t index_;
			};

			class ComponentDescriptor
			{
			public:
				typedef std::unordered_map<std::string,	ComponentParameterDescriptor> parameter_descriptor_container_t;
				explicit ComponentDescriptor(const std::string &name, const parameter_descriptor_container_t &parameters = {}) : name_(name), parameter_descriptors_(parameters)
				{
					CheckParameterIndices();
				}

				const std::string &GetName() const
				{
					return name_;
				}
				const ComponentParameterDescriptor &GetParameterDescriptor(const std::string &name) const
				{
					return parameter_descriptors_.at(name);
				}
				bool HasParameter(const std::string &name) const
				{
					return parameter_descriptors_.count(name);
				}

				void CheckParameterIndices();
				const parameter_descriptor_container_t &GetParameterDescriptors() const
				{
					return parameter_descriptors_;
				}
			private:
				const std::string name_;
				parameter_descriptor_container_t parameter_descriptors_;
			};

			class ComponentDescriptorInstance
			{
			public:
				ComponentDescriptorInstance(const ComponentDescriptor &descriptor);
				ComponentDescriptorInstance(const ComponentDescriptorInstance &other) = delete;
				~ComponentDescriptorInstance();

				const ComponentDescriptor &GetDescriptor() const
				{
					return descriptor_;
				}

				const std::string &GetName() const
				{
					return GetDescriptor().GetName();
				}
				template<typename T> T GetParameter(const std::string &parameter) const;
				template<typename T> void SetParameter(const std::string &parameter, T value);
			private:
				const ComponentDescriptor &descriptor_;
				std::vector<void*> parameter_value_ptrs_;

				void *GetParameterPointer(const std::string &parameter) const;
			};

			template<> void ComponentDescriptorInstance::SetParameter(const std::string& parameter, archsim::core::thread::ThreadInstance* value);
			template<> void ComponentDescriptorInstance::SetParameter(const std::string& parameter, Component* value);
			template<> void ComponentDescriptorInstance::SetParameter(const std::string& parameter, uint64_t value);

			class Component
			{
			public:

				static const ComponentDescriptor dummy_descriptor;
				Component() : descriptor_(dummy_descriptor)
				{
					throw std::logic_error("Attempted to construct an empty component while using a buggy G++");
				}

				Component(const ComponentDescriptor &descriptor) : descriptor_(descriptor) {}
				virtual ~Component();
				const ComponentDescriptorInstance &GetDescriptor() const
				{
					return descriptor_;
				}
				ComponentDescriptorInstance &GetDescriptor()
				{
					return descriptor_;
				}

				template<typename T> T GetParameter(const std::string &parameter) const
				{
					return GetDescriptor().GetParameter<T>(parameter);
				}
				template<typename T> void SetParameter(const std::string &parameter, T value)
				{
					GetDescriptor().SetParameter(parameter, value);
				}

				virtual bool Initialise() = 0;

			private:
				ComponentDescriptorInstance descriptor_;
			};

			template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::IRQLine *value);
			template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::MemoryComponent *value);
			template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::ConsoleSerialPort *value);
			template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::ConsoleOutputSerialPort *value);
			template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::generic::ps2::PS2KeyboardDevice *value);
			template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::generic::ps2::PS2MouseDevice *value);
			template<> void Component::SetParameter(const std::string &parameter, archsim::abi::devices::gfx::VirtualScreen *value);

			template<typename realtype> class ComponentPointer
			{
			public:
				ComponentPointer() : data_(nullptr) {}

				void operator=(realtype *t)
				{
					data_ = t;
				}
				realtype &operator*()
				{
					return *data_;
				}
				realtype *operator->()
				{
					return data_;
				}

				realtype *get()
				{
					return data_;
				}

				operator realtype*()
				{
					return data_;
				}

			private:
				realtype *data_;
			};

#define COMPONENT_PARAMETER_ENTRY(name, component_type, real_type) real_type *Get##name() { static_assert(std::is_base_of<Component, real_type>::value, "Component parameters must be derived from Component"); return dynamic_cast<real_type*>(GetParameter<Component*>(#name)); }
#define COMPONENT_PARAMETER_ENTRY_HDR(name, component_type, real_type) real_type *Get##name();
#define COMPONENT_PARAMETER_ENTRY_SRC(clazz, name, component_type, real_type) real_type *clazz::Get##name() { static_assert(std::is_base_of<Component, real_type>::value, "Component parameters must be derived from Component"); return dynamic_cast<real_type*>(GetParameter<Component*>(#name)); }

#define COMPONENT_PARAMETER_THREAD(name) archsim::core::thread::ThreadInstance *Get##name() { return (GetParameter<archsim::core::thread::ThreadInstance*>(#name)); }
#define COMPONENT_PARAMETER_U64(name) uint64_t Get##name() { return (uint64_t)GetParameter<uint64_t>(#name); }

			class CoreComponent : public Component
			{
			public:
				virtual bool ReadRegister(uint32_t reg, uint32_t& data) = 0;
				virtual bool WriteRegister(uint32_t reg, uint32_t data) = 0;
			};

			class MemoryComponent : public virtual Component
			{
			public:
				MemoryComponent(EmulationModel &parent_model, Address base_address, uint32_t size);
				virtual ~MemoryComponent();

				inline Address GetBaseAddress() const
				{
					return base_address;
				}
				inline uint32_t GetSize() const
				{
					return size;
				}

				virtual bool Read(uint32_t offset, uint8_t size, uint64_t& data) = 0;
				virtual bool Write(uint32_t offset, uint8_t size, uint64_t data) = 0;

			protected:
				Address base_address;
				uint32_t size;

				EmulationModel &parent_model;
			};

			class MemoryRegister
			{
			public:
				MemoryRegister(std::string name, uint32_t offset, uint8_t width, uint32_t default_value, bool can_read = true, bool can_write = true);

				inline uint32_t GetOffset() const
				{
					return offset;
				}

				inline uint32_t Read() const
				{
					if (can_read) {
						return value;
					} else {
						return 0;
					}
				}

				inline uint32_t Write(uint32_t new_value)
				{
					uint32_t old_value = value;

					if (can_write) {
						value = (new_value & (((uint64_t)1 << width) - 1));
					}

					return old_value;
				}

				inline uint32_t Get() const
				{
					return value;
				}

				inline void Set(uint32_t new_value)
				{
					value = (new_value & (((uint64_t)1 << width) - 1));
				}

				inline bool operator==(MemoryRegister& r)
				{
					return r.offset == this->offset;
				}

				inline bool operator==(uint32_t& v)
				{
					return this->value == v;
				}

				inline std::string GetName() const
				{
					return name;
				}

			private:
				bool can_write;
				bool can_read;

				uint32_t offset;
				uint8_t width;
				uint32_t value;

				std::string name;
			};

			class RegisterBackedMemoryComponent : public MemoryComponent
			{
			public:
				RegisterBackedMemoryComponent(EmulationModel& parent_model, Address base_address, uint32_t size, std::string name);
				virtual ~RegisterBackedMemoryComponent();

				bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;
				bool Write(uint32_t offset, uint8_t size, uint64_t data) override;

				inline std::string GetName() const
				{
					return name;
				}

			protected:
				MemoryRegister *GetRegister(uint32_t offset);
				void AddRegister(MemoryRegister& rg);

				virtual uint32_t ReadRegister(MemoryRegister& reg);
				virtual void WriteRegister(MemoryRegister& reg, uint32_t value);

			private:
				std::string name;
				typedef std::unordered_map<uint32_t, MemoryRegister *> register_map_t;
				register_map_t registers;
			};

		}
	}
}

#endif	/* COMPONENT_H */
