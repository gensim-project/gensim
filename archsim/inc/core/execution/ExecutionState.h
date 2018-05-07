/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ExecutionState.h
 * Author: harry
 *
 * Created on 07 May 2018, 12:15
 */

#ifndef EXECUTIONSTATE_H
#define EXECUTIONSTATE_H

namespace archsim {
	namespace core {
		namespace execution {
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

