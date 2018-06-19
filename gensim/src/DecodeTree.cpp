/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <assert.h>

#include <map>
#include <set>

#include "define.h"
#include "DecodeTree.h"

#include "Util.h"
#include <cstring>

namespace gensim
{
	namespace generator
	{

		uint32_t DecodeNode::max_node_num = 0;

		std::string GetPrefix(std::string str, uint8_t start)
		{
			char *chars = (char *)malloc(str.length() - start + 1);
			memset(chars, 0, str.length() - start + 1);

			uint8_t i = start;
			if (str[i] == 'x') {
				while (str[i] == 'x') {
					chars[i - start] = 'x';
					i++;
				}
			} else {
				while (str[i] != 'x' && i < str.length()) {
					chars[i - start] = str[i];
					i++;
				}
			}

			std::string out(chars);
			free(chars);
			return out;
		}

		void DecodeTransition::Break(uint8_t pos)
		{
			if (pos == 0 || pos == length) return;
			if (pos >= length) {
				target->BreakAllTransitions(pos - length);
				target->BreakUnconstrainedTransition(pos - length);
				return;
			}
			// create a new DecodeNode;
			DecodeNode *newTarget = new DecodeNode(target->target, target->start_ptr - length + pos);
			DecodeTransition trans(length - pos, UNSIGNED_BITS(value, length - pos, 0), target);
			newTarget->transitions.insert(std::make_pair(length - pos, trans));

			length = pos;
			value = UNSIGNED_BITS(value, length, length - pos);
			target = newTarget;
		}

		DecodeNode::~DecodeNode()
		{
			// recursively delete the tree
			for (std::multimap<uint8_t, DecodeTransition>::iterator i = transitions.begin(); i != transitions.end(); ++i) {
				delete i->second.target;
			}

			if (unconstrained_transition != NULL) {
				delete unconstrained_transition->target;
				delete unconstrained_transition;
			}
		}

		DecodeNode *DecodeNode::CreateTree(const isa::InstructionDescription &first_instruction)
		{
			DecodeNode *tree = new DecodeNode(&first_instruction, 0);
			tree->AddInstruction(first_instruction);
			return tree;
		}

		void DecodeNode::BreakAllTransitions(uint8_t pos)
		{
			// make a copy of transitions, clear transition list, break all transitions in list copy and re-add them to transition list
			TransitionMultimap transition_copy = transitions;
			transitions.clear();
			for (TransitionMultimap::iterator i = transition_copy.begin(); i != transition_copy.end(); ++i) {
				DecodeTransition trans = i->second;
				trans.Break(pos);
				transitions.insert(std::make_pair(trans.length, trans));
			}
		}

		void DecodeNode::BreakUnconstrainedTransition(uint8_t pos)
		{
			if (!unconstrained_transition) return;
			if (unconstrained_transition->length <= pos) {
				unconstrained_transition->target->BreakAllTransitions(pos - unconstrained_transition->length);
				unconstrained_transition->target->BreakUnconstrainedTransition(pos - unconstrained_transition->length);
				return;
			}
			DecodeTransition *new_transition = new DecodeTransition(unconstrained_transition->length - pos, 0, unconstrained_transition->target);
			DecodeNode *new_node = new DecodeNode(NULL, start_ptr + pos);
			new_node->unconstrained_transition = new_transition;

			unconstrained_transition->length = pos;
			unconstrained_transition->target = new_node;
		}

		/**
		 * This function merges two decode nodes together, including handling the unconstrained transition mergine.
		 */
		void MergeNodes(DecodeNode &target, DecodeNode &other)
		{

			assert(target.start_ptr == other.start_ptr);

			target.transitions.insert(other.transitions.begin(), other.transitions.end());
			other.transitions.clear();

			// now we need to deal with unconstrained transitions.
			// if the duplicate node has no unconstrained transition, we don't need to do any merging
			if (target.unconstrained_transition) {
				// if the original node has no unconstrained transition, we can just move the transition over
				if (other.unconstrained_transition)
					// otherwise we need to do some actual merging

					// if the two unconstrained transitions are the same length, we can directly merge the unconstrained targets
					if (target.unconstrained_transition->length == other.unconstrained_transition->length) {
						MergeNodes(*target.unconstrained_transition->target, *other.unconstrained_transition->target);
					} else {
						// if the transitions are different lengths, we need to figure out which one is the longest (a), split it at the length of the shortest (b) into c -> d)
						// so we then have (a->c->d and b->x), where length(a->c) == length(b->x) then, we merge x into c.
						DecodeTransition *longest, *shortest;
						if (target.unconstrained_transition->length < other.unconstrained_transition->length) {
							longest = other.unconstrained_transition;
							shortest = target.unconstrained_transition;
						} else {
							longest = target.unconstrained_transition;
							shortest = other.unconstrained_transition;
						}

						longest->Break(shortest->length);
						MergeNodes(*longest->target, *shortest->target);
					}
			} else if (other.unconstrained_transition)
				target.unconstrained_transition = other.unconstrained_transition;
		}

