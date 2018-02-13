/*
 * ArmCoprocessor.h
 *
 *  Created on: 18 Jun 2014
 *      Author: harry
 */

#ifndef ARMCOPROCESSOR_H_
#define ARMCOPROCESSOR_H_

#include "abi/devices/Device.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class ArmCoprocessor : public archsim::abi::devices::Device
			{
			protected:
				uint32_t opc1, opc2, rn, rm;

			public:
				virtual bool Initialise() override;

				virtual bool access_cp0(bool is_read, uint32_t &data);
				virtual bool access_cp1(bool is_read, uint32_t &data);
				virtual bool access_cp2(bool is_read, uint32_t &data);
				virtual bool access_cp3(bool is_read, uint32_t &data);
				virtual bool access_cp4(bool is_read, uint32_t &data);
				virtual bool access_cp5(bool is_read, uint32_t &data);
				virtual bool access_cp6(bool is_read, uint32_t &data);
				virtual bool access_cp7(bool is_read, uint32_t &data);
				virtual bool access_cp8(bool is_read, uint32_t &data);
				virtual bool access_cp9(bool is_read, uint32_t &data);
				virtual bool access_cp10(bool is_read, uint32_t &data);
				virtual bool access_cp11(bool is_read, uint32_t &data);
				virtual bool access_cp12(bool is_read, uint32_t &data);
				virtual bool access_cp13(bool is_read, uint32_t &data);
				virtual bool access_cp14(bool is_read, uint32_t &data);
				virtual bool access_cp15(bool is_read, uint32_t &data);

				virtual bool Read32(uint32_t addr, uint32_t &data) override;
				virtual bool Write32(uint32_t addr, uint32_t data) override;

				void dump_reads() const
				{
					for(const auto &read : reads) {
						fprintf(stderr, "%08x : %u\n", read.first, read.second);
					}
				}

				void dump_writes() const
				{
					for(const auto &write : writes) {
						fprintf(stderr, "%08x : %u\n", write.first, write.second);
					}
				}

			private:
				std::map<uint32_t, uint32_t> reads, writes;

				void instrument_read()
				{
					uint32_t hash = (rn << 24) | (rm << 16) | (opc1 << 8) | opc2;
					reads[hash]++;
				}

				void instrument_write()
				{
					uint32_t hash = (rn << 24) | (rm << 16) | (opc1 << 8) | opc2;
					writes[hash]++;
				}
			};

		}
	}
}

#endif /* ARMCOPROCESSOR_H_ */
