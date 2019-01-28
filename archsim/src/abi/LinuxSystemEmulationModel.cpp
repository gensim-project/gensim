/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/LinuxSystemEmulationModel.h"

#include "util/LogContext.h"

UseLogContext(LogSystemEmulationModel);
DeclareChildLogContext(LogLinuxSystemEmulationModel, LogSystemEmulationModel, "Linux");

using namespace archsim::abi;

LinuxSystemEmulationModel::LinuxSystemEmulationModel(bool is64bit) : SystemEmulationModel(is64bit)
{
	rootfs.valid = false;
	atags.valid = false;
	devicetree.valid = false;
}

LinuxSystemEmulationModel::~LinuxSystemEmulationModel()
{

}

bool LinuxSystemEmulationModel::Initialise(System& system, archsim::uarch::uArch& uarch)
{
	return SystemEmulationModel::Initialise(system, uarch);
}

void LinuxSystemEmulationModel::Destroy()
{
	SystemEmulationModel::Destroy();
}

bool LinuxSystemEmulationModel::InstallPlatform(loader::BinaryLoader& loader)
{
	if (archsim::options::RootFS.IsSpecified()) {
		// Load the root filesystem
		LC_DEBUG1(LogLinuxSystemEmulationModel) << "Installing root filesystem";
		if(!InstallRootFS(archsim::options::RootFS, Address(archsim::options::RootFSLocation))) {
			LC_ERROR(LogLinuxSystemEmulationModel) << "Unable to install root filesystem";
			return false;
		}
	}

	if (archsim::options::DeviceTreeFile.IsSpecified()) {
		// Load the device tree.
		LC_DEBUG1(LogLinuxSystemEmulationModel) << "Installing device tree";
		if (!InstallDeviceTree(archsim::options::DeviceTreeFile, Address(0x1000))) {
			LC_ERROR(LogLinuxSystemEmulationModel) << "Unable to install device tree.";
			return false;
		}
	} else {
		// Install ATAGS.
		LC_DEBUG1(LogLinuxSystemEmulationModel) << "Installing ATAGS";
		// earlyprintk=serial console=ttyAMA0 root=/dev/vda1 rw verbose", 0x100))
		if (!InstallATAGS(archsim::options::KernelArgs, Address(0x100))) {
			LC_ERROR(LogLinuxSystemEmulationModel) << "Unable to install ATAGS.";
			return false;
		}
	}

	return true;
}

bool LinuxSystemEmulationModel::InstallRootFS(std::string rootfs_file, Address location)
{
	uint32_t size = 0;
	bool rc = GetMemoryModel().InsertFile(location, rootfs_file, size);

	if (!rc) {
		LC_ERROR(LogLinuxSystemEmulationModel) << "Could not install root filesystem";
		return false;
	}

	rootfs.address = location.Get();
	rootfs.size = size;
	rootfs.valid = true;

	return true;
}

bool LinuxSystemEmulationModel::InstallDeviceTree(std::string device_tree_file, Address addr)
{
	uint32_t size = 0;
	bool rc = GetMemoryModel().InsertFile(addr, device_tree_file, size);

	if (!rc) {
		LC_ERROR(LogLinuxSystemEmulationModel) << "Could not install device tree";
		return false;
	}

	devicetree.address = addr.Get();
	devicetree.size = size;
	devicetree.valid = true;

	return true;
}

#define ATAG_CMDLINE 0x54410009

bool LinuxSystemEmulationModel::InstallATAGS(std::string kernel_args, Address base_address)
{
	auto addr = base_address;

	// CORE
	GetMemoryModel().Write32(addr, 5);
	addr += 4;
	GetMemoryModel().Write32(addr, 0x54410001);
	addr += 4;
	GetMemoryModel().Write32(addr, 0x1); //read only flag
	addr += 4;
	GetMemoryModel().Write32(addr, 4096); //page size?
	addr += 4;
	GetMemoryModel().Write32(addr, 0); //'root device number' (??? XXX what is this)
	addr += 4;

	// MEMORY
	GetMemoryModel().Write32(addr, 4);
	addr += 4;
	GetMemoryModel().Write32(addr, 0x54410002);
	addr += 4;
	GetMemoryModel().Write32(addr, 0x10000000);
	addr += 4;
	GetMemoryModel().Write32(addr, 0);
	addr += 4;

	// INITRD
	if (rootfs.valid) {
		//ATAG_INITRD2
		GetMemoryModel().Write32(addr, 4);
		addr += 4;
		GetMemoryModel().Write32(addr, 0x54420005);
		addr += 4;
		GetMemoryModel().Write32(addr, rootfs.address);
		addr += 4;
		GetMemoryModel().Write32(addr, rootfs.size);
		addr += 4;

		//ATAG_RAMDISK
		GetMemoryModel().Write32(addr, 5);
		addr += 4;
		GetMemoryModel().Write32(addr, 0x54410004);
		addr += 4;
		GetMemoryModel().Write32(addr, 0);
		addr += 4;
		GetMemoryModel().Write32(addr, 4096);
		addr += 4;
		GetMemoryModel().Write32(addr, 0);
		addr += 4;
	}

	// CMDLINE
	if (kernel_args.length()) {
		fprintf(stderr, "Installing kernel command-line to ATAGS: %s\n", kernel_args.c_str());
		const char *cmdline = kernel_args.c_str();
		GetMemoryModel().Write32(addr, 2 + (strlen(cmdline) + 3) / 4);
		addr += 4;
		GetMemoryModel().Write32(addr, 0x54410009);
		addr += 4;

		GetMemoryModel().WriteString(addr, cmdline);
		volatile uint32_t v = ((strlen(cmdline) + 3) / 4);
		addr += v * 4;
	}

	// NONE
	GetMemoryModel().Write32(addr, 0);
	addr += 4;
	GetMemoryModel().Write32(addr, 0);

	atags.address = base_address.Get();
	atags.size = (addr - base_address).Get();
	atags.valid = true;

	return true;
}
