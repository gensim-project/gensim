/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "generators/ExecutionEngine/BlockJITExecutionEngineGenerator.h"

using namespace gensim::generator;

BlockJITExecutionEngineGenerator::BlockJITExecutionEngineGenerator(GenerationManager &man) : EEGenerator(man, "ee_blockjit")
{

}


bool BlockJITExecutionEngineGenerator::GenerateHeader(util::cppformatstream& str) const
{
	return true;
}

bool BlockJITExecutionEngineGenerator::GenerateSource(util::cppformatstream& str) const
{
	return true;
}

DEFINE_COMPONENT(BlockJITExecutionEngineGenerator, ee_blockjit);