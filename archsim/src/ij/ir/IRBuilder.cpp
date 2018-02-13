#include "ij/ir/IRBuilder.h"

using namespace archsim::ij::ir;

IRBuilder::IRBuilder(IJIR& ir, IJIRBlock& block) : ir(ir), current_block(&block)
{

}