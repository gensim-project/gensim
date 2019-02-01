/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * GenericInterruptController.h
 *
 *  Created on: 6 Jan 2016
 *      Author: kuba
 */

#ifndef ARCSIM_INC_ABI_DEVICES_ARM_EXTERNAL_GENERICINTERRUPTCONTROLLER_H_
#define ARCSIM_INC_ABI_DEVICES_ARM_EXTERNAL_GENERICINTERRUPTCONTROLLER_H_


#include "abi/devices/Component.h"
#include "abi/devices/IRQController.h"

#include <atomic>
#include <array>
#include <stdio.h>
namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class GIC;
			class GICCPUInterface;
			class GICDistributorInterface;
			class GICIRQLine;

			class GICCPUInterface : public MemoryComponent
			{
				friend class GIC;
				friend class GICDistributorInterface;

			public:
				GICCPUInterface(EmulationModel &emu_model, Address address);
				~GICCPUInterface();

				bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;
				bool Write(uint32_t offset, uint8_t size, uint64_t data) override;

				bool enabled() const
				{
					return ctrl & 1;
				}

				bool Initialise() override;

				COMPONENT_PARAMETER_ENTRY_HDR(IRQLine, IRQLine, IRQLine);
				COMPONENT_PARAMETER_ENTRY_HDR(Owner, GIC.GIC, GIC);
			private:
				uint32_t last_active[96], running_irq;
				uint32_t ctrl, prio_mask, binpnt;
				uint32_t current_pending, running_priority;

				void update();
				uint32_t acknowledge();
				void complete(uint32_t irq);
			};

			class GICDistributorInterface : public MemoryComponent
			{
				friend class GIC;
				friend class GICCPUInterface;

			public:
				GICDistributorInterface(EmulationModel& emu_model, Address address);
				~GICDistributorInterface();

				bool Read(uint32_t offset, uint8_t size, uint64_t& data) override;
				bool Write(uint32_t offset, uint8_t size, uint64_t data) override;
				int GetComponentID()
				{
					return 0;
				}

				bool Initialise() override;

				COMPONENT_PARAMETER_ENTRY_HDR(Owner, GIC.GIC, GIC);
			private:
				uint32_t ctrl;

				uint32_t cpu_targets[24];
				uint32_t config[6];

				inline bool enabled() const
				{
					return !!(ctrl & 1);
				}

				void set_enabled(uint32_t base, uint8_t bits);
				void clear_enabled(uint32_t base, uint8_t bits);
				void set_pending(uint32_t base, uint8_t bits);
				void clear_pending(uint32_t base, uint8_t bits);
			};

			class GIC : public IRQController // <96u>
			{
				friend class GICCPUInterface;
				friend class GICDistributorInterface;

			public:
				GIC(EmulationModel& model);
				~GIC();
				GICCPUInterface& get_cpu(int id)
				{
					assert(id == 0);
					return *GetCpuInterface();
				}
				GICDistributorInterface& get_distributor()
				{
					return *GetDistributor();
				}
				CPUIRQLine* GetIRQLine(uint32_t);
				virtual bool AssertLine(uint32_t line_no) override; // captive irq_raised
				virtual bool RescindLine(uint32_t line_no) override; // captive irq_rescinded

				bool Initialise() override;
			private:
				std::mutex lock;

				COMPONENT_PARAMETER_ENTRY_HDR(CpuInterface, GIC.CpuInterface, GICCPUInterface);
				COMPONENT_PARAMETER_ENTRY_HDR(Distributor, GIC.Distributor, GICDistributorInterface);

				std::vector<GICIRQLine> irqs; // captive irqs struct array

				GICIRQLine& get_gic_irq(int index)
				{
					assert(index < 96);
					return irqs[index];
				}

				void update();

			};

			class GICIRQLine
			{
			public:
				GICIRQLine(int i);
				int GetIndex()
				{
					return index;
				}
				void SetIndex(int i)
				{
					index = i;
				}

				int index;
				bool enabled;
				bool pending;
				bool active;
				bool model;
				bool raised;
				bool edge_triggered;
				uint32_t priority;

				void Assert();
				void Rescind();
//	protected:
//		GIC *Controller;
//		bool Acknowledged;
			};
		}
	}
}


#endif /* ARCSIM_INC_ABI_DEVICES_ARM_EXTERNAL_GENERICINTERRUPTCONTROLLER_H_ */
