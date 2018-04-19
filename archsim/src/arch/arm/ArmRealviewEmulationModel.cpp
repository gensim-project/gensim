/*
 * arch/arm/ArmSystemEmulationModel.cpp
 */
#include "arch/arm/ArmRealviewEmulationModel.h"
#include "arch/arm/ARMDecodeContext.h"

#include "abi/loader/BinaryLoader.h"
#include "abi/devices/SerialPort.h"
#include "abi/devices/generic/ps2/PS2Device.h"
#include "abi/devices/generic/block/FileBackedBlockDevice.h"
#include "abi/devices/generic/net/TapInterface.h"
#include "abi/devices/virtio/VirtIOBlock.h"
#include "abi/devices/virtio/VirtIONet.h"
#include "abi/devices/arm/special/SimulatorCacheControlCoprocessor.h"
#include "abi/devices/gfx/VirtualScreen.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"

#include "session.h"
#include "gensim/gensim_processor.h"
#include "abi/devices/WSBlockDevice.h"

extern char bootloader_start, bootloader_end;
extern uint32_t bootloader_size;

using namespace archsim::arch::arm;
using namespace archsim::abi;
using namespace archsim::abi::memory;
using namespace archsim::module;

RegisterComponent(EmulationModel, ArmRealviewEmulationModel, "arm-realview", "ARM system emulation model")

UseLogContext(LogSystemEmulationModel);
DeclareChildLogContext(LogArmSystemEmulationModel, LogSystemEmulationModel, "ARM");

ArmRealviewEmulationModel::ArmRealviewEmulationModel() : entry_point(0)
{

}

ArmRealviewEmulationModel::~ArmRealviewEmulationModel()
{
}

bool ArmRealviewEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	if (!SystemEmulationModel::Initialise(system, uarch))
		return false;

	// Initialise "physical memory".
	if (GetMemoryModel().GetMappingManager())
		GetMemoryModel().GetMappingManager()->MapAll((archsim::abi::memory::RegionFlags)7);

	return true;
}

void ArmRealviewEmulationModel::Destroy()
{
	SystemEmulationModel::Destroy();
}

bool ArmRealviewEmulationModel::InstallBootloader(archsim::abi::loader::BinaryLoader& loader)
{
	entry_point = loader.GetEntryPoint();
	GetMemoryModel().Write(0, (uint8_t *)&bootloader_start, bootloader_size);

	return true;
}

bool ArmRealviewEmulationModel::InstallPlatform(loader::BinaryLoader& loader)
{
	if (!LinuxSystemEmulationModel::InstallPlatform(loader))
		return false;

	return InstallBootloader(loader);
}

bool ArmRealviewEmulationModel::PrepareCore(gensim::Processor& core)
{
	uint32_t *regs = (uint32_t *)core.GetRegisterBankDescriptor("RB").GetBankDataStart();

	// r0 = 0
	regs[0] = 0;

	// r1 = CPU Type
	regs[1] = 0x769;

	// r2 = Device Tree / ATAGS
	if (GetATAGSComponent().valid) {
		regs[2] = GetATAGSComponent().address;
	} else if (GetDeviceTreeComponent().valid) {
		regs[2] = GetDeviceTreeComponent().address;
	} else {
		regs[2] = 0;
	}

	// IP = Entry Point
	regs[12] = entry_point;

	return true;
}

bool ArmRealviewEmulationModel::HackyMMIORegisterDevice(abi::devices::MemoryComponent& device)
{
#define IO_ADDRESS(x) (((x) & 0x0fffffff) + (((x) >> 4) & 0x0f000000) + 0xf0000000)
	return RegisterMemoryComponent(device);
}

bool ArmRealviewEmulationModel::AddGenericPrimecellDevice(Address base_addr, uint32_t size, uint32_t peripheral_id)
{
	assert(size == 0x1000);
	auto device = GetSystem().GetModuleManager().GetModuleEntry<module::ModuleDeviceEntry>("devices.arm.system.GenericPrimecellDevice")->Get(*this, base_addr);
	device->SetParameter<uint64_t>("PeripheralID", peripheral_id);
	HackyMMIORegisterDevice(*device);
	return true;
}

