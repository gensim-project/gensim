/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "abi/devices/MMU.h"

namespace archsim
{
	namespace arch
	{
		namespace riscv
		{

			class RiscVMMU : public archsim::abi::devices::MMU
			{
			public:
				enum Mode {
					NoTxln,
					Sv32,
					Sv39,
					Sv48
				};

				class PTE
				{
				public:
					PTE(uint64_t data) : data_(data) {}

					Address::underlying_t GetPPN(Mode mode, int level) const;
					Address::underlying_t GetPPN() const;
					bool GetD() const
					{
						return GetDataBit(7);
					}
					bool GetA() const
					{
						return GetDataBit(6);
					}
					bool GetG() const
					{
						return GetDataBit(5);
					}
					bool GetU() const
					{
						return GetDataBit(4);
					}
					bool GetX() const
					{
						return GetDataBit(3);
					}
					bool GetW() const
					{
						return GetDataBit(2);
					}
					bool GetR() const
					{
						return GetDataBit(1);
					}
					bool GetV() const
					{
						return GetDataBit(0);
					}

					uint64_t GetData() const
					{
						return data_;
					}
				private:
					bool GetDataBit(int i) const
					{
						return (data_ >> i) & 1;
					}
					uint64_t data_;
				};

				bool Initialise() override;

				MMU::TranslateResult Translate(archsim::core::thread::ThreadInstance* cpu, Address virt_addr, Address& phys_addr, archsim::abi::devices::AccessInfo info) override;
				const archsim::abi::devices::PageInfo GetInfo(Address virt_addr) override;

				Mode GetMode() const;

				uint64_t GetSATP() const;
				void SetSATP(uint64_t new_satp);
				int GetPTLevels(Mode mode) const;

			private:
				using PTEInfo = std::tuple<archsim::Address, archsim::abi::devices::PageInfo>;
				PTEInfo GetInfoLevel(Address virt_addr, Address table, int level);
				PTE LoadPTE(Mode mode, Address address);

				Address::underlying_t GetVPN(Mode mode, Address addr, int vpn);
				Address::underlying_t GetPTESize(Mode mode);

				Mode mode_;
				uint64_t page_table_ppn_;
				uint64_t asid_;

			};
		}
	}
}