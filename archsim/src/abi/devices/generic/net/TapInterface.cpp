/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include "abi/devices/generic/net/TapInterface.h"
#include "util/LogContext.h"

#include <cstring>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <pthread.h>

DeclareLogContext(LogTapInterface, "TAP");

using namespace archsim::abi::devices::generic::net;

TapInterface::TapInterface(const std::string &tap_device) : _tap_device(tap_device), _terminate(false)
{

}

TapInterface::~TapInterface()
{

}

bool TapInterface::transmit_packet(const uint8_t* buffer, uint32_t length)
{
	LC_DEBUG1(LogTapInterface) << "Transmitting Packet";

	write(_tap_fd, buffer, length);
	return true;
}

bool TapInterface::start()
{
	_tap_fd = open("/dev/net/tun", O_RDWR);
	if (_tap_fd < 0) {
		LC_ERROR(LogTapInterface) << "Unable to open tunnel control: ";
		return false;
	}

	struct ifreq ifr;
	bzero(&ifr, sizeof(ifr));

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	strncpy(ifr.ifr_name, _tap_device.c_str(), IFNAMSIZ);

	LC_DEBUG1(LogTapInterface) << "Attaching to: " << _tap_device;
	int rc = ioctl(_tap_fd, TUNSETIFF, (void *)&ifr);
	if (rc < 0) {
		close(_tap_fd);

		LC_ERROR(LogTapInterface) << "Unable to create TAP device: ";
		return false;
	}

	_terminate = false;
	_receive_thread = new std::thread(receive_thread_trampoline, this);

	return true;
}

void TapInterface::stop()
{
	LC_DEBUG1(LogTapInterface) << "Stopping TAP Interface...";

	_terminate = true;
	close(_tap_fd);
}

void TapInterface::receive_thread_trampoline(TapInterface* obj)
{
	pthread_setname_np(pthread_self(), "net-receive");
	obj->receive_thread_proc();
}

#define MTU	1600
void TapInterface::receive_thread_proc()
{
	LC_DEBUG1(LogTapInterface) << "Starting TAP receive thread...";

	while (!_terminate) {
		uint8_t buffer[MTU];

		LC_DEBUG1(LogTapInterface) << "Waiting for packet";
		int bytes = read(_tap_fd, buffer, sizeof(buffer));

		if (bytes <= 0) break;

		LC_DEBUG1(LogTapInterface) << "Receiving Packet";
		invoke_receive((const uint8_t *)buffer, bytes);
	}

	LC_DEBUG1(LogTapInterface) << "Exiting TAP receive thread";
}