bool ArmRealviewEmulationModel::InstallPlatformDevices()
{
	LC_INFO(LogArmSystemEmulationModel) << "Installing platform devices";

	auto &mm = GetSystem().GetModuleManager();
	auto adm = mm.GetModule("devices.arm.system");

	//SP810 System controller - registered at multiple places in the device space
	auto sp810 = adm->GetEntry<ModuleDeviceEntry>("SP810")->Get(*this, Address(0x10000000));
	if(!HackyMMIORegisterDevice(*sp810)) return false;

	archsim::abi::devices::gfx::VirtualScreenManagerBase *screen_man = NULL;
	if(!GetComponentInstance(archsim::options::ScreenManagerType, screen_man)) {
		fprintf(stderr, "Could not instantiate Screen Manager %s!\n%s\n", archsim::options::ScreenManagerType.GetValue().c_str(), GetRegisteredComponents(screen_man).c_str());;
		return false;
	}

	auto screen = screen_man->CreateScreenInstance("lcd", &GetMemoryModel(), &GetSystem());

	screen->SetKeyboard(GetSystem().GetSession().global_kbd);
	screen->SetMouse(GetSystem().GetSession().global_mouse);

	auto gic_distributor = adm->GetEntry<ModuleDeviceEntry>("GIC.Distributor")->Get(*this, Address(0x1e001000));
	auto gic_cpuinterface = adm->GetEntry<ModuleDeviceEntry>("GIC.CpuInterface")->Get(*this, Address(0x1e000000));

	auto gic = adm->GetEntry<ModuleComponentEntry>("GIC.GIC")->Get(*this);
	gic_distributor->SetParameter("Owner", gic);
	gic_cpuinterface->SetParameter("Owner", gic);
	gic_cpuinterface->SetParameter("IRQLine", (archsim::abi::devices::Component*)GetBootCore()->GetIRQLine(1));
	auto irq_controller = dynamic_cast<archsim::abi::devices::IRQController*>(gic);

	gic->SetParameter("CpuInterface", gic_cpuinterface);
	gic->SetParameter("Distributor", gic_distributor);

	if(!HackyMMIORegisterDevice(*gic_cpuinterface)) return false;
	if(!HackyMMIORegisterDevice(*gic_distributor)) return false;


	auto timer0 = adm->GetEntry<ModuleDeviceEntry>("SP804")->Get(*this, Address(0x10011000));
	timer0->SetParameter("IRQLine", irq_controller->RegisterSource(36));
	if(!HackyMMIORegisterDevice(*timer0)) return false;

	auto timer1 = adm->GetEntry<ModuleDeviceEntry>("SP804")->Get(*this, Address(0x10012000));
	timer1->SetParameter("IRQLine", irq_controller->RegisterSource(37));
	if(!HackyMMIORegisterDevice(*timer1)) return false;

	auto timer2 = adm->GetEntry<ModuleDeviceEntry>("SP804")->Get(*this, Address(0x10018000));
	timer2->SetParameter("IRQLine", irq_controller->RegisterSource(73));
	if(!HackyMMIORegisterDevice(*timer2)) return false;

	auto timer3 = adm->GetEntry<ModuleDeviceEntry>("SP804")->Get(*this, Address(0x10019000));
	timer3->SetParameter("IRQLine", irq_controller->RegisterSource(74));
	if(!HackyMMIORegisterDevice(*timer3)) return false;

	// SP805 Watchdog 0
//	AddGenericPrimecellDevice(0x1000f000, 0x1000, 0xf0f0f0f0); //TODO: fill in Peripheral ID

	// SP805 Watchdog 1
//	AddGenericPrimecellDevice(0x10010000, 0x1000, 0xf0f0f0f0); //TODO: fill in Peripheral ID

	//GPIOs
	auto pl061a = adm->GetEntry<ModuleDeviceEntry>("PL061")->Get(*this, Address(0x10013000));
	auto pl061b = adm->GetEntry<ModuleDeviceEntry>("PL061")->Get(*this, Address(0x10014000));
	auto pl061c = adm->GetEntry<ModuleDeviceEntry>("PL061")->Get(*this, Address(0x10015000));

	pl061a->SetParameter("IRQLine", irq_controller->RegisterSource(38));
	pl061b->SetParameter("IRQLine", irq_controller->RegisterSource(39));
	pl061c->SetParameter("IRQLine", irq_controller->RegisterSource(40));

	if (!HackyMMIORegisterDevice(*pl061a)) return false;
	if (!HackyMMIORegisterDevice(*pl061b)) return false;
	if (!HackyMMIORegisterDevice(*pl061c)) return false;

	//RTC
	auto pl031 = adm->GetEntry<ModuleDeviceEntry>("PL031")->Get(*this, Address(0x10017000));
	if (!HackyMMIORegisterDevice(*pl031)) return false;

	//Advanced audio codec interface PL041
	AddGenericPrimecellDevice(Address(0x10004000), 0x1000, 0x41100429);

	// Synchronous Serial Port (PL022)
//	AddGenericPrimecellDevice(0x1000d000, 0x1000, 0x22102400);

	auto pl011 = adm->GetEntry<ModuleDeviceEntry>("PL011")->Get(*this, Address(0x10009000));
	pl011->SetParameter("IRQLine", irq_controller->RegisterSource(44));

	if(archsim::options::Verify) {
		pl011->SetParameter("SerialPort", new abi::devices::ConsoleOutputSerialPort());
	} else {
		pl011->SetParameter("SerialPort", new abi::devices::ConsoleSerialPort());
	}
	if(!HackyMMIORegisterDevice(*pl011)) return false;

	pl011 = adm->GetEntry<ModuleDeviceEntry>("PL011")->Get(*this, Address(0x1000A000));
	pl011->SetParameter("IRQLine", irq_controller->RegisterSource(45));
	if(!HackyMMIORegisterDevice(*pl011)) return false;

	pl011 = adm->GetEntry<ModuleDeviceEntry>("PL011")->Get(*this, Address(0x1000B000));
	pl011->SetParameter("IRQLine", irq_controller->RegisterSource(46));
	if(!HackyMMIORegisterDevice(*pl011)) return false;

	pl011 = adm->GetEntry<ModuleDeviceEntry>("PL011")->Get(*this, Address(0x1000C000));
	pl011->SetParameter("IRQLine", irq_controller->RegisterSource(47));
	if(!HackyMMIORegisterDevice(*pl011)) return false;

	// PL081 Direct Memory Access (DMA) Controller
	AddGenericPrimecellDevice(Address(0x10030000), 0x1000, 0x8110040a);

	// PL131 Smart Card Interface
//	AddGenericPrimecellDevice(0x1000e000, 0x1000, 0x31110400);

	// PL350 Static memory controller
//	AddGenericPrimecellDevice(0x100e1000, 0x1000, 0x93101400);

	// PL180
//	AddGenericPrimecellDevice(0x10005000, 0x1000, 0x81110400);

	// SBCON *sbcon0 = new SBCON();
	// cfg.devices.push_back(GuestDeviceConfiguration(0x10002000, *sbcon0));

	// Keyboard/Mouse Interface 0 (KB)
	devices::generic::ps2::PS2KeyboardDevice *ps2kbd = new devices::generic::ps2::PS2KeyboardDevice(*irq_controller->RegisterSource(52));
	GetSystem().GetSession().global_kbd.AddKeyboard(ps2kbd);

	auto pl050kb = adm->GetEntry<ModuleDeviceEntry>("PL050")->Get(*this, Address(0x10006000));
	pl050kb->SetParameter("PS2", ps2kbd);
	if(!HackyMMIORegisterDevice(*pl050kb)) return false;

	// Keyboard/Mouse Interface 0 (Mouse)
	devices::generic::ps2::PS2MouseDevice *ps2ms = new devices::generic::ps2::PS2MouseDevice(*irq_controller->RegisterSource(53));
	GetSystem().GetSession().global_mouse.AddMouse(ps2ms);

	auto pl050ms = adm->GetEntry<ModuleDeviceEntry>("PL050")->Get(*this, Address(0x10007000));
	pl050ms->SetParameter("PS2", ps2ms);
	if(!HackyMMIORegisterDevice(*pl050ms)) return false;

	//Color (sic) LCD Controller
	auto pl110 = adm->GetEntry<ModuleDeviceEntry>("PL110")->Get(*this, Address(0x10020000));
	pl110->SetParameter("Screen", screen);
	if(!HackyMMIORegisterDevice(*pl110)) return false;

	// VirtIO Block Device

	devices::virtio::VirtIOBlock *vbda = new devices::virtio::VirtIOBlock(*this, *irq_controller->RegisterSource(35), Address(0x10100000), "virtio-block-a", *GetSystem().GetBlockDevice("vda"));
	if (!HackyMMIORegisterDevice(*vbda)) return false;

	/*
	devices::generic::net::TapInterface *tap = new devices::generic::net::TapInterface("tap0");
	if(tap->start()) {
		devices::virtio::VirtIONet *vnet = new devices::virtio::VirtIONet(*this, *gic0->RegisterSource(34), 0x10200000, "virtio-net-a", *tap, 0x020000000001ULL);
		if(!HackyMMIORegisterDevice(*vnet)) return false;
	} else {
		LC_ERROR(LogArmSystemEmulationModel) << "Could not start tap device";
	}
	 */

//	WSBlockDevice *wsblock = new WSBlockDevice(*this, archsim::translate::profile::Address(0x10200000), *GetSystem().GetBlockDevice("vda"));
//	if (!HackyMMIORegisterDevice(*wsblock)) return false;

	//Multi-port memory controller
	//AddGenericPrimecellDevice(0x10110000, 0x10000, 0x75110447);


	//Watchdog interface
	//AddGenericPrimecellDevice(0x101e1000, 0x1000, 0x05181400);


	// Ethernet Device
	//AddGenericPrimecellDevice(0x10010000, 0x10000, 0xf0f0f0f0);

	// GPU

	if (archsim::options::GPU) {
		auto gpu_m = mm.GetModule("devices.arm.gpu");
		auto mali = gpu_m->GetEntry<ModuleDeviceEntry>("Mali")->Get(*this, Address(0x18001000));
		if (!HackyMMIORegisterDevice(*mali)) return false;

		mali->SetParameter("IRQLineGPU", irq_controller->RegisterSource(77));
		mali->SetParameter("IRQLineJOB", irq_controller->RegisterSource(75));
		mali->SetParameter("IRQLineMMU", irq_controller->RegisterSource(76));
		mali->SetParameter("GPUID", (uint64_t)0x60000000);
		mali->SetParameter("PubSubContext", (archsim::abi::devices::Component*)(&GetSystem().GetPubSub()));
		mali->SetParameter("PhysicalMemoryModel", (archsim::abi::devices::Component*)&GetMemoryModel());
		mali->SetParameter("PeripheralManager", (archsim::abi::devices::Component*)(&(main_thread_->GetPeripherals())));
		mali->Initialise();
	}
	return true;
}

