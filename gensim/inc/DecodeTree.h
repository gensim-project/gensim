/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   DecodeTree.h
 * Author: s0803652
 *
 * Created on 28 September 2011, 14:02
 */

#ifndef _DECODETREE_H
#define _DECODETREE_H

#include "isa/ISADescription.h"

#include <iomanip>
#include <ostream>

namespace gensim
{
	namespace generator
	{

		class DecodeNode;

		class DecodedInstruction
		{
		public:
			typedef std::map<std::string, uint32_t> ChunkMap;

			const isa::InstructionDescription *insn;
			ChunkMap chunks;
		};

		class DecodeTransition
		{
		public:
			uint8_t length;
			uint32_t value;
			DecodeNode *target;

			DecodeTransition(uint8_t _l, uint32_t _v, DecodeNode *_t) : length(_l), value(_v), target(_t) {}

			// break this transition at bit (pos)
			void Break(uint8_t pos);

			void PrettyPrint(std::ostream &str) const
			{
				str << "((" << (uint32_t)length << ", " << std::hex << value << ") -> " << target << ")";
			}
		private:
			DecodeTransition();
		};

		class DecodeNode
		{
		public:
			uint8_t start_ptr;

			typedef std::multimap<uint8_t, DecodeTransition> TransitionMultimap;
			TransitionMultimap transitions;

			DecodeTransition *unconstrained_transition;

			// if this is a leaf node, the target indicates the decoded instruction
			const isa::InstructionDescription *target;

			uint32_t node_number;

			// break all transitions
			void BreakAllTransitions(uint8_t pos);

			// break the unconstrained transition into two separate transitions
			// e.g. a --xxxx--> c   ==>   a --xx--> b --xx--> c
			void BreakUnconstrainedTransition(uint8_t pos);

			// add a new instruction to this subtree. Should only be called at the root of the tree
			bool AddInstruction(const isa::InstructionDescription &instruction);

			// group together subnodes with common transition prefixes. The prefixes must be at least
			// min_prefix_length long before nodes are grouped to avoid degeneration
			void Optimize(int min_prefix_length, uint8_t min_prefix_members);

			// Merge together subnodes with common transitions
			void MergeCommonNodes();

			// Recursively count the number of nodes in the tree
			uint32_t CountNodes();

			DecodedInstruction *Decode(uint32_t remaining_bits);

			virtual ~DecodeNode();

			static DecodeNode *CreateTree(const isa::InstructionDescription &first_instruction);

			explicit DecodeNode(const isa::InstructionDescription *inst, uint8_t _start_ptr) : unconstrained_transition(NULL), target(NULL), node_number(max_node_num++)
			{
				start_ptr = _start_ptr;
				if (inst && start_ptr == inst->Format->GetLength()) target = inst;
			};

		private:
			static uint32_t max_node_num;

			DecodeNode(const DecodeNode &);

			DecodeNode() : unconstrained_transition(NULL), target(NULL), node_number(max_node_num++), start_ptr(0) {};
			DecodeNode &operator=(const DecodeNode &orig);
			bool AddInstruction(const isa::InstructionDescription *instruction, std::string bitstring);
		};

	}  // namespace generator
}  // namespace gensim

#endif /* _DECODETREE_H */
