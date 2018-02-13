/*
 * File:   ISADescription.h
 * Author: s0803652
 *
 * Created on 27 September 2011, 14:20
 */

#ifndef _ISADESCRIPTION_H
#define _ISADESCRIPTION_H

#include <assert.h>
#include <stdint.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "clean_defines.h"

#include "clean_defines.h"
#include "define.h"
#include "Util.h"

#include "isa/AsmDescription.h"
#include "isa/AsmMapDescription.h"
#include "isa/FieldDescription.h"
#include "isa/HelperFnDescription.h"
#include "isa/InstructionDescription.h"
#include "isa/InstructionFormatDescription.h"


namespace gensim
{
	namespace arch
	{
		class ArchDescription;
	}
	namespace genc
	{
		namespace ssa
		{
			class SSAContext;
		}
	}

	namespace isa
	{

		class ISADescription
		{
		public:
			//FIXME Parser should only use public interface
			friend class ISADescriptionParser;

			std::string ISAName;
			unsigned int isa_mode_id;

			typedef std::map<std::string, InstructionFormatDescription *> InstructionFormatMap;
			typedef std::map<std::string, InstructionDescription *> InstructionDescriptionMap;
			typedef std::map<std::string, AsmMapDescription> AsmMapMap;
			typedef std::map<std::string, std::string *> ActionMap;

		private:
			InstructionFormatMap formats_;
			InstructionDescriptionMap instructions_;
			AsmMapMap mappings_;
			ActionMap behaviour_actions_;
			ActionMap execute_actions_;

			genc::ssa::SSAContext *ssa_ctx_;

		public:
			bool HasFormat(const std::string &name) const
			{
				return Formats.count(name);
			}

			const InstructionFormatMap &Formats;

			const InstructionDescriptionMap &Instructions;

			const AsmMapMap &Mappings;

			const ActionMap &ExecuteActions;
			const ActionMap &BehaviourActions;

			std::vector<std::string> BehaviourFiles;
			std::vector<std::string> DecodeFiles;
			std::vector<std::string> ExecuteFiles;

			bool valid;
			bool var_length_insns;
			uint8_t insn_len;

			/**
			 * Return a map of each field, and its corresponding length, in bits
			 */
			std::map<std::string, uint32_t> &Get_Decode_Fields() const;
			std::map<std::string, std::string> &Get_Disasm_Fields() const;
			std::map<std::string, std::string> Get_Hidden_Fields();

			std::list<FieldDescription> UserFields;

			std::vector<HelperFnDescription> HelperFns;

			bool HasOrthogonalFields(bool verbose) const;
			bool IsFieldOrthogonal(const std::string &field) const;

			std::list<std::string> GetPropertyList() const;

			inline const genc::ssa::SSAContext &GetSSAContext() const
			{
				GASSERT(ssa_ctx_ != nullptr);
				return *ssa_ctx_;
			}
			inline genc::ssa::SSAContext &GetSSAContext()
			{
				GASSERT(ssa_ctx_ != nullptr);
				return *ssa_ctx_;
			}
			inline void SetSSAContext(genc::ssa::SSAContext* ctx)
			{
				ssa_ctx_ = ctx;
			}

			bool BuildSSAContext(gensim::arch::ArchDescription *arch, gensim::DiagnosticContext &diag_ctx);

			inline uint16_t GetFetchLength() const
			{
				if (fetchLengthSet)
					return FetchLength;
				else {
					fprintf(stderr, "Fetch length not set, assuming 16-bit\n");
					SetFetchLength(16);
					return FetchLength;
				}
			}

			inline uint16_t GetMaxInstructionLength() const
			{
				uint16_t maxinsnlen = 0;
				for(const auto &format : Formats) {
					if(format.second->GetLength() > maxinsnlen) maxinsnlen = format.second->GetLength();
				}
				return maxinsnlen;
			}

			void SetFetchLength(int length) const
			{
				fetchLengthSet = true;
				FetchLength = length;
			}

