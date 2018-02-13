/*
 * TranslationProvider.h
 *
 *  Created on: 17 Jun 2014
 *      Author: harry
 */

#ifndef TRANSLATIONPROVIDER_H_
#define TRANSLATIONPROVIDER_H_

#include <stdint.h>

#include "abi/devices/MMU.h"

namespace gensim
{
	class Processor;
}

namespace archsim
{
	namespace abi
	{

		namespace memory
		{

			class TranslationProvider
			{
			public:
				virtual ~TranslationProvider();

				virtual uint32_t Translate(gensim::Processor *cpu, uint32_t virt_addr, uint32_t &phys_addr, const struct archsim::abi::devices::AccessInfo info) = 0;
			};

			class MMUBasedTranslationProvider : public TranslationProvider
			{
			public:
				MMUBasedTranslationProvider(devices::MMU &MMU);
				virtual ~MMUBasedTranslationProvider();

				virtual uint32_t Translate(gensim::Processor *cpu, uint32_t virt_addr, uint32_t &phys_addr, const struct archsim::abi::devices::AccessInfo info) override;
			private:
				devices::MMU &mmu;
			};

		}

	}
}


#endif /* TRANSLATIONPROVIDER_H_ */