bool ArmRealviewEmulationModel::InstallPeripheralDevices()
{
	LC_INFO(LogArmSystemEmulationModel) << "[ARM-SYSTEM] Installing peripheral devices";

	archsim::abi::devices::Device *coprocessor;
	if(!GetComponentInstance("armcoprocessor", coprocessor)) return false;
	main_thread_->GetPeripherals().RegisterDevice("coprocessor", coprocessor);
	main_thread_->GetPeripherals().AttachDevice("coprocessor", 15);

	if(!GetComponentInstance("armdebug", coprocessor)) return false;
	main_thread_->GetPeripherals().RegisterDevice("armdebug", coprocessor);
	main_thread_->GetPeripherals().AttachDevice("armdebug", 14);

	archsim::abi::devices::Device *mmu;
	if(!GetComponentInstance("ARMv6MMU", mmu)) return false;
	main_thread_->GetPeripherals().RegisterDevice("mmu", mmu);

	devices::SimulatorCacheControlCoprocessor *sccc = new devices::SimulatorCacheControlCoprocessor();
	main_thread_->GetPeripherals().RegisterDevice("sccc", sccc);
	main_thread_->GetPeripherals().AttachDevice("sccc", 13);

	main_thread_->GetPeripherals().InitialiseDevices();

	return true;
}

