/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "abi/devices/virtio/VirtIONet.h"
#include "util/LogContext.h"
#include "abi/devices/virtio/VirtQueue.h"

#include <string.h>
#include <mutex>

UseLogContext(LogVirtIO);
DeclareChildLogContext(LogNetwork, LogVirtIO, "Network");

using namespace archsim::abi::devices;
using namespace archsim::abi::devices::virtio;
using namespace archsim::abi::devices::generic::net;

static ComponentDescriptor virtionet_descriptor ("VirtioNet");
VirtIONet::VirtIONet(EmulationModel& parent_model, IRQLine& irq, Address base_address, const std::string &name, NetworkInterface &iface, uint64_t mac_address)
	: 
		VirtIO(parent_model, irq, base_address, 0x1000, name, 1, 1, 2),
		Component(virtionet_descriptor),
		_iface(iface)
	  
{
	bzero(&config, sizeof(config));

	config.mac[0] = (mac_address >> 40) & 0xff;
	config.mac[1] = (mac_address >> 32) & 0xff;
	config.mac[2] = (mac_address >> 24) & 0xff;
	config.mac[3] = (mac_address >> 16) & 0xff;
	config.mac[4] = (mac_address >>  8) & 0xff;
	config.mac[5] = (mac_address >>  0) & 0xff;

	config.status = 1;

	this->HostFeatures.Set((1 << 5) | (1 << 16));

	iface.attach([this](const uint8_t *buffer, uint32_t length){this->receive_packet(buffer, length);});
}

VirtIONet::~VirtIONet()
{

}

void VirtIONet::ResetDevice()
{

}

void VirtIONet::ProcessEvent(VirtIOQueueEvent* evt)
{
	if (evt->owner.Index() == 0) {
		std::unique_lock<std::mutex> l(_receive_buffer_lock);
		_receive_buffers.push_back(evt);
	} else if (evt->owner.Index() == 1) {
		VirtIOQueueEventBuffer& buffer = evt->read_buffers.back();
		_iface.transmit_packet((const uint8_t *)buffer.data, buffer.size);

		evt->owner.Push(evt->Index(), evt->response_size);
		AssertInterrupt(1);

		delete evt;
	}
}

void VirtIONet::receive_packet(const uint8_t* buffer, uint32_t length)
{
	_receive_buffer_lock.lock();

	if (_receive_buffers.size() == 0) {
		_receive_buffer_lock.unlock();
		return;
	}

	VirtIOQueueEvent *evt = _receive_buffers.front();
	_receive_buffers.pop_front();

	_receive_buffer_lock.unlock();

	if (evt->write_buffers.size() != 2) {
		return;
	}

	VirtIOQueueEventBuffer& hdr_buffer = evt->write_buffers.front();

	virtio_net_hdr *net_hdr = (virtio_net_hdr *)hdr_buffer.data;
	net_hdr->num_buffers = 2;

	VirtIOQueueEventBuffer& io_buffer = evt->write_buffers.back();

	unsigned int used_length = (length > io_buffer.size) ? io_buffer.size : length;
	memcpy(io_buffer.data, buffer, used_length);

	evt->response_size = hdr_buffer.size + used_length;

	evt->owner.Push(evt->Index(), evt->response_size);
	AssertInterrupt(1);

	delete evt;
}
