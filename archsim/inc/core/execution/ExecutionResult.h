/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ExecutionResult.h
 * Author: harry
 *
 * Created on 26 April 2018, 11:48
 */

#ifndef EXECUTIONRESULT_H
#define EXECUTIONRESULT_H

namespace archsim {
	namespace core {
		namespace execution {
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

