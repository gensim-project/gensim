/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef _FUNCTIONALDECODEGENERATOR_H
#define _FUNCTIONALDECODEGENERATOR_H

#include "DecodeGenerator.h"

#include <vector>

namespace gensim
{
	namespace generator
	{

		class FunctionalDecodeGenerator : public DecodeGenerator
		{
		public:
			FunctionalDecodeGenerator(GenerationManager &man);

		private:
			FunctionalDecodeGenerator(const FunctionalDecodeGenerator &orig);

			virtual bool GenerateDecodeHeader(util::cppformatstream &str) const;
			virtual bool GenerateDecodeSource(util::cppformatstream &str) const;
			virtual bool GenerateDecodeTree(const isa::ISADescription &isa, DecodeNode &tree, util::cppformatstream &stream, int &i) const;

			virtual bool GenerateDecodeLeaf(const isa::ISADescription &isa, const isa::InstructionDescription &insn, util::cppformatstream &stream) const;
			virtual bool GenerateFormatDecoder(const isa::ISADescription &isa, const isa::InstructionFormatDescription &format, util::cppformatstream &stream) const;

			virtual bool EmitExtraClassMembers(util::cppformatstream &stream) const;

			virtual bool EmitDecodeFields(const isa::ISADescription &isa, const isa::InstructionFormatDescription &format, util::cppformatstream &stream) const;
			virtual bool EmitEndOfBlockHandling(const isa::ISADescription &isa, const isa::InstructionFormatDescription &format, util::cppformatstream &stream) const;
			virtual bool EmitDecodeBehaviourCall(const isa::ISADescription &isa, const isa::InstructionFormatDescription &format, util::cppformatstream &stream) const;
		};

	}  // namespace generator
}  // namespace gensim

#endif /* _FUNCTIONALDECODEGENERATOR_H */
