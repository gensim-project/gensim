/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * TickConsumer.h
 *
 *  Created on: 19 Jan 2015
 *      Author: harry
 */

#ifndef TICKCONSUMER_H_
#define TICKCONSUMER_H_

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace timing
			{
				class TickConsumer
				{
				public:
					virtual ~TickConsumer();

					virtual void Tick(uint32_t tick_periods) = 0;
				};
			}
		}
	}
}
#endif /* TICKCONSUMER_H_ */
