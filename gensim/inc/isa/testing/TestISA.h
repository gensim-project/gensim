/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   TestISA.h
 * Author: harry
 *
 * Created on 16 August 2017, 16:21
 */

#ifndef TESTISA_H
#define TESTISA_H

namespace gensim
{
	class DiagnosticContext;
	namespace isa
	{
		class ISADescription;
		namespace testing
		{
			ISADescription *GetTestISA(bool include_instruction);
		}
	}
}

#endif /* TESTISA_H */

