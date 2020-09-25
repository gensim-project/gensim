/* This file is Copyright University of Edinburgh 2020. For license details, see LICENSE. */
#include "libtrace/disasm/CapstoneDisassembler.h"
#include "libtrace/RecordReader.h"
#include <capstone/capstone.h>

using namespace libtrace;
using namespace libtrace::disasm;

CapstoneDisassembler::CapstoneDisassembler(const std::string& arch) : csh_(nullptr), init_(false)
{
    if (arch == "arm") {
        init_ = cs_open(CS_ARCH_ARM, CS_MODE_ARM, (csh *)&csh_) == CS_ERR_OK;
    } else if (arch == "arm64") {
        init_ = cs_open(CS_ARCH_ARM64, CS_MODE_ARM, (csh *)&csh_) == CS_ERR_OK;
    } else if (arch == "x86-64") {
        init_ = cs_open(CS_ARCH_X86, CS_MODE_64, (csh *)&csh_) == CS_ERR_OK;
    } else if (arch == "x86-32") {
        init_ = cs_open(CS_ARCH_X86, CS_MODE_32, (csh *)&csh_) == CS_ERR_OK;
    }
}

CapstoneDisassembler::~CapstoneDisassembler()
{
    if (init_) {
        cs_close((csh *)&csh_);
    }
}

std::string CapstoneDisassembler::Disassemble(const InstructionCodeReader& icr)
{
    if (!init_) {
        return "<error>";
    }

    unsigned int instruction_data = icr.GetCode().AsU32();

    cs_insn *insn;

    size_t count = cs_disasm((csh)csh_, (const uint8_t *)&instruction_data, sizeof(instruction_data), 0, 0, &insn);

    if (count == 0) {
        return "<error>";
    } else {
        std::string r = std::string(insn[0].mnemonic) + " " + std::string(insn[0].op_str);
        cs_free(insn, count);

        return r;
    }
}
