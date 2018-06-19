/*
 * MMU.h
 *
 *  Created on: 17 Jun 2014
 *      Author: harry
 */

#ifndef MMU_H_
#define MMU_H_

#include "define.h"
#include "abi/devices/Device.h"
#include "abi/devices/PeripheralManager.h"

namespace archsim
{
	namespace abi	
	{
		namespace memory
		{
			class MemoryModel;
		}

		namespace devices
		{

			struct AccessInfo {
				uint8_t Write:1, Kernel:1, Fetch:1, SideEffects:1;

				AccessInfo() {}
				AccessInfo(bool K, bool W, bool F, bool SE=0) : Write(W), Kernel(K), Fetch(F), SideEffects(SE) {}
			};

#define MMUACCESSINFO(K, W, F) archsim::abi::devices::AccessInfo(K, W, F)
#define MMUACCESSINFO2(K, W, F, SE) archsim::abi::devices::AccessInfo(K, W, F, SE)
#define MMUACCESSINFO_SE(K, W, F) archsim::abi::devices::AccessInfo(K, W, F, 1)

			class PageInfo
			{
			public:
				PageInfo();

				Address phys_addr;
				Address::underlying_t mask;

				bool Present:1;

				bool UserCanRead:1;
				bool UserCanWrite:1;
				bool KernelCanRead:1;
				bool KernelCanWrite:1;
			};

			class MMU : public Device
			{
			public:
				enum TranslateResult {
					TXLN_OK = 0,
					TXLN_FAULT_SECTION = 1,
					TXLN_FAULT_PAGE = 2,
					TXLN_FAULT_OTHER = 3,
					TXLN_ACCESS_DOMAIN_SECTION = 4,
					TXLN_ACCESS_DOMAIN_PAGE = 5,
					TXLN_ACCESS_PERMISSION_SECTION = 6,
					TXLN_ACCESS_PERMISSION_PAGE = 7,

					TXLN_INTERNAL_BASE = 1024,
					TXLN_INTERNAL_OTHER = 1024,
					TXLN_INTERNAL_EXEC_REGION_WRITE = 1025,
				};

				MMU();

				virtual ~MMU();

				virtual TranslateResult Translate(archsim::core::thread::ThreadInstance *cpu, Address virt_addr, Address &phys_addr, const struct AccessInfo info) = 0;
				virtual TranslateResult TranslateRegion(archsim::core::thread::ThreadInstance *cpu, Address virt_addr, uint32_t size, Address &phys_addr, const struct AccessInfo info);

				virtual const PageInfo GetInfo(Address virt_addr) = 0;

				virtual void FlushCaches();
				virtual void Evict(Address virt_addr);

				Address TranslateUnsafe(archsim::core::thread::ThreadInstance *cpu, Address virt_addr);

				void set_enabled(bool enabled);
				inline bool is_enabled() const
				{
					return should_be_enabled;
				}

				inline void SetPhysMem(memory::MemoryModel *mem_model)
				{
					phys_mem = mem_model;
				}

			protected:
				inline memory::MemoryModel *GetPhysMem()
				{
					return phys_mem;
				}

			private:
				bool should_be_enabled;

				memory::MemoryModel *phys_mem;
			};

		}
	}
}

inline std::ostream &operator <<(std::ostream &stream, const struct archsim::abi::devices::AccessInfo &info)
{
	stream << "[";
	if(info.Fetch) stream << "I";
	else stream << "D";
	if(info.Kernel) stream << "K";
	else stream << "U";
	if(info.Write) stream << "W";
	else stream << "R";

	stream << "]";
	return stream;
}

#endif /* MMU_H_ */
