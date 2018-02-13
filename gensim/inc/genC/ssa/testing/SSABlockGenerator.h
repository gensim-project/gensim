/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   SSABlockGenerator.h
 * Author: harry
 *
 * Created on 22 April 2017, 14:30
 */

#ifndef SSABLOCKGENERATOR_H
#define SSABLOCKGENERATOR_H

#include "define.h"
#include "genC/ssa/SSAContext.h"

#include <random>
#include <vector>

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

			class SSAFormAction;
			class SSABlock;
			class SSAContext;

			namespace testing
			{
				class SSABlockGenerator
				{
				public:
					typedef std::mt19937_64 random_t;
					typedef std::vector<SSAStatement *> value_container_t;
					typedef std::vector<SSASymbol *> symbol_container_t;
					typedef std::vector<SSABlock *> target_container_t;
					typedef std::vector<SSAFormAction *> callee_container_t;
					typedef SSAStatement *(*generator_fn_t)(SSABlock*, SSABlockGenerator *);
					typedef SSAControlFlowStatement *(*controlflow_generator_fn_t)(SSABlock*, SSABlockGenerator *);
					SSABlockGenerator(random_t &random, unsigned stmts, SSAFormAction *action, const std::vector<SSABlock*>& possible_targets, SSAContext& ctx);

					SSABlock *Generate();
					void GenerateStatement(SSABlock *block);
					void GenerateControlFlow(SSABlock *block);

					random_t::result_type Random()
					{
						return _random();
					}
					value_container_t &Values()
					{
						return _values;
					}
					const arch::ArchDescription *Arch()
					{
						return &_context.GetArchDescription();
					}
					const target_container_t &Targets()
					{
						return _possible_targets;
					}

					SSAStatement *RandomValue()
					{
						if(_values.empty()) return nullptr;
						else return _values.at(_random() % _values.size());
					}
					IRType RandomType();
					SSASymbol *RandomSymbol();
					SSABlock *RandomTarget()
					{
						return _possible_targets.at(Random() % _possible_targets.size());
					}
					SSAFormAction *RandomCallee();
				private:
					void FillGenerators();

					SSAContext& _context;

					const unsigned _stmts_per_block;
					SSAFormAction *_action;
					const target_container_t &_possible_targets;
					value_container_t _values;
					symbol_container_t _symbols;
					callee_container_t _callees;
					std::vector<generator_fn_t> _generators;
					std::vector<controlflow_generator_fn_t> _generators_controlflow;
					random_t &_random;
				};
			}
		}
	}
}

#endif /* SSABLOCKGENERATOR_H */

