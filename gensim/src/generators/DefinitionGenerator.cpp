/*
 * File:   DefinitionGenerator.cpp
 * Author: s0803652
 *
 * Created on 07 October 2011, 15:50
 */

#include <fstream>

#include "generators/DefinitionGenerator.h"

namespace gensim
{
	namespace generator
	{

		bool DefinitionGenerator::Generate()
		{
			std::string header = Manager.GetTarget();

			const GenerationComponent *decodeComponent = Manager.GetComponentC(GenerationManager::FnDecode);

			header.append("gensim/define.h");

			std::ofstream file(header.c_str());
			file << "#ifndef _DEFINE_H \n"
			     "#define	_DEFINE_H \n"
			     "/* Always have these. */ \n"
			     "#define UNSIGNED_BITS(v,u,l)  (((uint32_t)(v)<<(31-(u))>>(31-(u)+(l)))) \n"
			     "#define SIGNED_BITS(v,u,l)    (((int32_t)(v)<<(31-(u))>>(31-(u)+(l)))) \n"
			     "#define BITSEL(v,b)           (((v)>>b) & 1UL) \n"
			     "#define BIT_LSB(i)                  (1 << (i)) \n"
			     "#define BIT_32_MSB(i)                  (0x80000000 >> (i)) \n"
			     "#define ac_modifier_decode(x) uint32_t modifier_decode_ ## x(uint32_t input, " << decodeComponent->GetProperty("class") << " insn, uint32_t pc)\n"
			     "#endif	/* _DEFINE_H */ \n";

			file.close();

			return true;
		}

		DefinitionGenerator::~DefinitionGenerator() {}

	}  // namespace generator
}  // namespace gensim
