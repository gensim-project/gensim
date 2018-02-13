/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   SSAInterpreter.h
 * Author: harry
 *
 * Created on 22 April 2017, 08:43
 */

#ifndef SSAINTERPRETER_H
#define SSAINTERPRETER_H

#include "genC/ir/IRConstant.h"
#include "genC/ssa/testing/MachineState.h"

#include <map>

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
			class SSAStatement;
			class SSABlock;
			class SSASymbol;
			class SSAFormAction;

			namespace testing
			{

				enum InterpretResult {
					Interpret_Normal,
					Interpret_Exception,
					Interpret_Halt,
					Interpret_Error
				};

				class VMActionState
				{
				public:
					typedef std::unordered_map<SSAStatement*, std::pair<uint64_t, IRConstant>> statement_values_t; // timestamped statements so that phi statements can figure out which value to use
					typedef std::unordered_map<SSASymbol*, IRConstant> symbol_values_t;

					statement_values_t &Statements()
					{
						return statements_;
					}
					const statement_values_t &Statements() const
					{
						return statements_;
					}

					InterpretResult GetResult()
					{
						return interpret_result_;
					}
					SSABlock *GetNextBlock()
					{
						return next_block_;
					}

					void SetResult(InterpretResult result)
					{
						interpret_result_ = result;
					}
					void SetNextBlock(SSABlock *next_block)
					{
						next_block_ = next_block;
					}

					void SetStatementValue(SSAStatement *stmt, IRConstant value);
					void SetSymbolValue(SSASymbol *stmt, IRConstant value);

					uint64_t GetStatementTimestamp(SSAStatement *stmt) const;
					const IRConstant &GetStatementValue(SSAStatement *stmt);
					const IRConstant &GetSymbolValue(SSASymbol *symbol);
					bool HasStatementValue(SSAStatement* stmt) const
					{
						return statements_.count(stmt);
					}

					void Reset();

					const IRConstant &GetReturnValue() const
					{
						return return_value_;
					}
					void SetReturnValue(const IRConstant &val)
					{
						return_value_ = val;
					}
				private:
					statement_values_t statements_;
					symbol_values_t symbols_;

					InterpretResult interpret_result_;
					SSABlock *next_block_;
					IRConstant return_value_;
					uint64_t timestamp_;
				};

				class ActionResult
				{
				public:
					InterpretResult Result;
					IRConstant ReturnValue;

					std::map<int, IRConstant> ReferenceParameterValues;
				};

				class SSAInterpreter
				{
				public:
					typedef MachineStateInterface machine_state_t;
					typedef std::map<std::string, IRConstant> instruction_t;

					SSAInterpreter(const arch::ArchDescription *arch, machine_state_t &target_state);
					void Reset();
					ActionResult ExecuteAction(const SSAFormAction* action, const std::vector<IRConstant> &param_values);

					void SetTracing(bool trace_on)
					{
						trace_ = trace_on;
					}
					machine_state_t &State()
					{
						return state_;
					}
				private:
					struct BlockResult {
						SSABlock *NextBlock;
						InterpretResult Result;
					};

					BlockResult ExecuteBlock(SSABlock *block, VMActionState &state);

					machine_state_t &state_;

					const arch::ArchDescription *arch_;

					bool trace_;
				};

			}
		}
	}
}

#endif /* SSAINTERPRETER_H */

