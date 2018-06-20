/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "GenericInterruptController.h"
#include "abi/EmulationModel.h"
#include "util/LogContext.h"
#include "abi/devices/generic/timing/TickSource.h"

#include <cassert>
#include <stdio.h>
#include <string.h>

UseLogContext(LogArmExternalDevice);
DeclareChildLogContext(LogGIC, LogArmExternalDevice, "GIC");

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			COMPONENT_PARAMETER_ENTRY_SRC(GICCPUInterface, IRQLine, IRQLine, IRQLine);
			COMPONENT_PARAMETER_ENTRY_SRC(GICCPUInterface, Owner, GIC.GIC, GIC);
			COMPONENT_PARAMETER_ENTRY_SRC(GICDistributorInterface, Owner, GIC.GIC, GIC);
			COMPONENT_PARAMETER_ENTRY_SRC(GIC, CpuInterface, GIC.CpuInterface, GICCPUInterface);
			COMPONENT_PARAMETER_ENTRY_SRC(GIC, Distributor, GIC.Distributor, GICDistributorInterface);

			static ComponentDescriptor gicdistributor_descriptor ("GICDistributor", {{"Owner", ComponentParameter_Component}});
			/* GIC Distributor Interface*/
			GICDistributorInterface::GICDistributorInterface(EmulationModel& emu_model, Address address) : MemoryComponent(emu_model, address, 0x1000), ctrl(0), Component(gicdistributor_descriptor)
			{
				for (int i = 0; i < 24; i++) {
					cpu_targets[i] = 0x00000000;
				}
			}

			GICDistributorInterface::~GICDistributorInterface()
			{

			}

			bool GICDistributorInterface::Initialise()
			{
				return true;
			}

			bool GICDistributorInterface::Read(uint32_t off, uint8_t len, uint32_t& data)
			{
				LC_DEBUG2(LogGIC) << "Distributor read: offset " << off;
				switch (off) {
					case 0x00:
						data = ctrl;
//		fprintf(stderr, "Distributor Control register: %x\n", ctrl);
						return true;

					case 0x04:
						data = 0x2;
						return true;

					case 0x100 ... 0x108:
						data = 0; //set_enable[(off & 0xf) >> 2];
						return true;

					case 0x180 ... 0x188:
						data = 0; //clear_enable[(off & 0xf) >> 2];
						return true;

					case 0x200 ... 0x208:
						data = 0; //set_pending[(off & 0xf) >> 2];
						return true;

					case 0x280 ... 0x288:
						data = 0; //clear_pending[(off & 0xf) >> 2];
						return true;

					case 0x400 ... 0x45c:
						data = 0; //prio[(off & 0x7f) >> 2];
						return true;

					case 0x800 ... 0x85c:
						// for now, we're a 'uniprocessor implementation (RAZ/WI)'
						data = 0;
//		data = cpu_targets[(off & 0x7f) >> 2];
						return true;

					case 0xc00 ... 0xc14:
						data = config[(off & 0x1f) >> 2];
						return true;

					// ID registers
					case 0xfe0:
						data = 0x90;
						return true;
					case 0xfe4:
						data = 0x13;
						return true;
					case 0xfe8:
						data = 0x4;
						return true;
					case 0xfec:
						data = 0x0;
						return true;
				}

				return false;
			}

			void GICDistributorInterface::set_enabled(uint32_t base, uint8_t bits)
			{
				LC_DEBUG2(LogGIC) << "Distributor set enabled " << (uint32_t)bits;
				for (int i = 0; i < 8; i++) {
					if (bits & (1 << i)) {
						GICIRQLine& irq = GetOwner()->get_gic_irq(base + i);

						irq.enabled = true;
						if (irq.raised && !irq.edge_triggered) {
							irq.pending = true;
						}
					}
				}
			}

			void GICDistributorInterface::clear_enabled(uint32_t base, uint8_t bits)
			{
				LC_DEBUG2(LogGIC) << "Distributor clear enabled " << (uint32_t)bits;
				for (int i = 0; i < 8; i++) {
					if (bits & (1 << i)) {
						GetOwner()->get_gic_irq(base + i).enabled = false;
					}
				}
			}

			void GICDistributorInterface::set_pending(uint32_t base, uint8_t bits)
			{
				LC_DEBUG2(LogGIC) << "Distributor set pending " << (uint32_t)bits;
				for (int i = 0; i < 8; i++) {
					if (bits & (1 << i)) {
						GetOwner()->get_gic_irq(base + i).pending = true;
					}
				}
			}

			void GICDistributorInterface::clear_pending(uint32_t base, uint8_t bits)
			{
				LC_DEBUG2(LogGIC) << "Distributor clear pending " << (uint32_t)bits;
				for (int i = 0; i < 8; i++) {
					if (bits & (1 << i)) {
						GetOwner()->get_gic_irq(base + i).pending = false;
					}
				}
			}

			/* GIC Distributor Interface*/

			bool GICDistributorInterface::Write(uint32_t off, uint8_t len, uint32_t data)
			{
				LC_DEBUG2(LogGIC) << "Distributor write offset " << std::hex << (uint32_t)off << ", data: " << std::hex << data;

				switch (off) {
					case 0x00:
						ctrl = data & 1;
//		fprintf(stderr, "Distributor Control register: %x\n", ctrl);
						GetOwner()->update();
						return true;

					case 0x100 ... 0x108: {
						uint32_t base = (off - 0x100) * 8;
						for (int i = 0; i < len; i++, base += 8, data >>= 8) {
							set_enabled(base, (uint8_t)(data & 0xff));
						}

						GetOwner()->update();
						return true;
					}

					case 0x180 ... 0x188: {
						uint32_t base = (off - 0x180) * 8;
						for (int i = 0; i < len; i++, base += 8, data >>= 8) {
							clear_enabled(base, (uint8_t)(data & 0xff));
						}

						GetOwner()->update();
						return true;
					}

					case 0x200 ... 0x208: {
						uint32_t base = (off - 0x200) * 8;
						for (int i = 0; i < len; i++, base += 8, data >>= 8) {
							set_pending(base, (uint8_t)(data & 0xff));
						}

						GetOwner()->update();
						return true;
					}

					case 0x280 ... 0x288: {
						uint32_t base = (off - 0x280) * 8;
						for (int i = 0; i < len; i++, base += 8, data >>= 8) {
							clear_pending(base, (uint8_t)(data & 0xff));
						}

						GetOwner()->update();
						return true;
					}

					case 0x400 ... 0x45c: {
						uint32_t irqi = (off - 0x400);

						for (int i = 0; i < len; i++, data >>= 8) {
							GetOwner()->get_gic_irq(irqi + i).priority = data & 0xff;
						}

						GetOwner()->update();
						return true;
					}

					case 0x800 ... 0x85c:
						cpu_targets[(off & 0x7f) >> 2] = data;
						return true;

					case 0xc00 ... 0xc14:
#ifdef DEBUG_IRQ
						fprintf(stderr, "gic: config %x\n", data);
#endif
						config[(off & 0x1f) >> 2] = data;
						return true;

					case 0xf00:
						// just ping an IRQ to the only CPU we have
						// XXX this is probably wrong
//		owner.get_gic_irq(data & 0x3f).pending = 1;
						LC_ERROR(LogGIC) << "SGI " << std::hex << off << " accessed in GIC";
						return true;

					default:
						LC_ERROR(LogGIC) << "Unknown register " << std::hex << off << " accessed in GIC";
						return false;
				}


				return false;
			}

			/* GIC CPU Interface*/

			static ComponentDescriptor giccpu_descriptor ("GICCPUInterface", {{"Owner", ComponentParameter_Component}, {"IRQLine", ComponentParameter_Component}});
			GICCPUInterface::GICCPUInterface(EmulationModel& emu_model, Address address) : MemoryComponent(emu_model, address, 0x1000), Component(giccpu_descriptor), ctrl(0), prio_mask(0), binpnt(3), current_pending(1023), running_irq(1023), running_priority(0x100)
			{
				for (int i = 0; i < 96; i++) {
					last_active[i] = 1023;
				}
			}

			GICCPUInterface::~GICCPUInterface()
			{

			}

			bool GICCPUInterface::Initialise()
			{
				return true;
			}

			bool GICCPUInterface::Read(uint32_t off, uint8_t len, uint32_t& data)
			{
				LC_DEBUG2(LogGIC) << "CPU Interface read " << std::hex << off;
				switch (off) {
					case 0x00:
						data = ctrl; // Control Register (GICC_CTLR)
						break;

					case 0x04:		// CPU Priority Mask (GICC_PMR)
						data = prio_mask;
						break;

					case 0x08:		// Binary Point (GICC_BPR)
						data = binpnt;
						break;

					case 0x0c:		// Interrupt Acknowledge (GICC_IAR)
						data = acknowledge();
						break;

					case 0x14:		// Running Interrupt Priority (GICC_RPR)
						data = running_priority;
						break;

					case 0x18:		// Highest Pending Interrupt (GICC_HPPIR)
						data = current_pending;
						break;
					default:
						fprintf(stderr, "gic: cpu: unknown register read %02x\n", off);
						return false;
				}

				return true;
			}

			bool GICCPUInterface::Write(uint32_t off, uint8_t len, uint32_t data)
			{
				LC_DEBUG2(LogGIC) << "CPU Interface write " << std::hex << off << ", data " << data;
				switch (off) {
					case 0x00:
						ctrl = data & 1;
						update();
						return true;

					case 0x04:
						prio_mask = data & 0xff;
						update();
						return true;

					case 0x08:
						binpnt = data & 0x7;
						update();
						return true;

					case 0x10:
						complete(data);
						return true;
				}

				fprintf(stderr, "gic: cpu: unknown register write %02x\n", off);
				return false;
			}

			void GICCPUInterface::update()
			{
				LC_DEBUG2(LogGIC) << "CPU Interface update";

				current_pending = 1023;
				if (!enabled() || !GetOwner()->GetDistributor()->enabled()) {
					GetIRQLine()->Rescind();
					return;
				}

				uint32_t best_prio = 0x100;
				uint32_t best_irq = 1023;

				for (int irqi = 0; irqi < 96; irqi++) {
					GICIRQLine& irq = GetOwner()->get_gic_irq(irqi);

					if (irq.enabled && (irq.pending || (!irq.edge_triggered && irq.raised))) {
						if (irq.priority < best_prio) {
							best_prio = irq.priority;
							best_irq = irqi;
						}
					}
				}

				bool raise = false;
				if (best_prio < prio_mask) {

					current_pending = best_irq;
					if (best_prio < running_priority) {
						raise = true;
					}
				}

				if (raise) {
					LC_DEBUG2(LogGIC) << "CPU Interface: ASSERT IRQ";
					GetIRQLine()->Assert();
				} else {
					LC_DEBUG2(LogGIC) << "CPU Interface: RESCIND IRQ";
					GetIRQLine()->Rescind();
				}
			}

			uint32_t GICCPUInterface::acknowledge()
			{
#ifdef DEBUG_IRQ
				fprintf(stderr, "gic: acknowledge %d\n", current_pending);
#endif

				uint32_t irq = current_pending;
				if (irq == 1023 || GetOwner()->get_gic_irq(irq).priority >= running_priority) {
					return 1023;
				}

				last_active[irq] = running_irq;

				GetOwner()->get_gic_irq(irq).pending = false;
				running_irq = irq;
				if (irq == 1023) {
					running_priority = 0x100;
				} else {
					running_priority = GetOwner()->get_gic_irq(irq).priority;
				}

				update();
				return irq;
			}

			void GICCPUInterface::complete(uint32_t irq)
			{
#ifdef DEBUG_IRQ
				fprintf(stderr, "gic: complete %d running=%d\n", irq, running_irq);
#endif

				if (irq >= 96) {
					return;
				}

				if (running_irq == 1023) {
					return;
				}

				if (irq != running_irq) {
					int tmp = running_irq;

					while (last_active[tmp] != 1023) {
						if (last_active[tmp] == irq) {
							last_active[tmp] = last_active[irq];
							break;
						}

						tmp = last_active[tmp];
					}
				} else {
#ifdef DEBUG_IRQ
					fprintf(stderr, "running irq1: %x\n", running_irq);
#endif
					running_irq = last_active[running_irq];
#ifdef DEBUG_IRQ
					fprintf(stderr, "running irq2: %x\n", running_irq);
#endif
					if (running_irq == 1023) {
						running_priority = 0x100;
					} else {
						running_priority = GetOwner()->get_gic_irq(running_irq).priority;
					}
				}

				update();
			}

			/* GIC */

			static ComponentDescriptor gic_descriptor ("GIC.GIC", {{"CpuInterface", ComponentParameter_Component}, {"Distributor", ComponentParameter_Component}});
			GIC::GIC(EmulationModel& model) :
				IRQController(96), Component(gic_descriptor)
			{
				for (int i = 0; i < 96; i++) {
					irqs.push_back(GICIRQLine(i));
				}
			}

			GIC::~GIC()
			{
			}

			bool GIC::Initialise()
			{
				return true;
			}

			bool GIC::AssertLine(uint32_t l)
			{
#ifdef DEBUG_IRQ
				printf("AssertLine: %x\n", l);
#endif
				GICIRQLine* irq = &irqs[l];

				if (!irq->raised) {
#ifdef DEBUG_IRQ
					fprintf(stderr, "gic: raise %d\n", irq->index);
					fflush(stderr);
#endif

//		fprintf(stderr, "GIC: Asserted line %u at tick %lu\n", l, this->cpu.parent_model.GetSystem().GetTickSource()->GetCounter());

					irq->raised = true;
					if (irq->edge_triggered) irq->pending = true;

					update();
				}

				return true;
			}


			bool GIC::RescindLine(uint32_t l)
			{
#ifdef DEBUG_IRQ
				fprintf(stderr, "RescindLine: %x\n", l);
				fflush(stderr);
#endif
				GICIRQLine* irq = &irqs[l];

				if (irq->raised) {
#ifdef DEBUG_IRQ
					fprintf(stderr, "gic: rescind %d\n", irq->index);
					fflush(stderr);
#endif

					irq->raised = false;
					update();
				}

				return true;
			}

			void GIC::update()
			{
#ifdef DEBUG_IRQ
				fprintf(stderr, "GIC::update\n");
#endif
				GetCpuInterface()->update();
			}

			/* GIC IRQ LINE */

			GICIRQLine::GICIRQLine(int i)
			{
				index = 0;
				enabled = false;
				pending = false;
				active = false;
				model = false;
				raised = false;
				edge_triggered = false;
				priority = 0;
			}

			void GICIRQLine::Assert()
			{
//	printf("Assert GICIRQLine!\n GenericInteruptHandler.cpp:438\n");
				exit(0);
			}
			void GICIRQLine::Rescind()
			{
//	printf("Rescind GICIRQLine!\n GenericInteruptHandler.cpp:438\n");
				exit(0);
			}

		}
	}
}
