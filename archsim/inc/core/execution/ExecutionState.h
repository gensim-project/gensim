/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   ExecutionState.h
 * Author: harry
 *
 * Created on 07 May 2018, 12:15
 */

#ifndef EXECUTIONSTATE_H
#define EXECUTIONSTATE_H

namespace archsim
{
	namespace core
	{
		namespace execution
		{
			enum class ExecutionState {
				Unknown,
				Ready,
				Running,
				Halting,
				Halted,
				Suspending,
				Aborted
			};
		}
	}
}

#endif /* EXECUTIONSTATE_H */

