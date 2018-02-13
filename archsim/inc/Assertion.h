//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2012,
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Portable assertion implementation including convenience MACROS.
//
// =============================================================================

#ifndef INC_ASSERTION_H_
#define INC_ASSERTION_H_

namespace archsim
{

	class Assertion
	{
	public:
		Assertion(const char* file, int line) : file_(file), line_(line)
		{
			/* EMPTY */
		}

		void Fail(char const* fmt, ...);

	private:
		char const* const file_;
		const int line_;

		Assertion(const Assertion& m);     // DO NOT COPY
		void operator=(const Assertion&);  // DO NOT ASSIGN
	};
}

// -----------------------------------------------------------------------------
// Convenience MACROS that should be used to mark fatal, unimplemented and
// unreachable locations in code. Please use them wherever it makes sense!
//
#define FATAL(err) archsim::Assertion(__FILE__, __LINE__).Fail("%s", err)

#define FATAL1(fmt, p1) archsim::Assertion(__FILE__, __LINE__).Fail(fmt, (p1))

#define UNIMPLEMENTED() FATAL("UNIMPLEMENTED CODE")

#define UNIMPLEMENTED1(p1) FATAL1("UNIMPLEMENTED CODE", p1)

#define UNREACHABLE() FATAL("UNREACHABLE CODE")

#define UNREACHABLE1(p1) FATAL1("UNREACHABLE CODE", p1)

// -----------------------------------------------------------------------------
// ASSERT Macro (use -DNDEBUG to disable assertions)
//
#if defined(NDEBUG)
// Use 'cond' to avoid 'variable unused warnings' when not building in debug mode.
//
#define ASSERT(cond) \
	do           \
	{            \
	} while (0 && (cond))

#else

#define ASSERT(cond)                                                                            \
	do                                                                                      \
	{                                                                                       \
		if (!(cond)) archsim::Assertion(__FILE__, __LINE__).Fail("EXPECTED: %s", #cond); \
	} while (0)

#endif  // if defined(DEBUG)

#endif  // INC_ASSERTION_H_
