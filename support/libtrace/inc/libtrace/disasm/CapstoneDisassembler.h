/* This file is Copyright University of Edinburgh 2020. For license details, see LICENSE. */
#ifndef CAPSTONE_DISASSEMBLER_H_
#define CAPSTONE_DISASSEMBLER_H_

#include "Disassembler.h"
#include <string>

namespace libtrace
{
    namespace disasm {
        class CapstoneDisassembler : public Disassembler
        {
        public:
            CapstoneDisassembler(const std::string& arch);
            virtual ~CapstoneDisassembler();

            virtual std::string Disassemble(const InstructionCodeReader& icr) override;

        private:
            void *csh_;
            bool init_;
        };
    }
}

#endif
