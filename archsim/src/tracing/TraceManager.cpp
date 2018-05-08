/*
 * TraceManager.cpp
 *
 *  Created on: 8 Aug 2014
 *      Author: harry
 */

#include "define.h"

#include "util/LogContext.h"
#include "util/ComponentManager.h"

#include "gensim/gensim_decode.h"
#include "tracing/TraceManager.h"
#include "tracing/TraceTypes.h"

#include <lz4.h>

#include <cassert>
#include <errno.h>
#include <map>
#include <string>
#include <streambuf>
#include <string.h>
#include <stdio.h>
#include <vector>
