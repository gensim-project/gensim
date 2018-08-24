/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef __ANTLR_VER_H__
#define __ANTLR_VER_H__
#include <antlr3config.h>

#ifdef ANTLR3_ENC_8BIT
#define antlr3NewAsciiStringCopyStream(a, b, c) antlr3StringStreamNew(a, ANTLR3_ENC_8BIT, b, c)
#define antlr3AsciiFileStreamNew(name) antlr3FileStreamNew(name, ANTLR3_ENC_8BIT)
#else
#define ANTLR3_ENC_8BIT 0
#define antlr3StringStreamNew(a, b, c, d) antlr3NewAsciiStringCopyStream(a, c, d)
#define antlr3StringFactoryNew(a) antlr3StringFactoryNew()
#endif

#endif
