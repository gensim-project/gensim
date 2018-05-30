//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2005 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file defines the classes used to represent the esystem.
//  The 'system' is an SoC device containing a 'processor', 'memories'
//  and possibly I/O devices.
//
// =====================================================================

#ifndef _system_h_
#define _system_h_

#include "define.h"
#include "core/execution/ExecutionContextManager.h"
#include "util/PubSubSync.h"
#include "concurrent/Barrier.h"
#include "translate/profile/CodeRegionTracker.h"
#include "abi/devices/generic/block/BlockCache.h"
#include "abi/devices/generic/block/BlockDevice.h"
#include "module/ModuleManager.h"
#include "session.h"

#include <ostream>
#include <set>
#include <map>
#include <vector>

#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

namespace archsim
{
	class Session;

	namespace ij
	{
		class IJManager;
	}
	namespace translate
	{
		class TranslationManager;
	}
	namespace abi
	{
		class EmulationModel;

		namespace devices
		{
			namespace timing
			{
				class TickSource;
			}
		}

	}
	namespace uarch
	{
		class uArch;
	}
	namespace util
	{
		class PerformanceSource;
	}
}

class System
{
public:
	struct segfault_data {
		uint64_t addr;
	};

	typedef bool (*segfault_handler_t)(void *ctx, const segfault_data& data);


	uint32_t exit_code;

	std::string block_device_file;

	explicit System(archsim::Session& session);
	~System();

	bool Initialise();
	void Destroy();

	bool RunSimulation();
	void HaltSimulation();

	void EnableVerify();
	void SetVerifyNext(System *sys);
	void CheckVerify();
	void InitSocketVerify();
	static void InitVerify();

	void PrintStatistics(std::ostream& stream);

	inline archsim::abi::EmulationModel& GetEmulationModel() const
	{
		return *emulation_model;
	}

	inline bool HasBreakpoints() const
	{
		return breakpoints.size() > 0;
	}

	inline bool ShouldBreak(uint32_t addr) const
	{
		return breakpoints.count(addr);
	}

	inline void RegisterSegFaultHandler(uint64_t addr, size_t size, void *ctx, segfault_handler_t handler)
	{
		assert((addr & (getpagesize()-1)) == 0);
		assert(segfault_handlers.find(addr) == segfault_handlers.end());
		segfault_handler_registration_t& rg = segfault_handlers[addr];

		rg.size = size;
		rg.handler = handler;
		rg.ctx = ctx;
	}

	inline void AddPerformanceSource(archsim::util::PerformanceSource& source)
	{
		performance_sources.push_back(&source);
	}

	const std::vector<archsim::util::PerformanceSource *>& GetPerformanceSources() const
	{
		return performance_sources;
	}

	bool HandleSegFault(uint64_t addr);

	archsim::util::PubSubContext &GetPubSub()
	{
		return pubsubctx;
	}

	archsim::abi::devices::timing::TickSource *GetTickSource()
	{
		assert(_tick_source);
		return _tick_source;
	}
	void SetTickSource(archsim::abi::devices::timing::TickSource *new_source)
	{
		assert(!_tick_source);
		_tick_source = new_source;
	}

	inline archsim::Session& GetSession() const
	{
		return session;
	}

	inline archsim::translate::profile::CodeRegionTracker &GetCodeRegions()
	{
		return code_region_tracker_;
	}

	//TODO: move this into user mode emulation model
	inline int GetFD(int guest_fd)
	{
		return fds[guest_fd];
	}
	inline int OpenFD(int host_fd)
	{
		fds[max_fd++] = host_fd;
		return max_fd-1;
	}
	inline int CloseFD(int guest_fd)
	{
		int i = close(GetFD(guest_fd));
		fds.erase(guest_fd);
		return i;
	}

	inline void InstallBlockDevice(std::string name, archsim::abi::devices::generic::block::BlockDevice* dev)
	{
		_block_devices[name] = dev;
	}
	inline archsim::abi::devices::generic::block::BlockDevice* GetBlockDevice(std::string name)
	{
		return _block_devices.at(name);
	}

	inline archsim::module::ModuleManager &GetModuleManager()
	{
		return GetSession().GetModuleManager();
	}
	
	inline archsim::core::execution::ExecutionContextManager &GetECM()
	{
		return exec_ctx_mgr_;
	}

private:
	std::map<std::string, archsim::abi::devices::generic::block::BlockDevice*> _block_devices;

	std::map<int, int> fds;
	int max_fd;

	archsim::Session& session;
	archsim::core::execution::ExecutionContextManager exec_ctx_mgr_;
	
	static archsim::concurrent::LWBarrier2 _verify_barrier_enter;
	static archsim::concurrent::LWBarrier2 _verify_barrier_leave;
	bool _verify;
	System *_next_verify_system;
	uint32_t _verify_tid;
	struct sockaddr_un _verify_socket;
	int _verify_socket_fd;
	int _verify_bundle_index;
	int _verify_chunk_size;

	bool _halted;

	archsim::util::PubSubContext pubsubctx;
	archsim::translate::profile::CodeRegionTracker code_region_tracker_;

	archsim::module::ModuleManager module_manager_;

	std::vector<archsim::util::PerformanceSource *> performance_sources;
	std::set<uint32_t> breakpoints;

	struct segfault_handler_registration_t {
		size_t size;
		void *ctx;
		segfault_handler_t handler;
	};

	typedef std::map<uint64_t, segfault_handler_registration_t> segfault_handler_map_t;

	segfault_handler_map_t segfault_handlers;

	bool Simulate(bool trace);

	archsim::abi::EmulationModel *emulation_model;
	archsim::uarch::uArch *uarch;


	archsim::abi::devices::timing::TickSource *_tick_source;
};

#endif
