/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * File:   MemoryEvent.h
 * Author: s0457958
 *
 * Created on 26 August 2014, 10:09
 */

#ifndef MEMORYEVENT_H
#define	MEMORYEVENT_H

#include "abi/memory/MemoryModel.h"
#include "util/ComponentManager.h"
#include "core/thread/ThreadInstance.h"

namespace archsim
{
	namespace abi
	{
		namespace memory
		{
			class MemoryEventHandlerTranslator;

			class MemoryEventHandler
			{
			public:
				virtual ~MemoryEventHandler() = 0;
				virtual bool HandleEvent(archsim::core::thread::ThreadInstance *cpu, MemoryModel::MemoryEventType type, Address addr, uint8_t size) = 0;
				virtual MemoryEventHandlerTranslator& GetTranslator() = 0;
			};
		}
	}
}


#endif	/* MEMORYEVENT_H */

