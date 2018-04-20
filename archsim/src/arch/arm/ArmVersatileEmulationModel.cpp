/*
 * arch/arm/ArmSystemEmulationModel.cpp
 */
#include "arch/arm/ArmVersatileEmulationModel.h"
#include "arch/arm/ARMDecodeContext.h"

#include "abi/loader/BinaryLoader.h"
#include "abi/devices/SerialPort.h"
#include "abi/devices/generic/ps2/PS2Device.h"
#include "abi/devices/generic/block/FileBackedBlockDevice.h"
#include "abi/devices/virtio/VirtIOBlock.h"
#include "abi/devices/virtio/VirtIONet.h"
#include "abi/devices/arm/special/SimulatorCacheControlCoprocessor.h"
#include "abi/devices/gfx/VirtualScreenManager.h"
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

RegisterComponent(EmulationModel, ArmVersatileEmulationModel, "arm-versatile", "ARM system emulation model")

UseLogContext(LogSystemEmulationModel);
DeclareChildLogContext(LogArmVerstaileEmulationModel, LogSystemEmulationModel, "Versatile");

ArmVersatileEmulationModel::ArmVersatileEmulationModel() : entry_point(0)
{

}

ArmVersatileEmulationModel::~ArmVersatileEmulationModel()
{
}

bool ArmVersatileEmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	if (!SystemEmulationModel::Initialise(system, uarch))
		return false;

	// Initialise "physical memory".
	if (GetMemoryModel().GetMappingManager())
		GetMemoryModel().GetMappingManager()->MapAll((archsim::abi::memory::RegionFlags)7);

	return true;
}

void ArmVersatileEmulationModel::Destroy()
{
	SystemEmulationModel::Destroy();
}

bool ArmVersatileEmulationModel::InstallBootloader(archsim::abi::loader::BinaryLoader& loader)
{
	entry_point = loader.GetEntryPoint();
	GetMemoryModel().Write(0, (uint8_t *)&bootloader_start, bootloader_size);

	return true;
}

bool ArmVersatileEmulationModel::InstallPlatform(loader::BinaryLoader& loader)
{
	if (!LinuxSystemEmulationModel::InstallPlatform(loader))
		return false;

	return InstallBootloader(loader);
}

