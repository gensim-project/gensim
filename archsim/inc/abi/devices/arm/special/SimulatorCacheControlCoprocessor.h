/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   CacheControlCoprocessor.h
 * Author: s0457958
 *
 * Created on 29 September 2014, 11:34
 */

#ifndef CACHECONTROLCOPROCESSOR_H
#define	CACHECONTROLCOPROCESSOR_H

#include "abi/devices/arm/core/ArmCoprocessor.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			class SimulatorCacheControlCoprocessor : public ArmCoprocessor
			{
			public:
				SimulatorCacheControlCoprocessor();

				virtual bool Initialise() override;

				// General purpose controller
				bool access_cp0(bool is_read, uint32_t& data) override;

				// Tracing subsystem controller
				bool access_cp1(bool is_read, uint32_t& data) override;
				bool access_cp2(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp3(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp4(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp5(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp6(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp7(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp8(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp9(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp10(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp11(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp12(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp13(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp14(bool is_read, uint32_t& data) override
				{
					return true;
				}
				bool access_cp15(bool is_read, uint32_t& data) override
				{
					return true;
				}

			private:
				uint32_t scptext_char;
			};
		}
	}
}

#endif	/* CACHECONTROLCOPROCESSOR_H */

