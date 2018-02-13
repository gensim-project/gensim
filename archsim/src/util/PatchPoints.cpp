#include "util/PatchPoints.h"

#include <dlfcn.h>
#include <string.h>
#include <sys/mman.h>

using namespace archsim::util;

bool GetPatchPoint(const char *name, char*& start, char*& end)
{
	char buffer[256];

	snprintf(buffer, 255, "__patchpoint_start_%s", name);
	start = (char *)dlsym(NULL, buffer);

	snprintf(buffer, 255, "__patchpoint_end_%s", name);
	end = (char *)dlsym(NULL, buffer);

	return start != NULL && end != NULL;
}

bool PatchPoints::Enable(std::string name)
{
	char *start, *end;

	if (!GetPatchPoint(name.c_str(), start, end))
		return false;


	return true;
}

bool PatchPoints::Disable(std::string name)
{
	char *start, *end;

	if (!GetPatchPoint(name.c_str(), start, end))
		return false;

	mprotect((void *)((unsigned long)start & ~4095L), end - start, PROT_READ | PROT_EXEC | PROT_WRITE);
	memset(start, 0x90, end - start);
	mprotect((void *)((unsigned long)start & ~4095L), end - start, PROT_READ | PROT_EXEC);
	return true;
}

bool PatchPoints::Apply()
{
	return Disable("AWD");
}
