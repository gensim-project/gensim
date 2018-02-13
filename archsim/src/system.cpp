/*
 * system.cpp
 */
#include "system.h"

#include "abi/EmulationModel.h"
#include "abi/memory/MemoryCounterEventHandler.h"
#include "abi/devices/generic/timing/TickSource.h"

#include "gensim/gensim.h"
#include "gensim/gensim_processor.h"

//#include "tracing/TraceManager.h"
#include "translate/TranslationManager.h"

#include "util/ComponentManager.h"
#include "util/LogContext.h"
#include "util/SimOptions.h"
#include "util/LivePerformanceMeter.h"

#include "uarch/uArch.h"

#include <iostream>

DeclareLogContext(LogSystem, "System");

System::System(archsim::Session& session) :
	session(session),
	exit_code(EXIT_SUCCESS),
	mode(Interpreter),
	txln_mgr(NULL),
	ij_mgr(NULL),
	uarch(NULL),
	emulation_model(NULL),
	_verify(false),
	_next_verify_system(NULL),
	_verify_tid(-1),
	_halted(false),
	_tick_source(NULL),
	profile_manager(&pubsubctx)
{
	max_fd = 0;
	OpenFD(STDIN_FILENO);
	OpenFD(STDOUT_FILENO);
	OpenFD(STDERR_FILENO);
}

System::~System()
{
}

/**
 * Initialises the simulation system
 * @return Returns true if the simulation system was successfully initialised, false otherwise.
 */
bool System::Initialise()
{
	LC_DEBUG1(LogSystem) << "Initialising System";

	if (archsim::options::Mode == "interp") {
		LC_INFO(LogSystem) << "Interpretive Mode Enabled";
		mode = Interpreter;
	} else if (archsim::options::Mode == "single-step") {
		LC_INFO(LogSystem) << "Single-step Mode Enabled";
		mode = SingleStep;
	} else {
		LC_INFO(LogSystem) << "JIT Mode Enabled";
		mode = JIT;
	}

	if (mode == JIT) {
		// Create and initialise the translation manager
		if (!GetComponentInstance(archsim::options::JitTranslationManager, txln_mgr, &pubsubctx)) {
			LC_ERROR(LogSystem) << "Unable to create translation manager '" << archsim::options::JitTranslationManager.GetValue() << "'";
			return false;
		}

		if (!txln_mgr->Initialise()) {
			LC_ERROR(LogSystem) << "Unable to initialise translation manager '" << archsim::options::JitTranslationManager.GetValue() << "'";
			return false;
		}
	}

	// Create and initialise the emulation model
	if (!GetComponentInstance(archsim::options::EmulationModel, emulation_model)) {
		if(txln_mgr) {
			txln_mgr->Destroy();
			delete txln_mgr;
		}

		LC_ERROR(LogSystem) << "Unable to create emulation model '" << archsim::options::EmulationModel.GetValue() << "'";
		LC_ERROR(LogSystem) << GetRegisteredComponents(emulation_model);
		return false;
	}

	uarch = new archsim::uarch::uArch();

	if (!uarch->Initialise()) {
		delete uarch;

		txln_mgr->Destroy();
		delete txln_mgr;

		LC_ERROR(LogSystem) << "Unable to initialise uArch model";
		return false;
	}

	if (!emulation_model->Initialise(*this, *uarch)) {
		if(uarch != nullptr) {
			uarch->Destroy();
			delete uarch;
		}

		if(txln_mgr != nullptr) {
			txln_mgr->Destroy();
			delete txln_mgr;
		}

		LC_ERROR(LogSystem) << "Unable to initialise emulation model '" << archsim::options::EmulationModel.GetValue() << "'";
		return false;
	}

	if(txln_mgr)txln_mgr->SetManager(profile_manager);

	return true;
}

void System::Destroy()
{
	LC_DEBUG1(LogSystem) << "Destroying system";

	// We have to destroy the translation manager here, although it upsets me that
	// it's breaking symmetry, because if we're using the async model, it needs to
	// have released all references to the CPU, before we destroy it.
	if(txln_mgr) {
		txln_mgr->Destroy();
		delete txln_mgr;
	}

	emulation_model->Destroy();
	delete emulation_model;

	uarch->Destroy();
	delete uarch;
}