			inline bool GetDefaultPredicated() const
			{
				if (defaultPredicatedSet) return defaultPredicated;

				fprintf(stderr, "Default predication mode not set, assuming IS PREDICATED\n");
				SetDefaultPredicated(true);

				return defaultPredicated;
			}

			inline void SetDefaultPredicated(bool dp) const
			{
				defaultPredicatedSet = true;
				defaultPredicated = dp;
			}

			inline std::string GetProperty(const std::string key) const
			{
				return Properties.at(key);
			}

			inline bool HasProperty(const std::string key) const
			{
				return Properties.find(key) != Properties.end();
			}

			inline void SetProperty(const std::string key, const std::string val)
			{
				Properties[key] = val;
			}

			inline bool HasExecuteAction(const std::string key) const
			{
				return (ExecuteActions.find(key) != ExecuteActions.end()) && (ExecuteActions.at(key) != NULL);
			}

			inline bool HasBehaviourAction(const std::string key) const
			{
				return (BehaviourActions.find(key) != BehaviourActions.end()) && (BehaviourActions.at(key) != NULL);
			}

			inline const std::string GetExecuteAction(std::string key) const
			{
				try {
					return *execute_actions_.at(key);
				} catch (std::out_of_range &e) {
					std::cerr << "No execute action found with name " << key << ", aborting\n";
					abort();
				}
			}

			inline const std::string GetBehaviourAction(std::string key) const
			{
				try {
					return *behaviour_actions_.at(key);
				} catch (std::out_of_range &e) {
					std::cerr << "No behaviour found with name " << key << ", aborting\n";
					abort();
				}
			}

			inline void AddFormat(InstructionFormatDescription *format)
			{
				formats_[format->GetName()] = format;
				fieldsSet = false;
				disasmFieldsSet = false;
				if (insn_len == 0) insn_len = format->GetLength();
				if (format->GetLength() != insn_len) var_length_insns = true;
			}

			inline void AddInstruction(InstructionDescription *desc)
			{
				instructions_[desc->Name] = desc;
			}

			inline void AddMapping(AsmMapDescription map)
			{
				mappings_[map.Name] = map;
			}

			inline void SetBehaviour(std::string insName, std::string behaviourName)
			{
				instructions_[insName]->BehaviourName = behaviourName;
			}

			inline void AddExecuteAction(std::string key, std::string val)
			{
				assert(ExecuteActionDeclared(key));
				if (execute_actions_[key] != NULL)
					execute_actions_[key]->assign(val);
				else
					execute_actions_[key] = new std::string(val);
			}

			inline void DeclareExecuteAction(const std::string &key)
			{
				assert(!ExecuteActionDeclared(key));
				execute_actions_[key] = NULL;
			}

			inline bool ExecuteActionDeclared(const std::string &key) const
			{
				return execute_actions_.count(key);
			}

			inline bool ExecuteActionDefined(const std::string &key) const
			{
				return ExecuteActionDeclared(key) && (execute_actions_.at(key) != NULL);
			}

			inline void AddBehaviourAction(std::string key, std::string val)
			{
				if (behaviour_actions_[key] != NULL)
					behaviour_actions_[key]->assign(val);
				else
					behaviour_actions_[key] = new std::string(val);
			}

			void AddHelperFn(const std::string& prototype, const std::string& body);

			inline bool hasInstruction(std::string n)
			{
				return instructions_.find(n) != instructions_.end();
			}

			void CleanupBehaviours();

			ISADescription(uint8_t);
			virtual ~ISADescription();

		private:
			// disallow copy and assignment
			ISADescription(const ISADescription &orig, uint8_t);
			ISADescription &operator=(const ISADescription &orig);

			std::map<std::string, std::string> Properties;

			mutable bool defaultPredicatedSet;
			mutable bool defaultPredicated;

			mutable bool fetchLengthSet;
			mutable uint8_t FetchLength;

			mutable bool fieldsSet;
			mutable std::map<std::string, uint32_t> fields;

			mutable bool disasmFieldsSet;
			mutable std::map<std::string, std::string> disasmFields;

		};

	}  // namespace isa

}  // namespace gensim

#endif /* _ISADESCRIPTION_H */
