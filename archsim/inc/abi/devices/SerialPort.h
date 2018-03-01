/*
 * SerialPort.h
 *
 *  Created on: 4 Jul 2014
 *      Author: harry
 */

#ifndef SERIALPORT_H_
#define SERIALPORT_H_

#include "abi/devices/Component.h"
#include "concurrent/Mutex.h"
#include "concurrent/Thread.h"

#include <deque>

#include <termios.h>

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			class SerialPort : public Component
			{
			public:
				SerialPort();
				virtual ~SerialPort();
				virtual bool IsDataAvailable(int timeout) = 0;

				virtual bool ReadChar(char &data) = 0;
				virtual bool WriteChar(char data) = 0;

				virtual bool Open() = 0;
				virtual bool Close() = 0;

				bool Initialise() override;
			};

			class NullSerialPort : public SerialPort
			{
			public:
				bool IsDataAvailable(int timeout) override;

				bool ReadChar(char &data) override;
				bool WriteChar(char data) override;

				bool Open() override;
				bool Close() override;
			};

			class ConsoleSerialPort : public SerialPort
			{
			public:
				virtual bool IsDataAvailable(int timeout) override;

				virtual bool ReadChar(char &data) override;
				virtual bool WriteChar(char data) override;

				virtual bool Open() override;
				virtual bool Close() override;

			private:
				struct termios orig_settings;
				int filedes_in, filedes_out;
			};

			class ConsoleOutputSerialPort : public ConsoleSerialPort
			{
			public:
				bool IsDataAvailable(int timeout) override;
				bool ReadChar(char &data) override;
			};

			class InputFifoSerialPort : public SerialPort
			{
			public:
				bool IsDataAvailable(int timeout) override;

				bool ReadChar(char &data) override;
				bool WriteChar(char data) override;

				bool Open() override;
				bool Close() override;

				bool InputChar(char data);
			private:
				std::deque<char> fifo;
				concurrent::Mutex fifo_lock;
			};

			class AsyncSerial : public SerialPort, private archsim::concurrent::Thread
			{
			public:
				typedef void (*on_data_avail_t)(AsyncSerial *port, void*);

				AsyncSerial(SerialPort *port, on_data_avail_t callback, void* ctx);

				bool IsDataAvailable(int timeout) override;

				bool ReadChar(char &data) override;
				bool WriteChar(char data) override;

				bool Open() override;
				bool Close() override;

			private:
				void run() override;

				volatile bool terminate;

				SerialPort *inner_port;
				on_data_avail_t avail_callback;
				void *ctx;
			};

		}
	}
}


#endif /* SERIALPORT_H_ */