		void DecodeNode::MergeCommonNodes()
		{
			// first, partition the transition map into multiple separate maps, by transition length
			std::map<uint8_t, std::list<std::pair<DecodeTransition, TransitionMultimap::iterator> > > parted_transitions;
			for (TransitionMultimap::iterator ci = transitions.begin(); ci != transitions.end(); ++ci) {
				if (parted_transitions.find(ci->first) == parted_transitions.end()) parted_transitions[ci->first] = std::list<std::pair<DecodeTransition, TransitionMultimap::iterator> >();
				parted_transitions[ci->first].push_back(std::make_pair(ci->second, ci));
			}

			// now loop through each group length
			for (std::map<uint8_t, std::list<std::pair<DecodeTransition, TransitionMultimap::iterator> > >::iterator group_i = parted_transitions.begin(); group_i != parted_transitions.end(); ++group_i) {
				// for each group, keep a list of already-seen transitions. If we see the same transition multiple times, merge the transitions
				std::map<std::pair<uint8_t, uint32_t>, DecodeNode *> seen_transitions;
				std::list<std::list<DecodeTransition>::iterator> duplicates;
				for (std::list<std::pair<DecodeTransition, TransitionMultimap::iterator> >::iterator i = group_i->second.begin(); i != group_i->second.end(); ++i) {
					std::pair<uint8_t, uint32_t> decode_pair = std::make_pair(i->first.length, i->first.value);
					// if this is the first time we've seen a transition like this, add it to the list
					if (seen_transitions.find(decode_pair) == seen_transitions.end())
						seen_transitions[decode_pair] = i->first.target;
					else {
						// othewise, we need to merge the nodes. The simplest part of this
						// is merging the constrained transitions, since we can just add all
						// of the transitions from the duplicate node to the original node
						DecodeNode *orig_node = seen_transitions[decode_pair];
						DecodeNode *duplicate_node = i->first.target;

						MergeNodes(*orig_node, *duplicate_node);

						transitions.erase(i->second);
					}
				}
			}

			for (TransitionMultimap::iterator i = transitions.begin(); i != transitions.end(); ++i) i->second.target->MergeCommonNodes();
			if (unconstrained_transition) unconstrained_transition->target->MergeCommonNodes();
		}

		bool DecodeNode::AddInstruction(const isa::InstructionDescription &instruction)
		{
			bool success = true;
			std::vector<std::string> bitstrings = instruction.GetBitString();
			for (std::vector<std::string>::iterator i = bitstrings.begin(); i != bitstrings.end(); ++i) {
				if (!AddInstruction(&instruction, *i)) return false;
			}
			return success;
		}

		bool DecodeNode::AddInstruction(const isa::InstructionDescription *instruction, std::string bitstring)
		{
			bool success = true;
			// if we're at the end of the instruction, bail out
			if (start_ptr == bitstring.length()) {
				if (instruction && target && instruction != target) {
					fprintf(stderr, "ISA Conflict detected: %s and %s overlap.\n", instruction->Name.c_str(), target->Name.c_str());
					return false;
				}
				return true;
			}
			// get the initial prefix of this bitstring
			std::string prefix = GetPrefix(bitstring, start_ptr);

			// if the prefix is constrained,
			if (prefix[0] == 'x') {
				// if we currently don't have an unconstrained transition, create a new one and pass the
				// instruction on to it
				if (unconstrained_transition == NULL) {
					DecodeNode *unconstrained_node = new DecodeNode(instruction, (uint8_t)(start_ptr + prefix.length()));
					unconstrained_transition = new DecodeTransition(prefix.length(), 0, unconstrained_node);
					success &= unconstrained_node->AddInstruction(instruction, bitstring);
					return success;
				} else {
					// we have an unconstrained transition but we need to make sure this prefix is the same length.
					// if the prefix is the same length or longer, we pass the instruction through
					if (prefix.length() >= unconstrained_transition->length) {
						success &= unconstrained_transition->target->AddInstruction(instruction, bitstring);
						return success;
					} else {
						// the transition is shorter so we need to break the unconstrained transition into two parts
						// and pass the instruction through to the first part.
						BreakUnconstrainedTransition(prefix.length());
						success &= unconstrained_transition->target->AddInstruction(instruction, bitstring);
						return success;
					}
				}
			} else {
				// the prefix of the instruction is completely defined
				// first, look for a transition which matches this prefix
				uint32_t val = util::Util::parse_binary(prefix);
				std::pair<std::multimap<uint8_t, DecodeTransition>::iterator, std::multimap<uint8_t, DecodeTransition>::iterator> matches = transitions.equal_range(prefix.length());

				std::multimap<uint8_t, DecodeTransition>::iterator i = matches.first;
				while (i != matches.second) {
					if (i->second.value == val) {
						break;
					}
					++i;
				}
				if (i != matches.second) {
					// we've found a match
					i->second.target->AddInstruction(instruction, bitstring);
				} else {
					// no current transition exists with that prefix: create a new one
					DecodeNode *target = new DecodeNode(instruction, start_ptr + prefix.length());
					DecodeTransition trans(prefix.length(), val, target);
					transitions.insert(std::pair<uint8_t, DecodeTransition>(prefix.length(), trans));
					success &= target->AddInstruction(instruction, bitstring);
				}
			}
			return success;
		}

