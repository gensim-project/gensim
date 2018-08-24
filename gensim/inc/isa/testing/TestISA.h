/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

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

