/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include <ostream>
#include <list>
#include <typeinfo>

#include "arch/ArchDescription.h"
#include "genC/Parser.h"

#include "genC/ir/IRAction.h"

#include "genC/ssa/SSABlock.h"
#include "genC/ssa/SSAContext.h"
#include "genC/ssa/SSAFormAction.h"
#include "genC/ssa/statement/SSAStatements.h"
#include "genC/ssa/SSASymbol.h"

#define __DEFINE_INTRINSIC_STRINGS
#include "genC/ssa/statement/SSAIntrinsicEnum.h"
#undef __DEFINE_INTRINSIC_STRINGS

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			void SSAStatement::PrettyPrint(std::ostringstream &str) const
			{
				if (HasValue())
					str << GetName() << " = ??? " << typeid(*this).name();
				else
					str << GetName() << ": ??? " << typeid(*this).name();
				if (IsFixed()) str << "(const)";
			}

			void SSAConstantStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << " = constant " << GetType().PrettyPrint() << " ";
				switch(Constant.Type()) {
					case IRConstant::Type_Integer:
						str << std::hex << Constant.Int() << " (const)";
						break;
					case IRConstant::Type_Float_Single:
						str << std::hex << Constant.Flt() << " (const)";
						break;
					case IRConstant::Type_Float_Double:
						str << std::hex << Constant.Dbl() << " (const)";
						break;
					case IRConstant::Type_Vector:
						str << "{}" << " (const)";
						break;
					default:
						throw std::logic_error("");
				}
			}

			void SSAReadStructMemberStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << "=" << Target()->GetName() << "->" << MemberName;
				if (IsFixed()) str << "(const)";
			}

			void SSAVariableReadStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << " = " << Target()->GetName();
				if (IsFixed()) str << " (const)";
				str << " " << GetType().GetCType();
			}

			void SSAVariableWriteStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << ": " << Target()->GetName() << " = " << Expr()->GetName();
				if (IsFixed())
					str << " (const), dominates: ";
				else
					str << ", dominates: ";
				std::list<SSAVariableReadStatement *> dominated_reads = Parent->Parent->GetDominatedReads(this);
				for (std::list<SSAVariableReadStatement *>::const_iterator ci = dominated_reads.begin(); ci != dominated_reads.end(); ++ci) {
					str << (*ci)->GetName() << " ";
				}
			}

			void SSACastStatement::PrettyPrint(std::ostringstream &str) const
			{
				if (GetCastType() == Cast_Reinterpret) {
					str << GetName() << " = reinterpret_cast<" << GetType().PrettyPrint() << ">(" << Expr()->GetName() << ")";
				} else {
					str << GetName() << " = (" << GetType().PrettyPrint() << ")" << Expr()->GetName();
				}

				if (IsFixed()) str << " (const)";
			}

			void SSABinaryArithmeticStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << " = " << LHS()->GetName();
				str << BinaryOperator::PrettyPrintOperator(Type);
				str << RHS()->GetName();
				if (IsFixed()) str << " (const)";
			}

			void SSAUnaryArithmeticStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << " = ";
				switch (Type) {
						using namespace SSAUnaryOperator;
					case OP_NEGATE:
						str << "!";
						break;
					case OP_NEGATIVE:
						str << "-";
						break;
					case OP_COMPLEMENT:
						str << "~";
						break;
					default:
						str << "?";
						break;
				}
				str << Expr()->GetName();
				if (IsFixed()) str << " (const)";
			}

			void SSAIntrinsicStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << " = " << IntrinsicNames[Type];

				// TODO: Print arguments?
			}

			void SSAMemoryReadStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << " = Load " << (uint32_t)Width << (Signed ? "s" : "") << " " << Addr()->GetName() << " => " << Target()->GetName();
			}

			void SSAMemoryWriteStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << ": Store " << (uint32_t)Width << " " << Addr()->GetName() << " <= " << Value()->GetName();
			}

			void SSARegisterStatement::PrettyPrint(std::ostringstream &str) const
			{
				if (IsRead) {
					if (!IsBanked) {
						str << GetName() << " = ReadReg " << (uint32_t)Bank << " (" << Parent->Parent->Arch->GetRegFile().GetSlotByIdx(Bank).GetIRType().PrettyPrint() << ")";
					} else {
						str << GetName() << " = ReadRegBank " << (uint32_t)Bank << ":" << RegNum()->GetName() << " (" << Parent->Parent->Arch->GetRegFile().GetBankByIdx(Bank).GetRegisterIRType().PrettyPrint() << ")";
					}
				} else {
					if (!IsBanked) {
						str << GetName() << ": WriteReg " << (uint32_t)Bank << " = " << Value()->GetName();
					} else {
						str << GetName() << ": WriteRegBank " << (uint32_t)Bank << ":" << RegNum()->GetName() << " = " << Value()->GetName();
					}
				}
			}

			void SSAJumpStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << ": Jump " << Target()->GetName() << (IsFixed() ? " (const)" : "");
			}

			void SSAIfStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << ": If " << Expr()->GetName() << ": Jump " << TrueTarget()->GetName() << " else " << FalseTarget()->GetName();
				if (IsFixed()) str << " (const)";
			}

			void SSASelectStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << ": Select " << Cond()->GetName() << " ? " << TrueVal()->GetName() << " : " << FalseVal()->GetName();
				if (IsFixed()) str << " (const)";
			}
			void SSASwitchStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << ": Switch " << Expr()->GetName() << ": ";
				str << "< <todo> >";
				str << " def " << Default()->GetName();
				if (IsFixed()) str << " (const)";
				str << " -> ";
				std::vector<const SSABlock *> targets = GetTargets();
				for (auto i : targets) {
					str << i->GetName() << ", ";
				}
			}

			void SSACallStatement::PrettyPrint(std::ostringstream &str) const
			{
				if (HasValue())
					str << GetName() << " = Call " << Target()->GetPrototype().GetIRSignature().GetName();
				else
					str << GetName() << ": Call " << Target()->GetPrototype().GetIRSignature().GetName();

				// todo: print args
			}

			void SSAReturnStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << ": Return";
				if (Value() != NULL) str << " " << Value()->GetName();
			}

			void SSARaiseStatement::PrettyPrint(std::ostringstream &str) const
			{
				str << GetName() << ": Raise";
			}

			void SSABitDepositStatement::PrettyPrint(std::ostringstream&str) const
			{
				str << GetName() << ": " << Expr()->GetName() << "[" << BitFrom()->GetName() << ":" << BitTo()->GetName() << "] = " << Value()->GetName();
			}

			void SSABitExtractStatement::PrettyPrint(std::ostringstream&str) const
			{
				str << GetName() << " [BIT EXTRACT]";
			}
		}
	}
}
