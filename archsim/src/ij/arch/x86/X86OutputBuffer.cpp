#include "ij/arch/x86/X86OutputBuffer.h"
#include "ij/arch/x86/X86Support.h"

using namespace archsim::ij::arch::x86;

void X86OutputBuffer::_xor(const X86Register& source, const X86Register& dest)
{
	reg2reg(0x30, source, dest);
}

void X86OutputBuffer::mov(const X86Immediate& source, const X86Register& dest)
{
	//imm2reg()
}

void X86OutputBuffer::mov(const X86Register& source, const X86Register& dest)
{
	reg2reg(0x80, source, dest);
}

void X86OutputBuffer::reg2reg(uint16_t op, const X86Register& src, const X86Register& dst)
{
	assert(src.GetSize() == dst.GetSize());

	if (src.GetSize() == 1)
		op |= 1;
}

void X86OutputBuffer::imm2reg(uint16_t op, const X86Immediate& src, const X86Register& dst)
{
	assert(src.GetSize() == dst.GetSize());
}
