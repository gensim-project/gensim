/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   ArchDescriptor.h
 * Author: harry
 *
 * Created on 10 April 2018, 15:23
 */

#ifndef ARCHDESCRIPTOR_H
#define ARCHDESCRIPTOR_H

#include "abi/Address.h"

#include "core/arch/RegisterFileDescriptor.h"
#include "core/arch/MemoryInterfaceDescriptor.h"

#include "gensim/gensim_decode_context.h"

#include <functional>
#include <map>
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace gensim
{
	class DecodeContext;
	class BaseDecode;
	class BaseDisasm;
	class BaseJumpInfo;
}

namespace archsim
{

	class MemoryInterface;

	namespace core
	{
		namespace thread
		{
			class ThreadInstance;
		}
	}


	class FeatureDescriptor
	{
	public:
		FeatureDescriptor(const std::string &name, uint32_t id, uint32_t default_value = 0) : name_(name), id_(id) {}
		const std::string &GetName() const
		{
			return name_;
		}
		uint32_t GetID() const
		{
			return id_;
		}
	private:
		std::string name_;
		uint32_t id_;
	};
	class FeaturesDescriptor
	{
	public:
		using FeatureContainer = std::vector<FeatureDescriptor>;

		FeaturesDescriptor(const std::initializer_list<FeatureDescriptor> &features) : features_(features) {}

		const FeatureContainer &GetFeatures() const
		{
			return features_;
		}

	private:
		FeatureContainer features_;
	};

	class InvocationContext
	{
	public:
		InvocationContext(archsim::core::thread::ThreadInstance *thread, const std::vector<uint64_t> &arguments) : thread_(thread), args_(arguments) {}

		archsim::core::thread::ThreadInstance *GetThread() const
		{
			return thread_;
		}
		const std::vector<uint64_t> &GetArgs() const
		{
			return args_;
		}
		uint64_t GetArg(int idx) const
		{
			return GetArgs().at(idx);
		}

	private:
		archsim::core::thread::ThreadInstance *thread_;
		std::vector<uint64_t> args_;
	};

	class BehaviourDescriptor
	{
	public:
		using InvocationResult = uint64_t;
		using FunctionType = std::function<InvocationResult (const InvocationContext&)>;

		BehaviourDescriptor(const std::string &name, const FunctionType& fn) : name_(name), function_(fn) {}
		const std::string &GetName() const
		{
			return name_;
		}

		InvocationResult Invoke(archsim::core::thread::ThreadInstance *thread, const std::vector<uint64_t> args) const
		{
			return function_(InvocationContext(thread, args));
		}
	private:
		const std::string name_;
		FunctionType function_;
	};

	class ISABehavioursDescriptor
	{
	public:
		ISABehavioursDescriptor(const std::initializer_list<BehaviourDescriptor> &behaviours);

		const BehaviourDescriptor &GetBehaviour(const std::string &name) const
		{
			return behaviours_.at(name);
		}
	private:
		std::unordered_map<std::string, BehaviourDescriptor> behaviours_;
	};

	using DecodeFunction = std::function<uint32_t(archsim::Address addr, archsim::MemoryInterface *, gensim::BaseDecode&)>;
	using NewDecoderFunction = std::function<gensim::BaseDecode*()>;
	using NewJumpInfoFunction = std::function<gensim::BaseJumpInfo*()>;
	using NewDTCFunction = std::function<gensim::DecodeTranslateContext*()>;

	/**
	 * Class which encapsulates ISA specific portions of the architecture,
	 * including instruction decoding and behaviour invocation.
	 */
	class ISADescriptor
	{
	public:
		ISADescriptor(const std::string &name, uint32_t id, const DecodeFunction &decoder, gensim::BaseDisasm *disasm, const NewDecoderFunction &newdecoder, const NewJumpInfoFunction &newjumpinfo, const NewDTCFunction &dtc, const ISABehavioursDescriptor &behaviours);

		// This is a bit of a hack right now. In the future there needs to be a clearer interaction between the thread, the 'decode context', and the decoded instruction
		uint32_t DecodeInstr(archsim::Address addr, archsim::MemoryInterface *interface, gensim::BaseDecode &target) const
		{
			return decoder_(addr, interface, target);
		}

		gensim::BaseDecode *GetNewDecode() const
		{
			return new_decoder_();
		}
		const gensim::BaseDisasm *GetDisasm() const
		{
			return disasm_;
		}
		gensim::BaseJumpInfo *GetNewJumpInfo() const
		{
			return new_jump_info_();
		}
		gensim::DecodeTranslateContext *GetNewDTC() const
		{
			return new_dtc_();
		}

		const ISABehavioursDescriptor &GetBehaviours() const
		{
			return behaviours_;
		}

		const std::string &GetName() const
		{
			return name_;
		}
		uint32_t GetID() const
		{
			return id_;
		}
	private:
		DecodeFunction decoder_;
		NewDecoderFunction new_decoder_;
		NewJumpInfoFunction new_jump_info_;
		NewDTCFunction new_dtc_;

		gensim::BaseDisasm *disasm_;
		ISABehavioursDescriptor behaviours_;
		const std::string name_;
		uint32_t id_;
	};

	/**
	 * This class contains all of the metadata about a specific architecture
	 * e.g. the layout of the register file, information about available
	 * memory interfaces, features, etc.
	 */
	class ArchDescriptor
	{
	public:
		ArchDescriptor(const std::string &name, const RegisterFileDescriptor &rf, const MemoryInterfacesDescriptor &mem, const FeaturesDescriptor &f, const std::initializer_list<ISADescriptor> &isas);

		const RegisterFileDescriptor &GetRegisterFileDescriptor() const
		{
			return register_file_;
		}
		const MemoryInterfacesDescriptor &GetMemoryInterfaceDescriptor() const
		{
			return mem_interfaces_;
		}
		const FeaturesDescriptor &GetFeaturesDescriptor() const
		{
			return features_;
		}

		const ISADescriptor &GetISA(const std::string &isaname) const
		{
			return isas_.at(isa_mode_ids_.at(isaname));
		}
		const ISADescriptor &GetISA(uint32_t mode) const
		{
			return isas_.at(mode);
		}

		const std::string &GetName() const
		{
			return name_;
		}

	private:
		const RegisterFileDescriptor register_file_;
		const MemoryInterfacesDescriptor mem_interfaces_;
		const FeaturesDescriptor features_;

		std::string name_;

		std::vector<ISADescriptor> isas_;
		std::map<std::string, uint32_t> isa_mode_ids_;
	};
}

#endif /* ARCHDESCRIPTOR_H */

