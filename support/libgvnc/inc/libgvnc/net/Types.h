/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#pragma once

namespace libgvnc
{
	namespace net
	{
		enum class AddressFamily {
			UNSPEC = 0,
			LOCAL = 1,
			UNIX = 1,
			FILE = 1,
			INET = 2,
			AX25 = 3,
			IPX = 4,
			INET6 = 10
		};

		enum class SocketType {
			STREAM = 1,
			DATAGRAM = 2,
			RAW = 3,
			RDM = 4,
			SEQPACKET = 5,
			DCCP = 6,
			PACKET = 10
		};

		enum class ProtocolType {
			NONE = 0
		};
	}
}