		void DecodeNode::Optimize(int min_prefix_length, uint8_t min_prefix_members)
		{
			// collect a list of possible grouped nodes (i.e. don't attempt to group nodes shorter than this
			std::list<DecodeTransition> candidates;
			for (std::multimap<uint8_t, DecodeTransition>::iterator trans_it = transitions.begin(); trans_it != transitions.end(); ++trans_it) {
				if (trans_it->first >= min_prefix_length) candidates.push_back(trans_it->second);
			}

			// if we don't have any candidates go straight to optimizing subnodes
			if (candidates.size() != 0) {

				// a multimap from a prefix to a transition. each prefix is represented as a length and then a value
				std::multimap<std::pair<uint8_t, uint32_t>, DecodeTransition> prefixes;
				for (std::list<DecodeTransition>::iterator trans_it = candidates.begin(); trans_it != candidates.end(); ++trans_it) {

					for (int start = trans_it->length - min_prefix_length; start >= min_prefix_length; --start) {
						uint8_t length = start;
						uint32_t value = UNSIGNED_BITS(trans_it->value, trans_it->length - 1, trans_it->length - start);
						prefixes.insert(std::pair<std::pair<uint8_t, uint32_t>, DecodeTransition>(std::pair<uint8_t, uint32_t>(length, value), *trans_it));

						assert(!(value != 0 && (32 - __builtin_clz(value) > length)) && "Prefix Length Error");
					}
				}

				// now sort the prefixes by how popular they are
				std::map<std::pair<uint8_t, uint32_t>, uint32_t> prefix_popularity;
				for (std::multimap<std::pair<uint8_t, uint32_t>, DecodeTransition>::iterator pref_it = prefixes.begin(); pref_it != prefixes.end(); ++pref_it) {
					if (prefix_popularity.find(pref_it->first) == prefix_popularity.end()) prefix_popularity[pref_it->first] = 0;

					prefix_popularity[pref_it->first] = prefix_popularity[pref_it->first] + 1;
				}

				// now invert the prefix_popularity map to get a ranking of prefixes
				std::multimap<uint32_t, std::pair<uint8_t, uint32_t> > prefix_ranking;
				for (std::map<std::pair<uint8_t, uint32_t>, uint32_t>::iterator pref_it = prefix_popularity.begin(); pref_it != prefix_popularity.end(); ++pref_it) {
					if (pref_it->second >= min_prefix_members) {
						prefix_ranking.insert(std::pair<uint32_t, std::pair<uint8_t, uint32_t> >(pref_it->second, pref_it->first));
					}
				}

				// now select which prefixes will be split. All selected prefixes must be mutually exclusive,
				// i.e. a transition cannot belong to more than one. we use a reverse iterator to get prefixes
				// from highest to lowest key
				std::list<std::pair<uint8_t, uint32_t> > selected_prefixes;
				for (std::multimap<uint32_t, std::pair<uint8_t, uint32_t> >::reverse_iterator it = prefix_ranking.rbegin(); it != prefix_ranking.rend(); ++it) {
					bool overlap = false;

					// check that this prefix is mutually exclusive with all previously selected prefixes
					// this will remove prefixes which are prefixes of other prefixes.
					for (std::list<std::pair<uint8_t, uint32_t> >::iterator p_it = selected_prefixes.begin(); p_it != selected_prefixes.end(); p_it++) {
						// checking for prefix overlap:
						// 1. take prefixes a and b, where a.length < b.length
						// 2. if(a == b<1 : a.length>) return PREFIX_OVERLAP
						// 3. else return PREFIX_DISTINCT

						std::string a = util::Util::FormatBinary(p_it->second, p_it->first);
						std::string b = util::Util::FormatBinary(it->second.second, it->second.first);
						if (a.find(b) == 0 || b.find(a) == 0) {
							overlap = true;
							break;
						}
					}
					if (!overlap) {
						selected_prefixes.push_back(it->second);
					}
				}

				// now create new nodes for each selected prefix and fix up the tree. if a transition already
				// exists then it becomes a zero-length unconstrained transition from the new node. at code
				// generation time, such a node simply becomes a fall-through from the previous node.
				for (std::list<std::pair<uint8_t, uint32_t> >::iterator p_it = selected_prefixes.begin(); p_it != selected_prefixes.end(); ++p_it) {
					// first, get a list of transitions matching this prefix. we remove them from the
					// transition list at the same time to avoid having to loop through the list again later
					std::list<DecodeTransition> matches;
					// keep track of those transitions we are going to left-factor so that we can remove them from this node
					std::list<std::multimap<uint8_t, DecodeTransition>::iterator> matched_transitions;
					for (std::multimap<uint8_t, DecodeTransition>::iterator trans_it = transitions.begin(); trans_it != transitions.end(); ++trans_it) {
						// first check to make sure this transition isn't shorter than the prefix
						if (trans_it->second.length >= p_it->first)
							// now check to see if it actually matches the prefix
							if (UNSIGNED_BITS(trans_it->second.value, trans_it->second.length - 1, trans_it->second.length - p_it->first) == p_it->second) {
								matched_transitions.push_back(trans_it);
								matches.push_back(trans_it->second);
							}
					}

					// remove the left-factorization candidates
					for (std::list<std::multimap<uint8_t, DecodeTransition>::iterator>::iterator it_it = matched_transitions.begin(); it_it != matched_transitions.end(); ++it_it) {
						transitions.erase(*it_it);
					}

					// create a new intermediate node (representing a transition along the prefix)
					DecodeNode *newNode = new DecodeNode();
					newNode->start_ptr = start_ptr + p_it->first;

					// create a transition from this node to the intermediate node
					DecodeTransition this_to_intermediate_trans = DecodeTransition(p_it->first, p_it->second, newNode);
					transitions.insert(std::pair<uint8_t, DecodeTransition>(p_it->first, this_to_intermediate_trans));

					// now loop through the matching transitions and add a modified version to the new node.
					for (std::list<DecodeTransition>::iterator trans_it = matches.begin(); trans_it != matches.end(); ++trans_it) {
						DecodeTransition matching_trans = *trans_it;
						matching_trans.value = UNSIGNED_BITS(matching_trans.value, matching_trans.length - this_to_intermediate_trans.length - 1, 0);
						matching_trans.length -= this_to_intermediate_trans.length;

						if (matching_trans.length == 0) {
							newNode->unconstrained_transition = new DecodeTransition(matching_trans);
						} else {
							newNode->transitions.insert(std::pair<uint8_t, DecodeTransition>(matching_trans.length, matching_trans));
						}
					}
				}
			}
			// now, optimize the nodes which are children of this node
			for (std::multimap<uint8_t, DecodeTransition>::iterator trans_it = transitions.begin(); trans_it != transitions.end(); ++trans_it) {
				trans_it->second.target->Optimize(min_prefix_length, min_prefix_members);
			}
			if (unconstrained_transition != NULL) unconstrained_transition->target->Optimize(min_prefix_length, min_prefix_members);
		}

