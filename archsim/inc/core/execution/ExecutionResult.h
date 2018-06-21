/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


/*
 * File:   ExecutionResult.h
 * Author: harry
 *
 * Created on 26 April 2018, 11:48
 */

#ifndef EXECUTIONRESULT_H
#define EXECUTIONRESULT_H

namespace archsim
{
	namespace core
	{
		namespace execution
		{
			enum class ExecutionResult {
				Continue,
				Exception,
				Abort,
				Halt
			};
		}
	}
}

#endif /* EXECUTIONRESULT_H */

