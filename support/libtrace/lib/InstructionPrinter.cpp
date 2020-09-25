/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */
#include "libtrace/InstructionPrinter.h"
#include "libtrace/RecordIterator.h"
#include "libtrace/RecordTypes.h"
#include "libtrace/TraceRecordPacket.h"
#include "libtrace/TraceRecordStream.h"
#include "libtrace/TraceRecordPacketVisitor.h"
#include "libtrace/RecordReader.h"
#include "libtrace/disasm/Disassembler.h"

#include <cassert>
#include <iomanip>
#include <sstream>
#include <vector>

using namespace libtrace;

InstructionPrinter::InstructionPrinter() : _disasm(nullptr)
{
	SetDisplayAll();
}

std::string InstructionPrinter::operator()(TracePacketStreamInterface *stream)
{
	std::string output;
	if(!TryFormat(stream, output)) {
		return "(malformed instruction)";
	}
	return output;
}

bool InstructionPrinter::TryFormat(TracePacketStreamInterface* stream, std::string& output)
{
	std::stringstream str;
	if(!PrintInstruction(str, stream)) {
		return false;
	}
	output = str.str();
	return true;
}


class InstructionPrinterVisitor : public TraceRecordPacketVisitor
{
public:
	InstructionPrinterVisitor(std::ostream &target) : target_(target) {}

	void VisitBankRegRead(const BankRegReadReader& record) override
	{
		target_ << "(R[" << (uint32_t)record.GetBank() << "][" << (uint32_t)record.GetRegNum() <<"] -> " << record.GetValue() << ")";
	}
	void VisitBankRegWrite(const BankRegWriteReader& record) override
	{
		target_ << "(R[" << (uint32_t)record.GetBank() << "][" << (uint32_t)record.GetRegNum() <<"] <- " << record.GetValue() << ")";
	}
	void VisitRegRead(const RegReadReader& record) override
	{
		target_ << "(R[" << (uint32_t)record.GetIndex() << "] -> " << record.GetValue() << ")";
	}
	void VisitRegWrite(const RegWriteReader& record) override
	{
		target_ << "(R[" << (uint32_t)record.GetIndex() << "] <- " << record.GetValue() << ")";
	}

	void VisitInstructionCode(const InstructionCodeReader& record) override {}
	void VisitInstructionHeader(const InstructionHeaderReader& record) override {}

	void VisitMemReadAddr(const MemReadAddrReader& record) override
	{
		target_ << "(M[" << record.GetWidth() << "][" << record.GetAddress() << "]";
	}
	void VisitMemReadData(const MemReadDataReader& record) override
	{
		target_ << " => " << record.GetData() << ")";
	}
	void VisitMemWriteAddr(const MemWriteAddrReader& record) override
	{
		target_ << "(M[" << record.GetWidth() << "][" << record.GetAddress() << "]";
	}
	void VisitMemWriteData(const MemWriteDataReader& record) override
	{
		target_ << " <= " << record.GetData() << ")";
	}

private:
	std::ostream &target_;
};

bool InstructionPrinter::PrintInstruction(std::ostream& str, TracePacketStreamInterface* stream)
{
	TraceRecordPacket header_packet = stream->Get();
	TraceRecordPacket code_packet = stream->Get();

	if(header_packet.GetRecord().GetType() != InstructionHeader) {
		return false;
	}
	if(code_packet.GetRecord().GetType() != InstructionCode) {
		return false;
	}

	InstructionHeaderReader hdr  (*(InstructionHeaderRecord*)&header_packet.GetRecord(), header_packet.GetExtensions());
	InstructionCodeReader   code (*(InstructionCodeRecord*)&code_packet.GetRecord(), code_packet.GetExtensions());

	str << "[" << std::hex << std::setw(16) << std::setfill('0') << hdr.GetPC() << "] ";
	str << std::hex << std::setw(8) << std::setfill('0') << code.GetCode().AsU32() << " ";

	if (_disasm != nullptr) {
		str << std::left << std::setw(20) << std::setfill(' ') << _disasm->Disassemble(code) << " ";
	}

	while(stream->Good() && (stream->Peek().GetRecord().GetType() != InstructionHeader)) {
		TraceRecordPacket next_packet = stream->Get();

		InstructionPrinterVisitor ipv (str);
		ipv.Visit(next_packet);
	}

	return true;
}

bool InstructionPrinter::PrintRegRead(std::ostream &str, RegReadRecord* rec, const extension_list_t& extensions)
{
	assert(rec->GetType() == RegRead);
	if(!_print_reg_read) return true;


	return true;
}

bool InstructionPrinter::PrintRegWrite(std::ostream &str, RegWriteRecord* rec, const extension_list_t& extensions)
{
	assert(rec->GetType() == RegWrite);
	if(!_print_reg_write) return true;

	str << "(R[" << rec->GetRegNum() << "] <- 0x" << std::hex << std::setw(8) << std::setfill('0') << rec->GetData() << ")";
	return true;
}

bool InstructionPrinter::PrintBankRegRead(std::ostream &str, BankRegReadRecord *rec, const extension_list_t& extensions)
{
	if(!_print_bank_read) return true;
	str << "(R[" << (uint32_t)rec->GetBank() << "][" << (uint32_t)rec->GetRegNum() <<"] -> 0x" << std::hex << std::setw(8) << std::setfill('0') << rec->GetData() << ")";
	return true;
}

bool InstructionPrinter::PrintBankRegWrite(std::ostream &str, BankRegWriteRecord *rec, const extension_list_t& extensions)
{
	if(!_print_bank_write) return true;
	str << "(R[" << (uint32_t)rec->GetBank() << "][" << (uint32_t)rec->GetRegNum() <<"] <- 0x" << std::hex << std::setw(8) << std::setfill('0') << rec->GetData() << ")";
	return true;
}

bool InstructionPrinter::PrintMemRead(std::ostream &str, RecordIterator &it, const extension_list_t& addr_extensions)
{

	TraceRecord raddr = *it++;
	extension_list_t data_extensions;
	while((*it).GetType() == DataExtension) {
		TraceRecord tr = *it;
		data_extensions.push_back(*(DataExtensionRecord*)&tr);
		it++;
	}
	TraceRecord rdata = *it;

	if(!_print_mem_read) return true;

	MemReadAddrRecord *addr = (MemReadAddrRecord*)&raddr;
	MemReadDataRecord *data = (MemReadDataRecord*)&rdata;

	str << "([" << addr->GetAddress() << "](" << (uint32_t)addr->GetWidth() << ") => " << data->GetData() << ")";

	return true;
}
bool InstructionPrinter::PrintMemWrite(std::ostream &str, RecordIterator &it, const extension_list_t& addr_extensions)
{
	TraceRecord raddr = *it++;

	extension_list_t data_extensions;
	while((*it).GetType() == DataExtension) {
		TraceRecord tr = *it;
		data_extensions.push_back(*(DataExtensionRecord*)&tr);
		it++;
	}
	TraceRecord rdata = *it;

	if(!_print_mem_write) return true;

	MemWriteAddrRecord *addr = (MemWriteAddrRecord*)&raddr;
	MemWriteDataRecord *data = (MemWriteDataRecord*)&rdata;

	str << "([" << addr->GetAddress() << "](" << (uint32_t)addr->GetWidth() << ") <= " << data->GetData() << ")";
	return true;
}
