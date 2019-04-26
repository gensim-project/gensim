/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IR.h"
#include "genC/ir/IRAction.h"

#include "Util.h"
#include "genC/Parser.h"

namespace gensim
{
	namespace genc
	{

		namespace BinaryOperator
		{
			std::string PrettyPrintOperator(BinaryOperator::EBinaryOperator op)
			{
				switch (op) {
					case Set:
						return "=";
					case Logical_Or:
						return "||";
					case Logical_And:
						return "&&";
					case Bitwise_Or:
						return "|";
					case Bitwise_And:
						return "&";
					case Bitwise_XOR:
						return "^";
					case Equality:
						return "==";
					case Inequality:
						return "!=";
					case LessThan:
						return "<";
					case GreaterThan:
						return ">";
					case LessThanEqual:
						return "<=";
					case GreaterThanEqual:
						return ">=";
					case ShiftLeft:
						return "<<";
					case ShiftRight:
						return ">>";
					case SignedShiftRight:
						return "->>";
					case Add:
						return "+";
					case Subtract:
						return "-";
					case Multiply:
						return "*";
					case Divide:
						return "/";
					case Modulo:
						return "%";
					case RotateLeft:
						return "<<<";
					case RotateRight:
						return ">>>";
					default:
						return "<?>";
				}
			}
		}

		void GenCContext::PrettyPrint(std::ostringstream &out) const
		{
			for (std::map<std::string, IRHelperAction *>::const_iterator ci = HelperTable.begin(); ci != HelperTable.end(); ++ci) {
				ci->second->PrettyPrint(out);
			}

			for (std::map<std::string, IRExecuteAction *>::const_iterator ci = ExecuteTable.begin(); ci != ExecuteTable.end(); ++ci) {
				ci->second->PrettyPrint(out);
			}
		}

		void IRType::PrettyPrint(std::ostringstream &out) const
		{
			out << GetUnifiedType();
		}

		std::string IRType::PrettyPrint() const
		{
			std::ostringstream str;
			PrettyPrint(str);
			return str.str();
		}

		std::string IRStatement::Dump() const
		{
			std::ostringstream str;
			PrettyPrint(str);
			return str.str();
		}

		void IRStatement::PrettyPrint(std::ostringstream &out) const
		{
			out << "<?>" << std::endl;
		}

		void IRStatement::PrettyPrint() const
		{
			std::ostringstream str;
			PrettyPrint(str);
			std::cout << str.str() << std::endl;
		}

		void EmptyExpression::PrettyPrint(std::ostringstream &out) const {}

		void IRTernaryExpression::PrettyPrint(std::ostringstream &out) const
		{
			out << "(";
			Cond->PrettyPrint(out);
			out << " ? ";
			Left->PrettyPrint(out);
			out << ":";
			Right->PrettyPrint(out);
			out << ")";
		}

		void IRUnaryExpression::PrettyPrint(std::ostringstream &out) const
		{

			out << "(";
			switch (Type) {
					using namespace IRUnaryOperator;
				case Dereference:
					out << "*";
					break;
				case Positive:
					out << "+";
					break;
				case Negative:
					out << "-";
					break;
				case Complement:
					out << "~";
					break;
				case Negate:
					out << "!";
					break;
				case Preincrement:
					out << "++";
					break;
				case Predecrement:
					out << "--";
					break;
				case Postincrement:
				case Postdecrement:
				case Index:
				case Member:
				case Ptr:
					break;
				default:
					out << "<?>";
					break;
			}

			BaseExpression->PrettyPrint(out);

			switch (Type) {
					using namespace IRUnaryOperator;
				case Postincrement:
					out << "++";
					break;
				case Postdecrement:
					out << "--";
					break;
				case Index:
					out << "[";
					Arg->PrettyPrint(out);
					out << "]";
					break;
				case Sequence:
					out << "[";
					Arg->PrettyPrint(out);
					out << ":";
					Arg2->PrettyPrint(out);
					out << "]";
					break;
				case Member:
					out << "." << MemberStr;
					break;
				case Ptr:
					out << "->" << MemberStr;
					break;
			}

			out << ")";
		}

		void IRBinaryExpression::PrettyPrint(std::ostringstream &out) const
		{
			out << "(";
			Left->PrettyPrint(out);
			out << PrettyPrintOperator(Type);
			Right->PrettyPrint(out);
			out << ")";
		}

		void IRCallExpression::PrettyPrint(std::ostringstream &out) const
		{
			out << TargetName << "(";
			for (std::vector<IRExpression *>::const_iterator ci = Args.begin(); ci != Args.end(); ++ci) {
				(*ci)->PrettyPrint(out);
				if (ci + 1 != Args.end()) out << ",";
			}

			out << ")";
		}

