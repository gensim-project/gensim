//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Class implementing functionality for opening/loading shared libraries
// and resolving symbols.
//
// =====================================================================

#ifndef _SharedLibrary_h_
#define _SharedLibrary_h_

#include <string>

namespace archsim
{
	namespace util
	{
		namespace system
		{

// class
//
			class SharedLibrary
			{
			public:
				SharedLibrary();

			private:
				SharedLibrary(const SharedLibrary& m);  // DO NOT COPY
				void operator=(const SharedLibrary&);   // DO NOT ASSIGN

				void *lib_handle;

			public:
				// Open and load shared library setting the handle
				bool Open(const std::string& path);

				// Close shared library
				bool Close();

				// Lookup symbol in loaded library
				void *LookupSymbol(std::string symbol_name);
			};
		}
	}
}  // namespace archsim::util::system

#endif /*  _SharedLibrary_h_ */