bool ArmRealviewEmulationModel::InstallDevices()
{
	return InstallPeripheralDevices() && InstallPlatformDevices();
}

void ArmRealviewEmulationModel::DestroyDevices()
{
//	archsim::abi::devices::ArmCoprocessor *coproc = (archsim::abi::devices::ArmCoprocessor *)cpu->peripherals.GetDeviceByName("coprocessor");
//	fprintf(stderr, "Reads:\n");
//	coproc->dump_reads();
//
//	fprintf(stderr, "Writes:\n");
//	coproc->dump_writes();
}

void ArmRealviewEmulationModel::HandleSemihostingCall()
{
	uint32_t *regs = nullptr; //(uint32_t *)cpu->GetRegisterBankDescriptor("RB").GetBankDataStart();
	UNIMPLEMENTED;
	
	uint32_t phys_addr = regs[1];
	switch(regs[0]) {
		case 3:
			uint8_t c;
			//cpu->translation_provider->Translate(cpu, regs[1], phys_addr, 0);
			GetMemoryModel().Read8(phys_addr, c);
			printf("\x1b[31m%c\x1b[0m", c);
			fflush(stdout);
			break;
		case 4:
			char buffer[256];
			//cpu->translation_provider->Translate(cpu, regs[1], phys_addr, 0);
			GetMemoryModel().ReadString(phys_addr, buffer, sizeof(buffer));
			printf("\x1b[31m%s\x1b[0m", buffer);
			fflush(stdout);
			break;
		case 5:
			UNIMPLEMENTED;
//			cpu->Halt();
			break;
		default:
			LC_WARNING(LogArmSystemEmulationModel) << "Unhandled semihosting API call " << regs[0];
			break;
	}
}

