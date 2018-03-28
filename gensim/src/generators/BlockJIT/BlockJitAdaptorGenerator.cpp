/*
 * BlockJitAdaptorGenerator.cpp
 *
 *  Created on: 24 Aug 2015
 *      Author: harry
 */

#include "generators/GenerationManager.h"
#include "arch/ArchDescription.h"

#include "generators/GenCInterpreter/GenCInterpreterGenerator.h"

namespace gensim
{
	namespace generator
	{

		class BlockJitAdaptorGenerator : public GenCInterpreterGenerator
		{
		public:
			BlockJitAdaptorGenerator(GenerationManager &mgr) : GenCInterpreterGenerator(mgr)
			{
				SetProperty("ParentClass", "gensim::BlockJitProcessor");
			}
			virtual ~BlockJitAdaptorGenerator() {}

			virtual bool GenerateExtraProcessorIncludes(util::cppformatstream &stream) const
			{
				GenCInterpreterGenerator::GenerateExtraProcessorIncludes(stream);
				stream << "#include <gensim/gensim_processor_blockjit.h>\n";
				stream << "#include \"jit.h\"\n";
				return true;
			}

			virtual bool GenerateStepSingle(util::cppformatstream & stream) const override
			{
				stream << "bool " << GetProperty("class") << "::step_single()\n{\n";
				stream << "assert(false);";
				stream << "}";
				return true;
			}
			virtual bool GenerateStepSingleFast(util::cppformatstream & stream) const override
			{
				stream << "bool " << GetProperty("class") << "::step_single_fast()\n{\n";
				stream << "assert(false);";
				stream << "}";
				return true;
			}
			virtual bool GenerateStepInstruction(util::cppformatstream & stream) const override
			{
				stream << "bool " << GetProperty("class") << "::step_instruction(bool trace)\n{\n";
				stream << "if (trace) return step_single(); else return step_single_fast();";
				stream << "}\n";
				return true;
			}
			virtual bool GenerateStepBlockFast(util::cppformatstream &stream) const override
			{
				stream << "bool Processor::step_block_fast() {";
				stream << "return BlockJitProcessor::step_block_fast();";
				stream << "return true;";
				stream << "}";
				return true;
			}
			virtual bool GenerateStepBlockFastThread(util::cppformatstream &stream) const override
			{
				stream << "bool Processor::step_block_fast_thread() {";
				stream << "return false;";
				stream << "}";
				return true;
			}
			virtual bool GenerateStepBlockTrace(util::cppformatstream &stream) const override
			{
				// Implemented by base class
				stream << "bool Processor::step_block_trace() {";
				stream << "return BlockJitProcessor::step_block_fast();";
				stream << "}";
				return true;
			}

			virtual bool GenerateExtraProcessorClassMembers(util::cppformatstream &stream) const override
			{
				stream << "gensim::blockjit::BaseBlockJITTranslate *CreateBlockJITTranslate() const;";
				return true;
			}

			virtual bool GenerateExtraProcessorSource(util::cppformatstream &stream) const override
			{
				stream << "gensim::blockjit::BaseBlockJITTranslate *Processor::CreateBlockJITTranslate() const { ";
				stream << "return new captive::arch::" << Manager.GetArch().Name << "::JIT();";
				stream << "}";
				return true;
			}

		};

	}
}

using gensim::generator::BlockJitAdaptorGenerator;
DEFINE_COMPONENT(BlockJitAdaptorGenerator, adapt_blkjit)
COMPONENT_INHERITS(adapt_blkjit, interpret)
