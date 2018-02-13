#include "libtrace/InstructionPrinter.h"
#include "libtrace/RecordIterator.h"
#include "libtrace/RecordTypes.h"
#include "libtrace/TraceRecordPacket.h"
#include "libtrace/TraceRecordStream.h"
#include "libtrace/TraceRecordPacketVisitor.h"
#include "libtrace/RecordReader.h"

#include <cassert>
#include <iomanip>
#include <sstream>
#include <vector>

using namespace libtrace;

class DataPrinter {
public:
	DataPrinter(uint32_t data_low, const InstructionPrinter::extension_list_t &extensions) : data_low_(data_low), extensions_(extensions) {
		
	}
	
	friend std::ostream& operator<< (std::ostream& stream, const DataPrinter& matrix) {
		stream << "0x" << std::hex << std::setw(8) << std::setfill('0');
		for(auto i : matrix.extensions_) {
			stream << i.GetData();
		}
		stream << matrix.data_low_;
		return stream;
	}
	
private:
	uint32_t data_low_;
	const InstructionPrinter::extension_list_t &extensions_;
};

InstructionPrinter::InstructionPrinter()
{
	SetDisplayAll();
}

std::string InstructionPrinter::operator()(TracePacketStreamInterface *stream)
{
	std::stringstream str;
	PrintInstruction(str, stream);
	return str.str();
}

class InstructionPrinterVisitor : public TraceRecordPacketVisitor {
public:
	InstructionPrinterVisitor(std::ostream &target) : target_(target) {}
	
	void VisitBankRegRead(const BankRegReadReader& record) override {
		target_ << "(R[" << (uint32_t)record.GetBank() << "][" << (uint32_t)record.GetRegNum() <<"] -> " << record.GetValue() << ")";
	}
	void VisitBankRegWrite(const BankRegWriteReader& record) override {
		target_ << "(R[" << (uint32_t)record.GetBank() << "][" << (uint32_t)record.GetRegNum() <<"] <- " << record.GetValue() << ")";
	}
	void VisitRegRead(const RegReadReader& record) override {
		target_ << "(R[" << (uint32_t)record.GetIndex() << "] -> " << record.GetValue() << ")";
	}
	void VisitRegWrite(const RegWriteReader& record) override {
		target_ << "(R[" << (uint32_t)record.GetIndex() << "] <- " << record.GetValue() << ")";
	}

	void VisitInstructionCode(const InstructionCodeReader& record) override {} 
	void VisitInstructionHeader(const InstructionHeaderReader& record) override {}

	void VisitMemReadAddr(const MemReadAddrReader& record) override {}
	void VisitMemReadData(const MemReadDataReader& record) override {}
	void VisitMemWriteAddr(const MemWriteAddrReader& record) override {}
	void VisitMemWriteData(const MemWriteDataReader& record) override {}

private:
	std::ostream &target_;
};

bool InstructionPrinter::PrintInstruction(std::ostream& str, TracePacketStreamInterface* stream)
{
	TraceRecordPacket header_packet = stream->Get();
	TraceRecordPacket code_packet = stream->Get();
	
	assert(header_packet.GetRecord().GetType() == InstructionHeader);
	assert(code_packet.GetRecord().GetType() == InstructionCode);
	
	InstructionHeaderReader hdr  (*(InstructionHeaderRecord*)&header_packet.GetRecord(), header_packet.GetExtensions());
	InstructionCodeReader   code (*(InstructionCodeRecord*)&code_packet.GetRecord(), code_packet.GetExtensions());
	
	str << "[" << std::hex << std::setw(8) << std::setfill('0') << hdr.GetPC().AsU32() << "] " << std::hex << std::setw(8) << std::setfill('0') << code.GetCode().AsU32() << " ";
	
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

bool InstructionPrinter::PrintBankRegRead(std::ostream &str, BankRegReadRecord *rec, const extension_list_t& extensions){
	if(!_print_bank_read) return true;
	str << "(R[" << (uint32_t)rec->GetBank() << "][" << (uint32_t)rec->GetRegNum() <<"] -> 0x" << std::hex << std::setw(8) << std::setfill('0') << rec->GetData() << ")";
	return true;
}

bool InstructionPrinter::PrintBankRegWrite(std::ostream &str, BankRegWriteRecord *rec, const extension_list_t& extensions){
	if(!_print_bank_write) return true;
	str << "(R[" << (uint32_t)rec->GetBank() << "][" << (uint32_t)rec->GetRegNum() <<"] <- 0x" << std::hex << std::setw(8) << std::setfill('0') << rec->GetData() << ")";
	return true;
}

bool InstructionPrinter::PrintMemRead(std::ostream &str, RecordIterator &it, const extension_list_t& addr_extensions){
	
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
	
	str << "([" << DataPrinter(addr->GetAddress(), addr_extensions) << "](" << (uint32_t)addr->GetWidth() << ") => " << DataPrinter(data->GetData(), data_extensions) << ")";
	
	return true;
}
bool InstructionPrinter::PrintMemWrite(std::ostream &str, RecordIterator &it, const extension_list_t& addr_extensions){	
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
	
	str << "([" << DataPrinter(addr->GetAddress(), addr_extensions) << "](" << (uint32_t)addr->GetWidth() << ") <= " << DataPrinter(data->GetData(), data_extensions) << ")";
	return true;
}