void System::PrintStatistics(std::ostream& stream)
{
	stream << std::endl;

	stream << "Event Statistics" << std::endl;
	pubsubctx.PrintStatistics(stream);

	stream << "Simulation Statistics" << std::endl;

	// Print Emulation Model statistics
	emulation_model->PrintStatistics(stream);

	stream << std::endl;

	// Print translation manager statistics
	if(txln_mgr)txln_mgr->PrintStatistics(stream);

	stream << std::endl;
}

bool System::RunSimulation()
{
	gensim::Processor *core = emulation_model->GetBootCore();

	if (!core) {
		LC_ERROR(LogSystem) << "No processor core configured!";
	}

	// Prepare to boot the system
	if (!emulation_model->PrepareBoot(*this)) return false;

	// Resolve breakpoints
	for (auto breakpoint_fn : *archsim::options::Breakpoints.GetValue()) {
		unsigned long breakpoint_addr;

		if (!emulation_model->ResolveSymbol(breakpoint_fn, breakpoint_addr) ) {
			LC_WARNING(LogSystem) << "Unable to resolve function symbol: " << breakpoint_fn;
			continue;
		}

		LC_DEBUG1(LogSystem) << "Resolved function breakpoint: " << breakpoint_fn << " @ " << std::hex << breakpoint_addr;
		breakpoints.insert(breakpoint_addr);
	}

	// Start live performance measurement (if necessary)
	archsim::util::LivePerformanceMeter *perf_meter = NULL;
	if (archsim::options::LivePerformance) {
		perf_meter = new archsim::util::LivePerformanceMeter(*core, performance_sources, "perf.dat", 1000000);
		perf_meter->start();
	}

	bool active = true;

	// TODO: Turn into a command-line option
	uint32_t steps = 1000000;

	while (active && !core->halted) {

		switch (mode) {
			case Interpreter:
				active = core->RunInterp(steps);
				break;

			case JIT:
				active = core->RunJIT(archsim::options::Verbose, steps);
				break;

			case SingleStep:
				//fprintf(stderr, "[%08x] %s\n", core->read_pc(), core->GetDisasm()->DisasmInstr(*core->curr_interpreted_insn(), core->read_pc()).c_str());
				active = core->RunSingleInstruction();
				std::cin.ignore();
				break;

			default:
				fprintf(stderr, "unsupported simulation mode\n");
				active = false;
				break;
		}
	}

	if (perf_meter) {
		perf_meter->stop();
		delete perf_meter;
	}

	return true;
}

void System::HaltSimulation()
{
	_halted = true;
	emulation_model->HaltCores();

	_verify_barrier_enter.Interrupt();
	_verify_barrier_leave.Interrupt();
}

void System::EnableVerify()
{
	_verify = true;
}

void System::SetVerifyNext(System *sys)
{
	if(sys) _verify_tid = 1;
	else _verify_tid = 0;
	_next_verify_system = sys;
}

static void DisplayRB(gensim::Processor *cpu_x, gensim::Processor *cpu_y, int rb_idx)
{
	auto desc = cpu_x->GetRegisterBankDescriptor(rb_idx);

	uint32_t *my_rb = ((uint32_t*)desc.GetBankDataStart());
	uint32_t *next_rb = ((uint32_t*)cpu_y->GetRegisterBankDescriptor(rb_idx).GetBankDataStart());

	for(uint32_t i = 0; i < desc.NumberOfRegistersInBank(); ++i) {
		fprintf(stderr, "%u\t%08x\t%08x", i, my_rb[i], next_rb[i]);
		if(my_rb[i] != next_rb[i]) fprintf(stderr, " !!!\n");
		else fprintf(stderr, "\n");
	}

	fprintf(stderr, "\n\n");

}