		uint32_t DecodeNode::CountNodes()
		{
			if (target) return 1;
			uint32_t count = 1;
			for (TransitionMultimap::const_iterator ci = transitions.begin(); ci != transitions.end(); ++ci) {
				count += ci->second.target->CountNodes();
			}
			if (unconstrained_transition) count += unconstrained_transition->target->CountNodes();
			return count;
		}

// TODO: Modify to support non-32-bit ISAs
		DecodedInstruction *DecodeNode::Decode(uint32_t bits)
		{
			if (target) {
				// Now decode the fields in the instruction
				const isa::InstructionDescription *id = target;
				const isa::InstructionFormatDescription *format = id->Format;
				DecodedInstruction *di = new DecodedInstruction();
				di->insn = target;

				uint32_t insn_ptr = 31;

				for (isa::InstructionFormatDescription::ChunkList::const_iterator ci = format->GetChunks().begin(); ci != format->GetChunks().end(); ++ci) {
					std::pair<std::string, uint32_t> chunk;
					chunk.first = ci->Name;
					chunk.second = UNSIGNED_BITS(bits, insn_ptr, insn_ptr - ci->length + 1);
					di->chunks.insert(chunk);
					insn_ptr -= ci->length;
				}
				return di;
			}

			for (TransitionMultimap::const_iterator trans_ci = transitions.begin(); trans_ci != transitions.end(); ++trans_ci) {
				const DecodeTransition &trans = trans_ci->second;
				uint32_t chunk = UNSIGNED_BITS(bits, 31 - start_ptr, 31 - start_ptr - trans.length + 1);
				if (chunk == trans.value) {
					DecodedInstruction *dec = trans.target->Decode(bits);
					if (dec) return dec;
				}
			}

			if (unconstrained_transition) return unconstrained_transition->target->Decode(bits);
			return NULL;
		}

	}  // namespace generator
}  // namespace gensim
