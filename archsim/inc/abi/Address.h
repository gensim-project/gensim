/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

#include "util/LogContext.h"

namespace archsim
{
	using namespace translate::profile;
	class Address
	{
	public:
		using underlying_t = uint32_t;

		explicit Address(underlying_t address) : _address(address) {}
		Address() = delete;
		underlying_t Get() const
		{
			return _address;
		}

		underlying_t GetPageBase() const
		{
			return RegionArch::PageBaseOf(Get());
		}
		underlying_t GetPageOffset() const
		{
			return RegionArch::PageOffsetOf(Get());
		}
		underlying_t GetPageIndex() const
		{
			return RegionArch::PageIndexOf(Get());
		}

		Address PageBase() const
		{
			return Address(GetPageBase());
		}
		Address PageOffset() const
		{
			return Address(GetPageOffset());
		}

		Address operator+(int b) const
		{
			return Address(Get() + b);
		}

		void operator+=(const underlying_t other)
		{
			_address += other;
		}
		void operator-=(const underlying_t other)
		{
			_address += other;
		}

		bool operator==(const Address &other) const
		{
			return Get() == other.Get();
		}

		friend std::ostream &operator<<(std::ostream &str, const archsim::Address& address);

	private:
		underlying_t _address;
	};

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

#endif /* INC_ABI_ADDRESS_H_ */