static void ReportDivergent(gensim::Processor *cpu_x, gensim::Processor *cpu_y)
{
	fprintf(stderr, "***** DIVERGENCE DETECTED ***** \n");

	fprintf(stderr, "RB:\n");
	DisplayRB(cpu_x, cpu_y, 0);
	fprintf(stderr, "FPRB:\n");
	DisplayRB(cpu_x, cpu_y, 1);

	int num_regs = cpu_x->GetRegisterCount();
	for(int i = 0; i < num_regs; ++i) {
		if(cpu_x->GetRegisterDescriptor(i).RegisterWidth != 1) continue;
		uint8_t x_d = *(uint8_t*)cpu_x->GetRegisterDescriptor(i).DataStart;
		uint8_t y_d = *(uint8_t*)cpu_y->GetRegisterDescriptor(i).DataStart;

		fprintf(stderr, "%s\t%02x\t%02x", cpu_x->GetRegisterDescriptor(i).Name.c_str(), x_d, y_d);
		if(x_d != y_d) fprintf(stderr, " !!!\n");
		else fprintf(stderr, "\n");
	}



	fprintf(stderr, "\n\n");

//	fprintf(stderr, "Sys Mem State X\n");
//	((archsim::abi::memory::SystemMemoryModel&)cpu_x->GetMemoryModel()).Dump();
//	fprintf(stderr, "\n\n\nSys Mem State Y\n");
//	((archsim::abi::memory::SystemMemoryModel&)cpu_y->GetMemoryModel()).Dump();
}

#define INSTRUCTION_BUNDLE_SIZE 1536

void dump_buffer(char *buffer, int size)
{
	printf("%u: ", size);
	while(size--) {
		printf("%02x ", *buffer);
		buffer++;
	}
	printf("\n");
}

int send_buffer(int socket_fd, char *buffer, int size)
{
	int bytes_sent = 0;
//	dump_buffer(buffer, size);
	while(bytes_sent < size) {
		bytes_sent += send(socket_fd, buffer+bytes_sent, size-bytes_sent, 0);
	}

	return 0;
}

char* verify_buffer = nullptr;

