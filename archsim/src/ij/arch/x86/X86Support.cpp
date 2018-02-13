#include "ij/arch/x86/X86Support.h"

using namespace archsim::ij::arch::x86;

// 8-byte registers
const X86Register X86Register::RAX(8, 0, "rax");
const X86Register X86Register::RCX(8, 1, "rcx");
const X86Register X86Register::RDX(8, 2, "rdx");
const X86Register X86Register::RBX(8, 3, "rbx");
const X86Register X86Register::RSP(8, 4, "rsp");
const X86Register X86Register::RBP(8, 5, "rbp");
const X86Register X86Register::RSI(8, 6, "rsi");
const X86Register X86Register::RDI(8, 7, "rdi");

// 8-byte high registers
const X86Register X86Register::R8(8, 0, "r8", true);
const X86Register X86Register::R9(8, 1, "r9", true);
const X86Register X86Register::R10(8, 2, "r10", true);
const X86Register X86Register::R11(8, 3, "r11", true);
const X86Register X86Register::R12(8, 4, "r12", true);
const X86Register X86Register::R13(8, 5, "r13", true);
const X86Register X86Register::R14(8, 6, "r14", true);
const X86Register X86Register::R15(8, 7, "r15", true);

// 4-byte registers
const X86Register X86Register::EAX(4, 0, "eax");
const X86Register X86Register::ECX(4, 1, "ecx");
const X86Register X86Register::EDX(4, 2, "edx");
const X86Register X86Register::EBX(4, 3, "ebx");
const X86Register X86Register::ESP(4, 4, "esp");
const X86Register X86Register::EBP(4, 5, "ebp");
const X86Register X86Register::ESI(4, 6, "esi");
const X86Register X86Register::EDI(4, 7, "edi");

// 4-byte high registers
const X86Register X86Register::R8D(4, 0, "r8d", true);
const X86Register X86Register::R9D(4, 1, "r9d", true);
const X86Register X86Register::R10D(4, 2, "r10d", true);
const X86Register X86Register::R11D(4, 3, "r11d", true);
const X86Register X86Register::R12D(4, 4, "r12d", true);
const X86Register X86Register::R13D(4, 5, "r13d", true);
const X86Register X86Register::R14D(4, 6, "r14d", true);
const X86Register X86Register::R15D(4, 7, "r15d", true);

// 2-byte registers
const X86Register X86Register::AX(2, 0, "ax");
const X86Register X86Register::CX(2, 1, "cx");
const X86Register X86Register::DX(2, 2, "dx");
const X86Register X86Register::BX(2, 3, "bx");
const X86Register X86Register::SP(2, 4, "sp");
const X86Register X86Register::BP(2, 5, "bp");
const X86Register X86Register::SI(2, 6, "si");
const X86Register X86Register::DI(2, 7, "di");

// 2-byte high registers
const X86Register X86Register::R8W(2, 0, "r8w", true);
const X86Register X86Register::R9W(2, 1, "r9w", true);
const X86Register X86Register::R10W(2, 2, "r10w", true);
const X86Register X86Register::R11W(2, 3, "r11w", true);
const X86Register X86Register::R12W(2, 4, "r12w", true);
const X86Register X86Register::R13W(2, 5, "r13w", true);
const X86Register X86Register::R14W(2, 6, "r14w", true);
const X86Register X86Register::R15W(2, 7, "r15w", true);

// 1-byte registers
const X86Register X86Register::AL(1, 0, "al");
const X86Register X86Register::CL(1, 1, "cl");
const X86Register X86Register::DL(1, 2, "dl");
const X86Register X86Register::BL(1, 3, "bl");
const X86Register X86Register::AH(1, 4, "ah");
const X86Register X86Register::CH(1, 5, "ch");
const X86Register X86Register::DH(1, 6, "dh");
const X86Register X86Register::BH(1, 7, "bh");

// 1-byte new registers
const X86Register X86Register::BPL(1, 0, "bpl", false, true);
const X86Register X86Register::SPL(1, 1, "spl", false, true);
const X86Register X86Register::SIL(1, 2, "sil", false, true);
const X86Register X86Register::DIL(1, 3, "dil", false, true);

// 1-byte high registers
const X86Register X86Register::R8B(1, 0, "r8b", true);
const X86Register X86Register::R9B(1, 0, "r9b", true);
const X86Register X86Register::R10B(1, 0, "r10b", true);
const X86Register X86Register::R11B(1, 0, "r11b", true);
const X86Register X86Register::R12B(1, 0, "r12b", true);
const X86Register X86Register::R13B(1, 0, "r13b", true);
const X86Register X86Register::R14B(1, 0, "r14b", true);
const X86Register X86Register::R15B(1, 0, "r15b", true);
