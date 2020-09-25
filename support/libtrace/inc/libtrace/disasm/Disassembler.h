/* This file is Copyright University of Edinburgh 2020. For license details, see LICENSE. */
#ifndef DISASSEMBLER_H_
#define DISASSEMBLER_H_

#include <string>

namespace libtrace
{
    class InstructionCodeReader;

    namespace disasm
    {
        class Disassembler
        {
        public:
            virtual ~Disassembler() {}
            virtual std::string Disassemble(const InstructionCodeReader& icr) = 0;
        };
    }
}

#endif
