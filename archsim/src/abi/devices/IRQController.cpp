/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * IRQController.cpp
 *
 *  Created on: 19 Dec 2013
 *      Author: harry
 */

#include "abi/devices/IRQController.h"
#include "core/thread/ThreadInstance.h"

namespace archsim
{
	namespace abi
	{
		namespace devices
		{

			IRQSource::~IRQSource() {}

			static ComponentDescriptor irq_line_descriptor ("IRQLine");
			IRQLine::IRQLine() :
				Component(irq_line_descriptor),
				Connected(false),
				line_(-1),
				asserted_(false),
				source(NULL)
			{

			}

			IRQLine::~IRQLine()
			{

			}

			bool IRQLine::Initialise()
			{
				return true;
			}

			void IRQLine::SetLine(uint32_t new_line)
			{
				assert(!Connected);
				line_ = new_line;
			}

			void IRQLine::SetSource(IRQSource *new_source)
			{
				source = new_source;
			}

			IRQControllerLine::IRQControllerLine(IRQController *controller) : Controller(controller)
			{

			}

			void IRQControllerLine::Assert()
			{
				if(!IsAsserted()) {
					SetAsserted();
					Controller->AssertLine(Line());
				}
			}

			void IRQControllerLine::Rescind()
			{
				if(IsAsserted()) {
					Controller->RescindLine(Line());
					ClearAsserted();
				}
			}


			CPUIRQLine::CPUIRQLine(archsim::core::thread::ThreadInstance *_cpu) : CPU(_cpu), Acknowledged(false)
			{

			}

			void CPUIRQLine::Assert()
			{
				if(!IsAsserted()) {
					SetAsserted();
					CPU->TakeIRQ();
//		source->IRQ_Asserted(this);
				}
			}

			void CPUIRQLine::Rescind()
			{
				if(IsAsserted()) {
					CPU->RescindIRQ();
					ClearAsserted();
					Acknowledged = false;
//		source->IRQ_Rescinded(this);
				}
			}

			IRQController::IRQController(uint32_t num_lines)
			{
				_lines.resize(num_lines, nullptr);
				for(uint32_t i = 0; i < num_lines; ++i) {
					_lines[i] = new IRQControllerLine(this);
					_lines[i]->SetLine(i);
				}
			}

			IRQController::~IRQController()
			{
				for(auto line : _lines) {
					delete line;
				}
			}

			IRQLine* IRQController::RegisterSource(uint32_t line)
			{
				if(line > _lines.size()) return NULL;

				_lines[line]->Connected = true;
				return _lines[line];
			}

			bool IRQController::DeregisterSource(uint32_t line)
			{
				if(_lines[line]->Connected) {
					_lines[line]->Connected = false;
					return true;
				}
				return false;
			}

			uint32_t IRQController::GetNumLines() const
			{
				return _lines.size();
			}



		}
	}
}
