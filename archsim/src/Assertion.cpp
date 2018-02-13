//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2012,
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#include <stdarg.h>

#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>

#include "Assertion.h"

namespace archsim
{

	void Assertion::Fail(const char *fmt, ...)
	{
		std::stringstream s;
		s << file_ << ":" << line_ << ": error: ";

		va_list args;
		va_start(args, fmt);

		char buf[BUFSIZ];
		vsnprintf(buf, sizeof(buf), fmt, args);

		va_end(args);
		s << buf << std::endl;

		std::string msg = s.str();
		std::fprintf(stderr, "%s", msg.c_str());  // print message to stderr
		std::fflush(stderr);

		// Semantics of Assert is to ABORT
		std::abort();
	}
}