ExceptionAction ArmRealviewEmulationModel::HandleException(archsim::ThreadInstance *cpu, unsigned int category, unsigned int data)
{
	UNIMPLEMENTED;
	
//	LC_DEBUG4(LogSystemEmulationModel) << "Handle Exception category: " << category << " data 0x" << std::hex << data << " PC " << cpu.read_pc() << " mode " << (uint32_t)cpu.get_cpu_mode();
//
//	if (category == 3 && data == 0x123456) {
//		HandleSemihostingCall();
//		return archsim::abi::ResumeNext;
//	}
//
//	// XXX ARM HAX
//	if (category == 11) {
//		uint32_t insn;
//		cpu.mem_read_32(data-4, insn);
////		GetMemoryModel().Read32(data-4, insn);
//		LC_DEBUG1(LogSystemEmulationModel) << "Undefined instruction exception! " << std::hex << (data-4) << " " << insn;
////		exit(0);
//	}
//
//	// update ITSTATE if we have a SWI
//	if(category == 3) {
//		auto &desc = cpu.GetRegisterDescriptor("ITSTATE");
//		uint8_t* itstate_ptr = (uint8_t*)desc.DataStart;
//		uint8_t itstate = *itstate_ptr;
//		if(itstate) {
//			uint8_t cond = itstate & 0xe0;
//			uint8_t mask = itstate & 0x1f;
//			mask = (mask << 1) & 0x1f;
//			if(mask == 0x10) {
//				cond = 0;
//				mask = 0;
//			}
//			*itstate_ptr = cond | mask;
//		}
//	}
//
//	cpu.handle_exception(category, data);
//	return archsim::abi::AbortInstruction;
}

gensim::DecodeContext* ArmRealviewEmulationModel::GetNewDecodeContext(gensim::Processor& cpu)
{
	return new archsim::arch::arm::ARMDecodeContext(&cpu);
}
