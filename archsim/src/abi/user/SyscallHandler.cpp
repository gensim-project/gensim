/*
 * abi/user/SyscallHandler.cpp
 */
#include "abi/user/SyscallHandler.h"
#include "abi/UserEmulationModel.h"

#include "util/LogContext.h"

using namespace archsim::abi::user;

UseLogContext(LogEmulationModelUser);
DeclareChildLogContext(LogSyscalls, LogEmulationModelUser, "Syscalls");

SyscallHandler::SyscallHandler() {}

SyscallHandler::~SyscallHandler() {}

SyscallHandler& SyscallHandlerProvider::Get(const arch_descriptor_t& arch)
{
	return handler_map_[arch];
}

SyscallHandlerProvider& SyscallHandlerProvider::Singleton()
{
	if(singleton_ == nullptr) {
		singleton_ = new SyscallHandlerProvider();
	}
	return *singleton_;
}

SyscallHandlerProvider *SyscallHandlerProvider::singleton_ = nullptr;

void SyscallHandler::RegisterSyscall(unsigned int nr, SYSCALL_FN_GENERIC fn, const std::string& name)
{
	syscall_fns[nr] = fn;
	syscall_fn_names[nr] = std::string(name);
}

std::string format_rval(int val)
{
	std::ostringstream ostr;
	ostr << val;
	switch(val) {
		case -EBADF:
			ostr << " (EBADF)";
			break;
		case -EINVAL:
			ostr << " (EINVAL)";
			break;
		case -EOVERFLOW:
			ostr << " (EOVERFLOW)";
			break;
		case -ESPIPE:
			ostr << " (ESPIPE)";
			break;
		case -ENXIO:
			ostr << " (ENXIO)";
			break;
	}
	return ostr.str();
}

bool SyscallHandler::HandleSyscall(SyscallRequest& request, SyscallResponse& response) const
{
	std::map<unsigned int, SYSCALL_FN_GENERIC>::const_iterator syscall = syscall_fns.find(request.syscall);

	if (syscall == syscall_fns.end()) {
		LC_WARNING(LogSyscalls) << "Unregistered Syscall " << request.syscall;
		return false;
	} else {
		char buffer[128];
		sprintf(buffer, syscall_fn_names.at(request.syscall).c_str(), request.arg0, request.arg1, request.arg2, request.arg3, request.arg4, request.arg5);
		LC_DEBUG1(LogSyscalls) << "Handling registered syscall " << buffer;

		response.result = (syscall->second)(request.thread, request.arg0, request.arg1, request.arg2, request.arg3, request.arg4, request.arg5);

		LC_DEBUG1(LogSyscalls) << "Syscall returned " << format_rval(response.result);
		return true;
	}
}
