/*
 * File:   GPIO.h
 * Author: s0457958
 *
 * Created on 16 September 2014, 11:41
 */

#ifndef GPIO_H
#define	GPIO_H

namespace archsim
{
	namespace abi
	{
		namespace devices
		{
			namespace generic
			{
				class GPIO
				{
				public:
					inline void Activate()
					{
						energised = true;
					}
					inline void Deactivate()
					{
						energised = false;
					}
					inline bool Activated() const
					{
						return energised;
					}
					inline bool Deactivated() const
					{
						return !energised;
					}

				private:
					bool energised;
				};
			}
		}
	}
}

#endif	/* GPIO_H */

