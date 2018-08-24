/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */


#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <cstdio>

#include <cstring>
#include <string>

#include "concurrent/Mutex.h"
#include "concurrent/ScopedLock.h"
#include "util/system/SharedLibrary.h"

#include "util/LogContext.h"

DeclareLogContext(LogSharedLibraries, "SharedLibraries");

namespace archsim
{
	namespace util
	{
		namespace system
		{

			SharedLibrary::SharedLibrary() {}

// Open and load shared library setting the handle
			bool SharedLibrary::Open(const std::string& path)
			{
				bool success = true;
				struct stat lib_status;

				if ((stat(path.c_str(), &lib_status) == 0) && S_ISREG(lib_status.st_mode)) {
					if ((lib_handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL)) == NULL) {
						LC_ERROR(LogSharedLibraries) << "Opening dynamic library '" << path << "' failed: '" << dlerror() << "'";
						success = false;
					}
				} else {
					LC_ERROR(LogSharedLibraries) << "Opening dynamic library '" << path << "' failed: '" << strerror(errno) << "'";
					success = false;
				}

				return success;
			}

// Close shared library
			bool SharedLibrary::Close()
			{
				static archsim::concurrent::Mutex mutex;
				archsim::concurrent::ScopedLock lock(mutex);

				if (lib_handle != NULL) {
					// Clear any old error conditions
					dlerror();

					if (dlclose(lib_handle) != 0) {
						char* err_msg = dlerror();
						LC_ERROR(LogSharedLibraries) << "Closing dynamic library failed: " << err_msg;
						return false;
					}
				}
				return true;
			}

// Lookup symbol in loaded library
//
			void *SharedLibrary::LookupSymbol(std::string symbol_name)
			{
				char* err_msg = NULL;

				// --------------------------------------------------------------------
				// Resolve address for symbol name. Make sure you read 'man dlsym' and
				// understand how to correctly lookup symbols and check for errors before
				// modifying the following code!
				// NOTE THAT CALL TO dlsym and dlerror must be atomic AND synchronized
				//      to enable calling this method from a parrallel or concurrent context
				// --------------------------------------------------------------------

				if (lib_handle != NULL) {
					// Clear any old error conditions
					dlerror();

					// Try to resolve symbol
					void *symbol_addr = dlsym(lib_handle, symbol_name.c_str());

					// Did symbol lookup succeed
					if ((err_msg = dlerror()) != NULL) {
						// Symbol lookup failed
						LC_ERROR(LogSharedLibraries) << "Looking up symbol '" << symbol_name << "' failed: '" << err_msg;
						return NULL;
					}
					return symbol_addr;

				}

				return NULL;
			}
		}
	}
}  // namespace archsim::util::system
