/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef _GENERATIONMANAGER_H
#define _GENERATIONMANAGER_H

#include <assert.h>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <sstream>
#include <fstream>

#include "Util.h"

#define DEFINE_COMPONENT(comp, comp_name)                                                                                                \
	namespace                                                                                                                        \
	{                                                                                                                                \
	class GenerationComponentInitializer_##comp_name : gensim::generator::GenerationComponentInitializer                             \
	{                                                                                                                                \
	       public:                                                                                                                   \
		GenerationComponentInitializer_##comp_name() : gensim::generator::GenerationComponentInitializer(false) {}               \
		virtual ~GenerationComponentInitializer_##comp_name() {}                                                                 \
		virtual std::string GetName() { return #comp_name; }                                                                     \
		virtual gensim::generator::GenerationComponent *Get(gensim::generator::GenerationManager &man) { return new comp(man); } \
	};                                                                                                                               \
	static GenerationComponentInitializer_##comp_name init_##comp_name __attribute__((init_priority(900)));                          \
	}
#define DEFINE_DUMMY_COMPONENT(comp, comp_name)                                                                           \
	namespace                                                                                                         \
	{                                                                                                                 \
	class GenerationComponentInitializer_##comp_name : gensim::generator::GenerationComponentInitializer              \
	{                                                                                                                 \
	       public:                                                                                                    \
		GenerationComponentInitializer_##comp_name() : gensim::generator::GenerationComponentInitializer(true) {} \
		virtual ~GenerationComponentInitializer_##comp_name() {}                                                  \
		virtual std::string GetName() { return #comp_name; }                                                      \
		virtual gensim::generator::GenerationComponent *Get(gensim::generator::GenerationManager &man)            \
		{                                                                                                         \
			assert(false && "Cannot initialize component of type " #comp_name);                               \
			return NULL;                                                                                      \
		}                                                                                                         \
	};                                                                                                                \
	static GenerationComponentInitializer_##comp_name init_##comp_name __attribute__((init_priority(900)));           \
	}

#define COMPONENT_INHERITS(sub, super)                                                                                                            \
	namespace                                                                                                                                 \
	{                                                                                                                                         \
	gensim::generator::GenerationInheritanceInitializer init_inheritance_##sub##_##super __attribute__((init_priority(1000))) (#sub, #super); \
	}
#define COMPONENT_OPTION(comp, name, def, help)                                                                                          \
	namespace                                                                                                                        \
	{                                                                                                                                \
	static gensim::generator::GenerationOptionInitializer comp##name __attribute__((init_priority(1000))) (#comp, #name, def, help); \
	};

namespace gensim
{

	namespace arch
	{
		class ArchDescription;
	}
	namespace generator
	{
		class GenerationComponent;
		class GenerationManager;

		class GenerationComponentInitializer
		{
		public:
			static std::list<GenerationComponentInitializer *> Initializers;

			virtual std::string GetName() = 0;
			virtual GenerationComponent *Get(GenerationManager &man) = 0;

			bool dummy;

			GenerationComponentInitializer(bool _dummy) : dummy(_dummy)
			{
				Initializers.push_back(this);
			}
		};

		class GenerationInheritanceInitializer
		{
		public:
			const char *const Subcomponent, *const Supercomponent;
			GenerationInheritanceInitializer(const char *const _sub, const char *const _super);
		};

		enum class ModuleEntryType {
			UNKNOWN,
			ExecutionEngine,
			BlockJITTranslator,
			Interpreter,
			ArchDescriptor
		};

		class ModuleEntry
		{
		public:
			ModuleEntry(const std::string &entry_name, const std::string &class_name, const std::string &class_header, ModuleEntryType entry_type) : entry_name_(entry_name), class_name_(class_name), class_header_(class_header), entry_type_(entry_type) {}

			const std::string &GetClassHeader() const
			{
				return class_header_;
			}
			const std::string &GetClassName() const
			{
				return class_name_;
			}
			const std::string &GetEntryName() const
			{
				return entry_name_;
			}

			ModuleEntryType GetEntryType() const
			{
				return entry_type_;
			}

		private:
			std::string class_header_;
			std::string class_name_;
			std::string entry_name_;
			ModuleEntryType entry_type_;
		};

		class FunctionEntry
		{
		public:
			FunctionEntry(const std::string prototype, const std::string &body, const std::vector<std::string> &local_headers= {}, const std::vector<std::string> &sys_headers= {}, const std::vector<std::string> &specialisations= {}, bool global=false);

			std::string Format() const;
			std::string FormatPrototype() const;
			std::string FormatIncludes() const;
			size_t GetBodySize() const;
			bool IsGlobal() const;

			const std::vector<std::string> &GetLocalHeaders() const;
			const std::vector<std::string> &GetSystemHeaders() const;

		private:
			std::string prototype_;
			std::string body_;
			std::vector<std::string> local_headers_;
			std::vector<std::string> system_headers_;
			std::vector<std::string> specialisations_;

			bool is_global_;
		};

		class GenerationManager
		{
		public:
			static std::string FnDisasm;
			static std::string FnDecode;
			static std::string FnInterpret;
			static std::string FnTranslate;
			static std::string FnPipeline;
			static std::string FnJumpInfo;

			GenerationManager(arch::ArchDescription &arch, std::string targetDir) : arch(arch), target(targetDir), _components_up_to_date(false) {};

			GenerationComponent *GetComponent(const std::string);
			const GenerationComponent *GetComponentC(const std::string) const;
			const std::vector<GenerationComponent *> &GetComponents() const;
			void AddComponent(GenerationComponent &component);

			void AddModuleEntry(const ModuleEntry &entry)
			{
				module_entries_.push_back(entry);
			}
			void AddFunctionEntry(const FunctionEntry &entry)
			{
				function_entries_.push_back(entry);
			}

			bool Generate();

			inline const std::string GetTarget() const
			{
				return target;
			}

			inline const arch::ArchDescription &GetArch() const
			{
				return arch;
			}

			inline void SetTarget(std::string s)
			{
				target = s;
			}

			virtual ~GenerationManager() {};

			const std::vector<ModuleEntry> &GetModuleEntries() const
			{
				return module_entries_;
			}
			const std::vector<FunctionEntry> &GetFunctionEntries() const
			{
				return function_entries_;
			}

		private:
			arch::ArchDescription &arch;
			std::string target;

			std::vector<ModuleEntry> module_entries_;
			std::vector<FunctionEntry> function_entries_;

			std::multimap<std::string, GenerationComponent *> Components;
			mutable bool _components_up_to_date;
			mutable std::vector<GenerationComponent *> _components;

			GenerationManager(const GenerationManager &orig);
		};

		class GenerationSetupManager
		{
			friend class GenerationManager;

		private:
			GenerationManager &Manager;

		public:
			inline GenerationComponent *GetComponent(std::string str)
			{
				return Manager.GetComponent(str);
			};

		private:
			GenerationSetupManager(GenerationManager &M) : Manager(M) {};
			GenerationSetupManager(const GenerationSetupManager &);
		};

		struct GenerationOption {
			const char *const Name;
			const char *const DefaultValue;
			const char *const HelpString;

			GenerationOption(const char *const _name, const char *const _default, const char *const _helpstring) : Name(_name), DefaultValue(_default), HelpString(_helpstring) {}
		};

		class GenerationOptionInitializer
		{
		public:
			GenerationOptionInitializer(const char *const component, const char *const name, const char *const default_value, const char *const help_string);
		};

		class GenerationComponent
		{
		public:
			static std::map<std::string, std::map<std::string, GenerationOption *> > Options;
			static std::map<std::string, std::string> Inheritance;

			friend class GenerationManager;
			virtual bool Generate() const = 0;

			virtual void Reset() {}

			virtual void Setup(GenerationSetupManager &Setup) {}

			virtual std::string GetFunction() const = 0;

			std::list<std::string> GetPropertyList() const;

			std::string GetProperty(const std::string key) const;

			bool HasProperty(const std::string key) const;

			inline void SetProperty(std::string key, std::string val)
			{
				assert(HasProperty(key));
				Properties[key] = val;
			}

			const virtual std::vector<std::string> GetSources() const = 0;

		protected:
			GenerationManager &Manager;

			void WriteOutputFile(const std::string filename, const util::cppformatstream &contents) const;
			void WriteOutputFile(const std::string filename, const std::stringstream &contents) const;

			inline const std::map<std::string, std::string> &GetProperties()
			{
				return Properties;
			}

			std::string name;

			GenerationComponent(GenerationManager &man, std::string _name) : Manager(man), name(_name) {}

		private:
			std::map<std::string, std::string> Properties;

			// disallow copy and assigment
			GenerationComponent();
			GenerationComponent(const GenerationComponent &orig);
			GenerationComponent &operator=(const GenerationComponent &orig);
		};

	}  // namespace generator
}  // namespace gensim

#endif /* _GENERATIONMANAGER_H */
