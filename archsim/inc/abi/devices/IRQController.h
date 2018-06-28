/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * IRQController.h
 *
 *  Created on: 19 Dec 2013
 *      Author: harry
 */

#ifndef IRQCONTROLLER_H_
#define IRQCONTROLLER_H_

#include "abi/devices/Component.h"

#include <cassert>
#include <cstdint>
#include <vector>
#include <mutex>

namespace archsim
{
	namespace core
	{
		namespace thread
		{
			class ThreadInstance;
		}
	}

	namespace abi
	{
		namespace devices
		{

			class IRQController;
			class IRQLine;

			class IRQSource
			{
			public:
				virtual ~IRQSource();
				virtual void IRQ_Asserted(IRQLine *line) = 0;
				virtual void IRQ_Rescinded(IRQLine *line) = 0;
			};

			class IRQLine : public Component
			{
			public:
				IRQLine();
				virtual ~IRQLine();

				virtual void Assert() = 0;
				virtual void Rescind() = 0;

				bool IsAsserted() const
				{
					return asserted_;
				}

				void SetLine(uint32_t new_line);
				uint32_t Line() const
				{
					return line_;
				}

				inline IRQSource *GetSource()
				{
					return source;
				}
				void SetSource(IRQSource *new_source);

				bool Initialise() override;


				bool Connected;

			protected:
				void SetAsserted()
				{
					asserted_ = true;
				}
				void ClearAsserted()
				{
					asserted_ = false;
				}
			private:
				uint32_t line_;
				bool asserted_;
				IRQSource *source;
			};

			class IRQControllerLine : public IRQLine
			{
			public:
				IRQControllerLine(IRQController *controller);

				virtual void Assert();
				virtual void Rescind();

			protected:
				IRQController *Controller;
			};

			class CPUIRQLine : public IRQLine
			{
			public:
				CPUIRQLine(archsim::core::thread::ThreadInstance *cpu);

				void Assert();
				void Rescind();

				inline void Acknowledge()
				{
					fflush(stderr);
					assert(IsAsserted());
					Acknowledged = true;
				}
				inline void Raise()
				{
					if(IsAsserted()) Acknowledged = false;
				}
				inline bool IsAcknowledged() const
				{
					return Acknowledged;
				}

			protected:
				archsim::core::thread::ThreadInstance *CPU;
				bool Acknowledged;
			};

			class IRQController : public virtual Component  //: public IRQSource
			{
			public:
				IRQController(uint32_t num_lines);
				virtual ~IRQController();

				IRQLine* RegisterSource(uint32_t line);
				bool DeregisterSource(uint32_t line);
				uint32_t GetNumLines() const;

				virtual bool AssertLine(uint32_t line) = 0;
				virtual bool RescindLine(uint32_t line) = 0;

			private:
				std::vector<IRQControllerLine*> _lines;
			};

		}
	}
}



#endif /* IRQCONTROLLER_H_ */
