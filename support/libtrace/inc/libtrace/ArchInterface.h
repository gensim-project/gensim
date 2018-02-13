#ifndef ARCHINTERFACE_H_
#define ARCHINTERFACE_H_

#include "RecordTypes.h"

#include <string>

namespace libtrace {

	class ArchInterface {
	public:
		virtual ~ArchInterface();
		
		virtual std::string DisassembleInstruction(const InstructionCodeRecord &record) = 0;
		
		virtual std::string GetRegisterSlotName(int index) = 0;
		virtual std::string GetRegisterBankName(int index) = 0;

		virtual uint32_t GetRegisterSlotWidth(int index) = 0;
		virtual uint32_t GetRegisterBankWidth(int index) = 0;
	};

	class DefaultArchInterface : public ArchInterface {
		virtual ~DefaultArchInterface();
		
		virtual std::string DisassembleInstruction(const InstructionCodeRecord &record) override;
		
		virtual std::string GetRegisterSlotName(int index) override;
		virtual std::string GetRegisterBankName(int index) override;
		
		virtual uint32_t GetRegisterSlotWidth(int index) override;
		virtual uint32_t GetRegisterBankWidth(int index) override;
	};

}

#endif