bool ArmVersatileEmulationModel::PrepareCore(archsim::ThreadInstance& core)
{
	uint32_t *regs = (uint32_t *)core.GetRegisterFileInterface().GetEntry<uint32_t>("RB");

	// r0 = 0
	regs[0] = 0;

	// r1 = CPU Type
	regs[1] = 0x183;

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

bool ArmVersatileEmulationModel::HackyMMIORegisterDevice(abi::devices::MemoryComponent& device)
{
#define IO_ADDRESS(x) (((x) & 0x0fffffff) + (((x) >> 4) & 0x0f000000) + 0xf0000000)
	return RegisterMemoryComponent(device);
}

bool ArmVersatileEmulationModel::AddGenericPrimecellDevice(Address base_addr, uint32_t size, uint32_t peripheral_id)
{
	assert(size == 0x1000);
	auto device = GetSystem().GetModuleManager().GetModuleEntry<module::ModuleDeviceEntry>("devices.arm.system.GenericPrimecellDevice")->Get(*this, base_addr);
	device->SetParameter<uint64_t>("PeripheralID", peripheral_id);
	HackyMMIORegisterDevice(*device);
	return true;
}

bool ArmVersatileEmulationModel::InstallPlatformDevices()
{
	using module::ModuleDeviceEntry;

	LC_INFO(LogArmVerstaileEmulationModel) << "Installing platform devices";

	auto &mm = GetSystem().GetModuleManager();
	auto adm = mm.GetModule("devices.arm.system");

	archsim::abi::devices::gfx::VirtualScreenManagerBase *screen_man = NULL;
	if(!GetComponentInstance(archsim::options::ScreenManagerType, screen_man)) {
		fprintf(stderr, "Could not instantiate Screen Manager %s!\n%s\n", archsim::options::ScreenManagerType.GetValue().c_str(), GetRegisteredComponents(screen_man).c_str());;
		return false;
	}

	auto screen = screen_man->CreateScreenInstance("lcd", &GetMemoryModel(), &GetSystem());

	screen->SetKeyboard(GetSystem().GetSession().global_kbd);
	screen->SetMouse(GetSystem().GetSession().global_mouse);

	//SP810 System controller - registered at multiple places in the device space
	auto sysctl = adm->GetEntry<ModuleDeviceEntry>("SP810")->Get(*this, Address(0x10000000));
	base_device_manager.InstallDevice((0x101e0000), (guest_size_t)0x1000, *sysctl);
	base_device_manager.InstallDevice((0xf11e0000), (guest_size_t)0x1000, *sysctl);
	if(!HackyMMIORegisterDevice(*sysctl)) return false;

	auto pl190 = adm->GetEntry<ModuleDeviceEntry>("PL190")->Get(*this, Address(0x1014000));
	auto irq_controller = dynamic_cast<IRQController*>(pl190->GetParameter<Component*>("IRQController"));
	pl190->SetParameter("IRQLine", GetBootCore()->GetIRQLine(0));
	pl190->SetParameter("FIQLine", GetBootCore()->GetIRQLine(1));
	pl190->Initialise();

	if(!HackyMMIORegisterDevice(*pl190)) return false;

	auto timer0 = adm->GetEntry<ModuleDeviceEntry>("SP804")->Get(*this, Address(0x101e2000));
	timer0->SetParameter("IRQLine", irq_controller->RegisterSource(4));
	if(!HackyMMIORegisterDevice(*timer0)) return false;

	auto timer1 = adm->GetEntry<ModuleDeviceEntry>("SP804")->Get(*this, Address(0x101e3000));
	timer1->SetParameter("IRQLine", irq_controller->RegisterSource(5));
	if(!HackyMMIORegisterDevice(*timer1)) return false;

	auto sic = adm->GetEntry<ModuleDeviceEntry>("VersatileSIC")->Get(*this, Address(0x10003000));
	auto sic_controller = dynamic_cast<IRQController*>(sic);
	sic->SetParameter("IRQLine", irq_controller->RegisterSource(31));
	if(!HackyMMIORegisterDevice(*sic)) return false;

	auto pl080 = adm->GetEntry<ModuleDeviceEntry>("PL080")->Get(*this, Address(0x10130000));
	if(!HackyMMIORegisterDevice(*pl080)) return false;

	auto pl011 = adm->GetEntry<ModuleDeviceEntry>("PL011")->Get(*this, Address(0x101f1000));
	pl011->SetParameter("IRQLine", irq_controller->RegisterSource(12));

	if(archsim::options::Verify) {
		pl011->SetParameter("SerialPort", new abi::devices::ConsoleOutputSerialPort());
	} else {
		pl011->SetParameter("SerialPort", new abi::devices::ConsoleSerialPort());
	}
	if(!HackyMMIORegisterDevice(*pl011)) return false;

	pl011 = adm->GetEntry<ModuleDeviceEntry>("PL011")->Get(*this, Address(0x101f2000));
	pl011->SetParameter("IRQLine", irq_controller->RegisterSource(13));
	if(!HackyMMIORegisterDevice(*pl011)) return false;

	pl011 = adm->GetEntry<ModuleDeviceEntry>("PL011")->Get(*this, Address(0x101f3000));
	pl011->SetParameter("IRQLine", irq_controller->RegisterSource(14));
	if(!HackyMMIORegisterDevice(*pl011)) return false;

	//Advanced audio codec interface
	AddGenericPrimecellDevice(Address(0x10004000), 0x1000, 0xf0f0f0f0); //TODO: fill in Peripheral ID

	// Keyboard/Mouse Interface 0 (KB)
	devices::generic::ps2::PS2KeyboardDevice *ps2kbd = new devices::generic::ps2::PS2KeyboardDevice(*sic_controller->RegisterSource(3));
	GetSystem().GetSession().global_kbd.AddKeyboard(ps2kbd);

	auto pl050kb = adm->GetEntry<ModuleDeviceEntry>("PL050")->Get(*this, Address(0x10006000));
	if(!HackyMMIORegisterDevice(*pl050kb)) return false;

	// Keyboard/Mouse Interface 0 (Mouse)
	devices::generic::ps2::PS2MouseDevice *ps2ms = new devices::generic::ps2::PS2MouseDevice(*sic_controller->RegisterSource(4));
	GetSystem().GetSession().global_mouse.AddMouse(ps2ms);

	auto pl050ms = adm->GetEntry<ModuleDeviceEntry>("PL050")->Get(*this, Address(0x10007000));
	if(!HackyMMIORegisterDevice(*pl050ms)) return false;

	//Static memory controller
	//AddGenericPrimecellDevice(Address(0x10100000), 0x10000, 0x93101400);

	//Multi-port memory controller
	//AddGenericPrimecellDevice(Address(0x10110000), 0x10000, 0x75110447);

	//Color (sic) LCD Controller
	auto pl110 = adm->GetEntry<ModuleDeviceEntry>("PL110")->Get(*this, Address(0x10120000));
	pl110->SetParameter("Screen", screen);
	if(!HackyMMIORegisterDevice(*pl110)) return false;

	//Watchdog interface
	AddGenericPrimecellDevice(Address(0x101e1000), 0x1000, 0x05181400);

	//GPIOs
//	devices::PL061 *pl061a = new devices::PL061(*this, *vic->GetIRQController().RegisterSource(6), Address(0x101E4000));
//	devices::PL061 *pl061b = new devices::PL061(*this, *vic->GetIRQController().RegisterSource(7), Address(0x101E5000));
//	devices::PL061 *pl061c = new devices::PL061(*this, *vic->GetIRQController().RegisterSource(8), Address(0x101E6000));
//	devices::PL061 *pl061d = new devices::PL061(*this, *vic->GetIRQController().RegisterSource(9), Address(0x101E7000));

//	if (!HackyMMIORegisterDevice(*pl061a)) return false;
//	if (!HackyMMIORegisterDevice(*pl061b)) return false;
//	if (!HackyMMIORegisterDevice(*pl061c)) return false;
//	if (!HackyMMIORegisterDevice(*pl061d)) return false;

	// Multimedia Card Interface 0
//	devices::PL180 *pl180 = new devices::PL180(*this,
//	        *vic->GetIRQController().RegisterSource(22),
//	        *sic->GetIRQController().RegisterSource(22),
//	        *sic->GetIRQController().RegisterSource(2),
//	        Address(0x10005000));
//	if(!HackyMMIORegisterDevice(*pl180)) return false;

	//RTC
	auto pl031 = adm->GetEntry<ModuleDeviceEntry>("PL031")->Get(*this, Address(0x101E8000));
	if (!HackyMMIORegisterDevice(*pl031)) return false;

	//Smart card interface
	AddGenericPrimecellDevice(Address(0x101F0000), 0x1000, 0x31110400);

	//SSP
	AddGenericPrimecellDevice(Address(0x101F4000), 0x1000, 0x22102400);

	// Ethernet Device
	//AddGenericPrimecellDevice(Address(0x10010000), 0x10000, 0xf0f0f0f0);

	// VirtIO Block Device

	devices::virtio::VirtIOBlock *vbda = new devices::virtio::VirtIOBlock(*this, *irq_controller->RegisterSource(30), Address(0x11001000), "virtio-block-a", *GetSystem().GetBlockDevice("vda"));
	if (!HackyMMIORegisterDevice(*vbda)) return false;


	// Block Device (B)

	/*devices::generic::block::FileBackedBlockDevice *bdevb = new devices::generic::block::FileBackedBlockDevice();
	if (!bdevb->Open("/tmp/swap")) {
		LC_ERROR(LogArmVerstaileEmulationModel) << "Unable to open block device file: " << archsim::options::BlockDeviceFile.GetValue();
		return false;
	}

	devices::virtio::VirtIOBlock *vbdb = new devices::virtio::VirtIOBlock(*this, *vic->GetIRQController().RegisterSource(29), 0x11002000, "virtio-block-b", *bdevb);
	if (!HackyMMIORegisterDevice(*vbdb)) return false;*/

	/*devices::virtio::VirtIONet *vnd = new devices::virtio::VirtIONet(*this, *vic->GetIRQController().RegisterSource(29), 0x11002000, "virtio-net");
	if (!HackyMMIORegisterDevice(*vnd)) return false;*/

	return true;
}

bool ArmVersatileEmulationModel::InstallPeripheralDevices()
{
	LC_INFO(LogArmVerstaileEmulationModel) << "[ARM-SYSTEM] Installing peripheral devices";

	archsim::abi::devices::Device *coprocessor;
	if(!GetComponentInstance("arm926coprocessor", coprocessor)) return false;
	main_thread_->GetPeripherals().RegisterDevice("coprocessor", coprocessor);
	main_thread_->GetPeripherals().AttachDevice("coprocessor", 15);

	if(!GetComponentInstance("armdebug", coprocessor)) return false;
	main_thread_->GetPeripherals().RegisterDevice("armdebug", coprocessor);
	main_thread_->GetPeripherals().AttachDevice("armdebug", 14);

	archsim::abi::devices::Device *mmu;
	if(!GetComponentInstance("ARM926EJSMMU", mmu)) return false;
	main_thread_->GetPeripherals().RegisterDevice("mmu", mmu);

	devices::SimulatorCacheControlCoprocessor *sccc = new devices::SimulatorCacheControlCoprocessor();
	main_thread_->GetPeripherals().RegisterDevice("sccc", sccc);
	main_thread_->GetPeripherals().AttachDevice("sccc", 13);

	main_thread_->GetPeripherals().InitialiseDevices();

	return true;
}

bool ArmVersatileEmulationModel::InstallDevices()
{
	return InstallPeripheralDevices() && InstallPlatformDevices();
}

void ArmVersatileEmulationModel::DestroyDevices()
{
//	archsim::abi::devices::ArmCoprocessor *coproc = (archsim::abi::devices::ArmCoprocessor *)cpu->peripherals.GetDeviceByName("coprocessor");
//	fprintf(stderr, "Reads:\n");
//	coproc->dump_reads();
//
//	fprintf(stderr, "Writes:\n");
//	coproc->dump_writes();
}

void ArmVersatileEmulationModel::HandleSemihostingCall()
{
	UNIMPLEMENTED;
	uint32_t *regs = nullptr; //(uint32_t *)cpu->GetRegisterBankDescriptor("RB").GetBankDataStart();

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
			LC_WARNING(LogArmVerstaileEmulationModel) << "Unhandled semihosting API call " << regs[0];
			break;
	}
}

ExceptionAction ArmVersatileEmulationModel::HandleException(archsim::ThreadInstance *cpu, unsigned int category, unsigned int data)
{
	UNIMPLEMENTED;
//	
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

gensim::DecodeContext* ArmVersatileEmulationModel::GetNewDecodeContext(gensim::Processor& cpu)
{
	return new archsim::arch::arm::ARMDecodeContext(&cpu);
}
