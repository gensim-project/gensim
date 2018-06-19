/*
 * Address.h
 *
 *  Created on: 28 Sep 2015
 *      Author: harry
 */

#ifndef INC_ABI_ADDRESS_H_
#define INC_ABI_ADDRESS_H_

#include <cstdint>
#include <functional>
#include <iostream>
#include <iomanip>

#include "translate/profile/RegionArch.h"

namespace archsim
{
	using namespace translate::profile;
	class Address
	{
	public:
		static const Address NullPtr;
		
		using underlying_t = uint64_t;
		
		explicit Address(underlying_t address) : address_(address) {}
		Address() : address_(0) {};
		underlying_t Get() const
		{
			return address_;
		}

		underlying_t GetPageBase() const
		{
			return Get() & ~archsim::RegionArch::PageMask;
		}
		underlying_t GetPageOffset() const
		{
			return Get() & archsim::RegionArch::PageMask;
		}
		underlying_t GetPageIndex() const
		{
			return Get() >> archsim::RegionArch::PageBits;
		}

		Address PageBase() const
		{
			return Address(GetPageBase());
		}
		Address PageOffset() const
		{
			return Address(GetPageOffset());
		}

		bool operator>(const Address &other) const { return Get() > other.Get(); }
		bool operator>=(const Address &other) const { return Get() >= other.Get(); }
		bool operator<(const Address &other) const { return Get() < other.Get(); }
		bool operator<=(const Address &other) const { return Get() <= other.Get(); }
		
		void operator+=(const underlying_t other)
		{
			address_ += other;
		}
		void operator-=(const underlying_t other)
		{
			address_ += other;
		}
		void operator&=(const underlying_t other)
		{
			address_ &= other;
		}

		bool operator==(const Address &other) const
		{
			return Get() == other.Get();
		}
		bool operator!=(const Address &other) const
		{
			return Get() != other.Get();
		}

	private:
		underlying_t address_;
	} __attribute__((packed));

	class PhysicalAddress : public Address
	{
	public:
		explicit PhysicalAddress(underlying_t address) : Address(address) {}
		PhysicalAddress() = delete;

		PhysicalAddress operator+(const underlying_t other)
		{
			return PhysicalAddress(Get()+other);
		}
		PhysicalAddress operator-(const underlying_t other)
		{
			return PhysicalAddress(Get()-other);
		}

		PhysicalAddress PageBase() const
		{
			return PhysicalAddress(GetPageBase());
		}

		bool operator==(const PhysicalAddress &other) const
		{
			return Get() == other.Get();
		}
	};

	class VirtualAddress : public Address
	{
	public:
		explicit VirtualAddress(underlying_t address) : Address(address) {}
		VirtualAddress() = delete;

		VirtualAddress operator+(const underlying_t other)
		{
			return VirtualAddress(Get()+other);
		}
		VirtualAddress operator-(const underlying_t other)
		{
			return VirtualAddress(Get()-other);
		}

		bool operator>(const VirtualAddress other) const
		{
			return Get() > other.Get();
		}

		VirtualAddress PageBase() const
		{
			return VirtualAddress(GetPageBase());
		}

		bool operator==(const VirtualAddress &other) const
		{
			return Get() == other.Get();
		}
	};
}

namespace std
{
	template<> struct hash<archsim::Address> {
	public:
		size_t operator()(const archsim::Address &x) const
		{
			std::hash<uint32_t> uhash;
			return uhash(x.Get());
		}
	};
	template<> struct hash<archsim::VirtualAddress> {
	public:
		size_t operator()(const archsim::VirtualAddress &x) const
		{
			std::hash<uint32_t> uhash;
			return uhash(x.Get());
		}
	};
	template<> struct hash<archsim::PhysicalAddress> {
	public:
		size_t operator()(const archsim::PhysicalAddress &x) const
		{
			std::hash<uint32_t> uhash;
			return uhash(x.Get());
		}
	};

}

template<typename T> T &operator<<(T& str, const archsim::Address &address)
{
	str << "0x" << std::hex << std::setw(8) << std::setfill('0') << address.Get();
	return str;
}

inline bool operator>=(archsim::Address::underlying_t a, archsim::Address b) {
	return a >= b.Get();
}

inline archsim::Address operator+(archsim::Address a, archsim::Address b)
{
	return archsim::Address(a.Get() + b.Get());
}
inline archsim::Address operator+(archsim::Address a, archsim::Address::underlying_t b)
{
	return archsim::Address(a.Get() + b);
}
inline archsim::Address operator+(archsim::Address::underlying_t a, archsim::Address b)
{
	return archsim::Address(a + b.Get());
}

inline archsim::Address operator-(archsim::Address a, archsim::Address b)
{
	return archsim::Address(a.Get() - b.Get());
}
inline archsim::Address operator-(archsim::Address a, archsim::Address::underlying_t b)
{
	return archsim::Address(a.Get() - b);
}
inline archsim::Address operator-(archsim::Address::underlying_t a, archsim::Address b)
{
	return archsim::Address(a - b.Get());
}

inline archsim::Address operator&(archsim::Address a, archsim::Address::underlying_t b)
{
	return archsim::Address(a.Get() & b);
}
inline archsim::Address operator|(archsim::Address a, archsim::Address::underlying_t b)
{
	return archsim::Address(a.Get() | b);
}
inline archsim::Address operator|(archsim::Address a, archsim::Address b)
{
	return archsim::Address(a.Get() | b.Get());
}

inline archsim::Address operator "" _ga(unsigned long long a) 
{
	return archsim::Address(a);
}

#endif /* INC_ADDRESS_H_ */
