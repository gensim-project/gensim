#ifndef INSTRUCTIONPRINTER_H
#define INSTRUCTIONPRINTER_H

#include <iostream>
#include <vector>
#include "RecordTypes.h"

namespace libtrace {

	class RecordIterator;
	class TracePacketStreamInterface;
	
	class InstructionPrinter
	{
	public:
		typedef std::vector<DataExtensionRecord> extension_list_t;
		
		InstructionPrinter();

		std::string operator()(TracePacketStreamInterface *stream);

		bool PrintInstruction(std::ostream &str, TracePacketStreamInterface *stream); 
		
		void SetDisplayNone()
		{
			_print_reg_read = _print_reg_write = _print_bank_read = _print_bank_write = _print_mem_read = _print_mem_write = 0;
		}
		
		void SetDisplayMem()
		{
			_print_mem_read = _print_mem_write = 1;
		}

		void SetDisplayAll()
		{
			_print_reg_read = _print_reg_write = _print_bank_read = _print_bank_write = _print_mem_read = _print_mem_write = 1;
		}
		
	private:
		bool PrintRegRead(std::ostream &str, RegReadRecord *rcd, const extension_list_t& extensions);
		bool PrintRegWrite(std::ostream &str, RegWriteRecord *rcd, const extension_list_t& extensions);
		bool PrintBankRegRead(std::ostream &str, BankRegReadRecord *rcd, const extension_list_t& extensions);
		bool PrintBankRegWrite(std::ostream &str, BankRegWriteRecord *rcd, const extension_list_t& extensions);

		bool PrintMemRead(std::ostream &str, RecordIterator &it, const extension_list_t& extensions);
		bool PrintMemWrite(std::ostream &str, RecordIterator &it, const extension_list_t& extensions);

		bool FormatData(std::ostream &str, uint32_t data_low, const extension_list_t &extensions);
		
		bool _print_reg_read, _print_reg_write, _print_bank_read, _print_bank_write, _print_mem_read, _print_mem_write;
	};

}

#endif
