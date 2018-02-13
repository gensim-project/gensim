/*
 * File:   PatchPoints.h
 * Author: s0457958
 *
 * Created on 13 August 2014, 13:01
 */

#ifndef PATCHPOINTS_H
#define	PATCHPOINTS_H

#define PATCH_DISABLED	0
#define PATCH_ENABLED	1



#define PATCH_START(__name, __uid, __dfl) { asm(".globl __patchpoint_start_" # __uid "\n__patchpoint_start_" # __uid ":\n");
#define PATCH_END(__uid) \
asm(".globl __patchpoint_end_" # __uid "\n__patchpoint_end_" # __uid ": \n"); \
asm(".pushsection patch_points\n"); \
asm(".pushsection patch_point_names\n"); \
asm("__patchpoint_name_" # __uid ":\n"); \
asm(".asciz \"" # __uid "\"\n"); \
asm(".popsection\n"); \
asm(".long __patchpoint_start_" # __uid); \
asm(".long __patchpoint_end_" # __uid); \
asm(".long __patchpoint_name_" # __uid); \
asm(".popsection\n"); \
}

#include <string>

namespace archsim
{
	namespace util
	{
		class PatchPoints
		{
		public:
			static bool Enable(std::string name);
			static bool Disable(std::string name);
			static bool Apply();
		};
	}
}

#endif	/* PATCHPOINTS_H */

