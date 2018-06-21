/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * SerialPort.cpp
 *
 *  Created on: 4 Jul 2014
 *      Author: harry
 */

#include "abi/devices/SerialPort.h"

#include "util/LogContext.h"

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <stdio.h>

DeclareLogContext(LogSerial, "Serial");

using namespace archsim::abi::devices;

static archsim::abi::devices::ComponentDescriptor serialport_descriptor("SerialPort");
SerialPort::SerialPort() : Component(serialport_descriptor) {}

SerialPort::~SerialPort()
{
}

bool SerialPort::Initialise()
{
	return true;
}


bool ConsoleSerialPort::IsDataAvailable(int timeout)
{
	struct pollfd poll_data;
	poll_data.fd = filedes_in;
	poll_data.events = POLLIN | POLLPRI;


	int ret = poll(&poll_data, 1, timeout);
	if(ret < 0) {
		LC_ERROR(LogSerial) << "Error encountered when polling.";
		perror("Error encountered when polling");
		return false;
	}

	bool data_avail = (poll_data.revents & POLLIN) != 0;
	LC_DEBUG1(LogSerial) << "Console checking data availability and data " << (data_avail ? "available": "not available");

	return data_avail;
}

bool ConsoleSerialPort::ReadChar(char &data)
{
	return read(filedes_in, &data, 1) > 0;
}

bool ConsoleSerialPort::WriteChar(char data)
{
	return write(filedes_out, &data, 1) > 0;
}

bool ConsoleSerialPort::Open()
{
	filedes_in = fileno(stdin);
	filedes_out = fileno(stdout);

	if(archsim::options::SerialGrab) {
		struct termios settings;
		tcgetattr(fileno(stdout), &settings);
		orig_settings = settings;
		cfmakeraw(&settings);
		settings.c_cc[VTIME] = 0;
		settings.c_cc[VMIN] = 1;

		tcsetattr(0, TCSANOW, &settings);
	}

	return true;
}

bool ConsoleSerialPort::Close()
{
	if(archsim::options::SerialGrab) {
		tcsetattr(0, TCSANOW, &orig_settings);
	}
	return true;
}

bool ConsoleOutputSerialPort::IsDataAvailable(int timeout)
{
	usleep(timeout);
	return false;
}

bool ConsoleOutputSerialPort::ReadChar(char &data)
{
	return false;
}

bool NullSerialPort::IsDataAvailable(int timeout)
{
	usleep(timeout);
	return false;
}
bool NullSerialPort::ReadChar(char &data)
{
	data = 0;
	return true;
}
bool NullSerialPort::WriteChar(char data)
{
	return true;
}
bool NullSerialPort::Open()
{
	return true;
}
bool NullSerialPort::Close()
{
	return true;
}

bool InputFifoSerialPort::IsDataAvailable(int timeout)
{
	while(fifo.empty() && timeout > 0) {
		timeout -= 500;
		usleep(500);
	}

	if(fifo.empty()) return false;
	return true;
}

bool InputFifoSerialPort::ReadChar(char &data)
{
	fifo_lock.acquire();
	if(fifo.empty()) return false;
	data = fifo.front();
	fifo.pop_front();
	fifo_lock.release();
	return true;
}

bool InputFifoSerialPort::WriteChar(char data)
{
	return false;
}

bool InputFifoSerialPort::Open()
{
	fifo_lock.acquire();
	fifo.clear();
	fifo_lock.release();
	return true;
}

bool InputFifoSerialPort::Close()
{
	return true;
}

bool InputFifoSerialPort::InputChar(char data)
{
	fifo_lock.acquire();
	fifo.push_back(data);
	fifo_lock.release();
	return true;
}

AsyncSerial::AsyncSerial(SerialPort *port, on_data_avail_t callback, void* ctx) : Thread("Serial Port"), inner_port(port), terminate(false), avail_callback(callback), ctx(ctx)
{

}

bool AsyncSerial::IsDataAvailable(int timeout)
{
	return inner_port->IsDataAvailable(timeout);
}

bool AsyncSerial::ReadChar(char &data)
{
	return inner_port->ReadChar(data);
}
bool AsyncSerial::WriteChar(char data)
{
	return inner_port->WriteChar(data);
}

bool AsyncSerial::Open()
{
	if(!inner_port->Open()) return false;
	start();
	return true;
}

bool AsyncSerial::Close()
{
	terminate = true;
	while(terminate) ;
	return inner_port->Close();
}

void AsyncSerial::run()
{
	while(!terminate) {
		if(IsDataAvailable(1000)) avail_callback(this, ctx);
	}
	terminate = false;
}