		void IRCastExpression::PrettyPrint(std::ostringstream &out) const
		{
			out << "(";
			ToType.PrettyPrint(out);
			out << ")";
			Expr->PrettyPrint(out);
		}

		void IRVectorExpression::PrettyPrint(std::ostringstream& out) const
		{
			out << "{}";
		}

		void IRConstExpression::PrettyPrint(std::ostringstream &out) const
		{
			if (Type.BaseType.PlainOldDataType > IRPlainOldDataType::INT64)
				out << "(" << GetValue().Dbl() << " : ";
			else
				out << "(" << GetValue().Int() << " : ";
			Type.PrettyPrint(out);
			out << ")";
		}

		void IRVariableExpression::PrettyPrint(std::ostringstream &out) const
		{
			out << "(" << name_;  // << ":";
			// Scope.GetSymbolType(VariableName)->PrettyPrint(out);
			out << ")";
		}

		void IRDefineExpression::PrettyPrint(std::ostringstream &out) const
		{
			type.PrettyPrint(out);
			out << " " << name;
		}

		void IRExpressionStatement::PrettyPrint(std::ostringstream &out) const
		{
			Expr->PrettyPrint(out);
			out << ";";
		}

		void IRSelectionStatement::PrettyPrint(std::ostringstream &out) const
		{
			switch (Type) {
				case SELECT_SWITCH: {
					out << "switch(";
					Expr->PrettyPrint(out);
					out << ")";
					Body->PrettyPrint(out);
					break;
				}
				case SELECT_IF: {
					out << "if(";
					Expr->PrettyPrint(out);
					out << ")";
					Body->PrettyPrint(out);
					if (ElseBody) {
						out << "else";
						ElseBody->PrettyPrint(out);
					}
					break;
				}
				default:
					out << "<?>";
			}
		}

		void IRIterationStatement::PrettyPrint(std::ostringstream &out) const
		{
			switch (Type) {
				case ITERATE_FOR: {
					out << "for(";
					For_Expr_Start->PrettyPrint(out);
					out << ";";
					For_Expr_Check->PrettyPrint(out);
					out << ";";
					Expr->PrettyPrint(out);
					out << ")";
					Body->PrettyPrint(out);
					break;
				}
				case ITERATE_DO_WHILE: {
					out << "do ";
					Body->PrettyPrint(out);
					out << " while(";
					Expr->PrettyPrint(out);
					out << ")";
					break;
				}
				case ITERATE_WHILE: {
					out << "while(";
					Expr->PrettyPrint(out);
					out << ")";
					Body->PrettyPrint(out);
					break;
				}

				default:
					out << "<?>";
			}
		}

		void IRFlowStatement::PrettyPrint(std::ostringstream &out) const
		{
			switch (Type) {
				case FLOW_CASE: {
					out << "case ";
					Expr->PrettyPrint(out);
					out << ":";
					Body->PrettyPrint(out);
					break;
				}
				case FLOW_DEFAULT: {
					out << "default:";
					Body->PrettyPrint(out);
					break;
				}
				case FLOW_BREAK: {
					out << "break;";
					break;
				}
				case FLOW_CONTINUE: {
					out << "continue;";
					break;
				}
				case FLOW_RETURN_VOID: {
					out << "return;";
					break;
				}
				case FLOW_RETURN_VALUE: {
					out << "return ";
					Expr->PrettyPrint(out);
					out << ";";
					break;
				}
			}
		}

		void IRBody::PrettyPrint(std::ostringstream &out) const
		{
			out << "{";
			for (std::vector<IRStatement *>::const_iterator ci = Statements.begin(); ci != Statements.end(); ++ci) {
				(*ci)->PrettyPrint(out);
				out << "\n";
			}
			out << "}";
		}

		void IRAction::PrettyPrint(std::ostringstream &out) const
		{
			PrettyPrintHeader(out);
			if (body != NULL) body->PrettyPrint(out);
			out << "\n";
		}

		void IRHelperAction::PrettyPrintHeader(std::ostringstream &out) const
		{
			GetSignature().GetType().PrettyPrint(out);
			out << " " << GetSignature().GetName() << "(";
			for(const auto param : GetSignature().GetParams()) {
				out << param.GetType().PrettyPrint()<< " " << param.GetName() << ", ";
			}
			out << ")";

		}

		void IRIntrinsicAction::PrettyPrintHeader(std::ostringstream& out) const
		{
			out << "???";
		}

		void IRExecuteAction::PrettyPrintHeader(std::ostringstream &out) const
		{
			out << "execute(" << GetSignature().GetName() << ")";
		}
	}
}