void System::CheckVerify()
{
	if(_halted) return;

	bool halt_simulation = false;

	if(archsim::options::VerifyMode == "callsonly") {
		((archsim::abi::devices::timing::CallbackTickSource*)_tick_source)->DoTick();

		gensim::Processor * my_cpu = GetEmulationModel().GetBootCore();
		uint32_t* my_bank = (uint32_t*)my_cpu->GetRegisterFilePointer();
		uint32_t pc = my_cpu->read_pc();
		if(pc == 0xb6f8c7b4) {
			printf("\n\n%08x\n\n", my_bank[12]);
		}
	} else if(archsim::options::VerifyMode == "socket") {
		// Send the primary register bank to the verification server.
		// The server will respond with a single byte indicating whether
		// to continue (0) or halt simulation (1)

		((archsim::abi::devices::timing::CallbackTickSource*)_tick_source)->DoTick();

		gensim::Processor * volatile my_cpu;
		my_cpu = GetEmulationModel().GetBootCore();

		uint32_t* my_bank = (uint32_t*)my_cpu->GetRegisterFilePointer();
		uint32_t bank_size = (uint32_t)my_cpu->GetRegisterFileSize();

		if(!verify_buffer) {
			verify_buffer = (char*)malloc(bank_size * _verify_chunk_size);
		}

		memcpy(verify_buffer + (bank_size * _verify_bundle_index), (void*)my_bank, bank_size);

		uint8_t status = 0;
		_verify_bundle_index++;

		if(_verify_bundle_index == _verify_chunk_size) {
			send_buffer(_verify_socket_fd, verify_buffer, bank_size * _verify_chunk_size);
			recv(_verify_socket_fd, &status, 1, 0);
			_verify_bundle_index = 0;
		}

		if(status) {
			halt_simulation = true;
		}
	} else {

		_verify_barrier_enter.Wait(_verify_tid);
		if(_halted) return;

		if(_next_verify_system) {
			((archsim::abi::devices::timing::CallbackTickSource*)_tick_source)->DoTick();

			// do some verification...
			gensim::Processor * volatile my_cpu;
			gensim::Processor * volatile next_cpu;
			my_cpu = GetEmulationModel().GetBootCore();
			next_cpu = _next_verify_system->GetEmulationModel().GetBootCore();

			auto &my_desc = my_cpu->GetRegisterBankDescriptor(0);
			void* my_bank = my_cpu->GetRegisterBankDescriptor(0).GetBankDataStart();
			void* other_bank = next_cpu->GetRegisterBankDescriptor(0).GetBankDataStart();

			// Two alternatives: check all registers (including PC) = fast
			//                   don't check PC = slower

			/*
			if(memcmp(my_bank, other_bank, my_desc.RegisterCount * my_desc.RegisterWidth)) {
				ReportDivergent(my_cpu, next_cpu);
				asm("int3\n");
				halt_simulation = true;
			}
			*/

			for(uint32_t i = 0; i < my_desc.NumberOfRegistersInBank()-1; ++i) {
				if(((uint32_t*)my_bank)[i] != ((uint32_t*)other_bank)[i]) {
					ReportDivergent(my_cpu, next_cpu);
					asm("int3\n");
					halt_simulation = true;
				}
			}

			/*
			//Check FP banks
			auto &my_fpu_desc = my_cpu->GetRegisterBankDescriptor(1);
			void* my_fpu_bank = my_cpu->GetRegisterBankDescriptor(1).BankDataStart;
			void* other_fpu_bank = next_cpu->GetRegisterBankDescriptor(1).BankDataStart;

			if(memcmp(my_fpu_bank, other_fpu_bank, my_desc.RegisterCount * my_desc.RegisterWidth)) {
				ReportDivergent(my_cpu, next_cpu);
				asm("int3\n");
				halt_simulation = true;
			}
			*/

			if(!halt_simulation) {
				int num_regs = my_cpu->GetRegisterCount();
				for(int i = 0; i < num_regs; ++i) {
					if(my_cpu->GetRegisterDescriptor(i).RegisterWidth != 1) continue;
					uint8_t my_data = *(uint8_t*)my_cpu->GetRegisterDescriptor(i).DataStart;
					uint8_t next_data = *(uint8_t*)next_cpu->GetRegisterDescriptor(i).DataStart;

					if(my_data != next_data) {
						ReportDivergent(my_cpu, next_cpu);
						asm("int3\n");
						halt_simulation = true;
						break;
					}
				}
			}

		}

		_verify_barrier_leave.Wait(_verify_tid);
	}



	if(halt_simulation) {
		if(_next_verify_system)_next_verify_system->HaltSimulation();
		asm("int3");
		HaltSimulation();
	}
}

archsim::concurrent::LWBarrier2 System::_verify_barrier_enter;
archsim::concurrent::LWBarrier2 System::_verify_barrier_leave;
void System::InitVerify()
{
}

void System::InitSocketVerify()
{
	_verify_bundle_index = 0;

	_verify_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(_verify_socket_fd == -1) {
		perror("Could not initialise socket");
		exit(1);
	}

	_verify_socket.sun_family = AF_UNIX;
	strcpy(_verify_socket.sun_path, "/tmp/archsim_verify_server");

	if(connect(_verify_socket_fd, (struct sockaddr*)&_verify_socket, sizeof(struct sockaddr_un)) == -1) {
		perror("Could not connect to socket");
		exit(1);
	}

	gensim::Processor * my_cpu;
	my_cpu = GetEmulationModel().GetBootCore();

	uint32_t bank_size = my_cpu->GetRegisterFileSize();

	send(_verify_socket_fd, &bank_size, 4, 0);

	uint8_t rcode;
	recv(_verify_socket_fd, &rcode, 1, 0);
	if(rcode) {
		fprintf(stderr, "Bank size mismatch\n");
		exit(1);
	}

	uint32_t chunk_size;
	recv(_verify_socket_fd, &chunk_size, 4, 0);
	_verify_chunk_size = chunk_size;
}

bool System::HandleSegFault(uint64_t addr)
{
	if (segfault_handlers.size() == 0)
		return false;

	segfault_handler_map_t::const_iterator iter = segfault_handlers.upper_bound(addr);
	iter--;

	if (addr >= iter->first && (addr < iter->first + iter->second.size)) {
		segfault_data data;
		data.addr = addr;

		bool falafel = iter->second.handler(iter->second.ctx, data);
		return falafel;
	} else {
		return false;
	}
}
