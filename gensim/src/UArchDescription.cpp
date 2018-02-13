/*
 * File:   UArchDescription.cpp
 * Author: s0803652
 *
 * Created on 27 February 2012, 14:58
 */

#include <iostream>

#include "UArchDescription.h"

#include "archCuArch/archCuArchLexer.h"
#include "archCuArch/archCuArchParser.h"

#include "antlr-ver.h"

namespace gensim
{
	namespace uarch
	{
		namespace expression
		{

			UArchExpression_Unary::UArchExpression_Unary(UArchExpressionOperator _Op, UArchExpression *_Expr) : Expr(_Expr), Operator(_Op) {}

			UArchExpression_Binary::UArchExpression_Binary(UArchExpressionOperator _Operator, UArchExpression *_Left, UArchExpression *_Right) : Operator(_Operator), Left(_Left), Right(_Right) {}

			UArchExpression_Function::UArchExpression_Function(std::string _Function) : Function(_Function) {}

			UArchExpression_Property::UArchExpression_Property(std::string _Parent, std::string _Child) : Parent(_Parent), Child(_Child) {}

			UArchExpression_Index::UArchExpression_Index(std::string _Parent, UArchExpression *_Child) : Parent(_Parent), Index(_Child) {}

			UArchExpression_ID::UArchExpression_ID(std::string _ID) : Name(_ID) {}

			UArchExpression_Const::UArchExpression_Const(uint32_t _Const) : Const(_Const) {}

			UArchExpression::~UArchExpression()
			{
				switch (NodeType) {
					case UARCH_UNARY_OPERATOR:
						delete Node.Unary;
						break;
					case UARCH_BINARY_OPERATOR:
						delete Node.Binary;
						break;
					case UARCH_FUNCTION:
						delete Node.Function;
						break;
					case UARCH_PROPERTY:
						delete Node.Property;
						break;
					case UARCH_INDEX:
						delete Node.Index;
						break;
					case UARCH_ID:
						delete Node.ID_Node;
						break;
					case UARCH_CONST:
						delete Node.Const;
						break;
				}
			}

			UArchExpression *UArchExpression::Parse(pANTLR3_BASE_TREE node)
			{
				UArchExpression *r = new UArchExpression();
				switch (node->getType(node)) {
					case MULT_OP:
					case ADDITION_OP:
					case SHIFT_OP:
					case REL_OP:
					case EQ_OP:
					case BITAND:
					case CARET:
					case BITOR:
					case LOGAND:
					case LOGOR: {
						assert(node->getChildCount(node) == 2);
						pANTLR3_BASE_TREE left = (pANTLR3_BASE_TREE)node->getChild(node, 0);
						pANTLR3_BASE_TREE right = (pANTLR3_BASE_TREE)node->getChild(node, 1);
						r->NodeType = UARCH_BINARY_OPERATOR;
						r->Node.Binary = new UArchExpression_Binary(ParseOperator(node), Parse(left), Parse(right));
						break;
					}

					case INT: {
						assert(node->getChildCount(node) == 0);
						r->NodeType = UARCH_CONST;

						uint32_t val = atoi((char *)(node->getText(node)->chars));
						r->Node.Const = new UArchExpression_Const(val);
						break;
					}

					case ID: {
						assert(node->getChildCount(node) == 0);
						r->NodeType = UARCH_ID;

						r->Node.ID_Node = new UArchExpression_ID((char *)(node->getText(node)->chars));
						break;
					}

					case FUNC: {
						assert(node->getChildCount(node) >= 1);
						r->NodeType = UARCH_FUNCTION;

						pANTLR3_BASE_TREE name_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);

						r->Node.Function = new UArchExpression_Function((char *)(name_node->getText(name_node)->chars));
						for (uint16_t i = 1; i < node->getChildCount(node); ++i) {
							r->Node.Function->Arguments.push_back(Parse((pANTLR3_BASE_TREE)node->getChild(node, i)));
						}
						break;
					}

					case PROPERTY: {
						assert(node->getChildCount(node) == 2);
						r->NodeType = UARCH_PROPERTY;

						pANTLR3_BASE_TREE parent_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
						pANTLR3_BASE_TREE child_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);

						r->Node.Property = new UArchExpression_Property((char *)(parent_node->getText(parent_node)->chars), (char *)(child_node->getText(child_node)->chars));
						break;
					}

					case OBRACKET: {
						assert(node->getChildCount(node) == 2);
						r->NodeType = UARCH_INDEX;

						pANTLR3_BASE_TREE parent_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
						pANTLR3_BASE_TREE child_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);

						r->Node.Index = new UArchExpression_Index((char *)(parent_node->getText(parent_node)->chars), Parse((pANTLR3_BASE_TREE)(child_node->getText(child_node))));
						break;
					}
				}
				return r;
			}

			UArchExpressionOperator UArchExpression::ParseOperator(pANTLR3_BASE_TREE node)
			{
				char *node_text = (char *)node->getText(node)->chars;
				switch (node_text[0]) {
					case '!': {
						if (node_text[1] == '=')
							return UARCH_NE;
						else
							return UARCH_NOT;
					}
					case '<': {
						if (node_text[1] == '<') return UARCH_LSHIFT;
						if (node_text[1] == '=') return UARCH_LE;
						return UARCH_LT;
					}
					case '>': {
						if (node_text[1] == '>') return UARCH_RSHIFT;
						if (node_text[1] == '=') return UARCH_GE;
						return UARCH_GT;
					}

					case '|': {
						if (node_text[1] == '|') return UARCH_L_OR;
						return UARCH_B_OR;
					}
					case '&': {
						if (node_text[1] == '&') return UARCH_L_AND;
						return UARCH_B_AND;
					}
					case '^':
						return UARCH_B_XOR;

					case '=':
						return UARCH_EQ;

					case '+':
						return UARCH_PLUS;
					case '-':
						return UARCH_MINUS;
					case '*':
						return UARCH_MULTIPLY;
					case '/':
						return UARCH_DIVIDE;
					case '%':
						return UARCH_MODULO;
				}
				assert(false);
				UNEXPECTED;
			}

			UArchExpression *UArchExpression::CreateConst(uint32_t c)
			{
				UArchExpression *expr = new gensim::uarch::expression::UArchExpression();
				expr->NodeType = gensim::uarch::expression::UARCH_CONST;
				expr->Node.Const = new gensim::uarch::expression::UArchExpression_Const(c);

				return expr;
			}

			std::string UArchExpression::Print()
			{
				std::ostringstream oss;

				switch (NodeType) {
					case UARCH_CONST:
						oss << Node.Const->Const;
						break;
					case UARCH_ID:
						oss << Node.ID_Node->Name;
						break;
					case UARCH_PROPERTY:
						oss << Node.Property->Parent << "." << Node.Property->Child;
						break;
					case UARCH_FUNCTION:
						oss << Node.Function->Function << "(";
						if (Node.Function->Function.size() >= 1) {
							oss << Node.Function->Arguments[0]->Print();
							for (std::vector<UArchExpression *>::iterator i = ++(Node.Function->Arguments.begin()); i != Node.Function->Arguments.end(); ++i) {
								oss << ", " << (*i)->Print();
							}
						}
						oss << ")";
						break;
					case UARCH_BINARY_OPERATOR:
						oss << "(" << Node.Binary->Left->Print();
						oss << PrintOperator(Node.Binary->Operator);
						oss << Node.Binary->Right->Print() << ")";
						break;
					case UARCH_UNARY_OPERATOR:
						oss << PrintOperator(Node.Unary->Operator) << Node.Unary->Expr->Print();
						break;
					default:
						assert(false);
						UNEXPECTED;
				}
				return oss.str();
			}

			std::string UArchExpression::PrintOperator(UArchExpressionOperator op)
			{
				switch (op) {
					case UARCH_NOT:
						return "!";
					case UARCH_COMPLEMENT:
						return "~";
					case UARCH_PLUS:
						return "+";
					case UARCH_MINUS:
						return "-";
					case UARCH_MULTIPLY:
						return "*";
					case UARCH_DIVIDE:
						return "/";
					case UARCH_MODULO:
						return "%";
					case UARCH_LSHIFT:
						return "<<";
					case UARCH_RSHIFT:
						return ">>";
					case UARCH_GT:
						return ">";
					case UARCH_GE:
						return ">=";
					case UARCH_LT:
						return "<";
					case UARCH_LE:
						return "<=";
					case UARCH_EQ:
						return "==";
					case UARCH_NE:
						return "!=";
					case UARCH_B_AND:
						return "&";
					case UARCH_B_OR:
						return "|";
					case UARCH_B_XOR:
						return "^";
					case UARCH_L_AND:
						return "&&";
					case UARCH_L_OR:
						return "||";
					default:
						assert(false);
						UNEXPECTED;
				}
			}

		}  // namespace expression

		PredicatedLatency::PredicatedLatency(gensim::uarch::expression::UArchExpression *Predicate, gensim::uarch::expression::UArchExpression *Latency)
		{
			this->Predicate = Predicate;
			this->Latency = Latency;
		}

		void PipelineDetails::AddDestReg(std::string dest)
		{
			Dest_Single_Regs.push_back(dest);
		}

		void PipelineDetails::AddDestBankedReg(std::string bank, gensim::uarch::expression::UArchExpression *reg)
		{
			Dest_Banked_Regs.insert(std::pair<std::string, expression::UArchExpression *>(bank, reg));
		}

		void PipelineDetails::AddSourceReg(std::string dest)
		{
			Source_Single_Regs.push_back(dest);
		}

		void PipelineDetails::AddSourceBankedReg(std::string bank, gensim::uarch::expression::UArchExpression *reg)
		{
			Source_Banked_Regs.insert(std::pair<std::string, gensim::uarch::expression::UArchExpression *>(bank, reg));
		}

		UArchDescription::UArchDescription(isa::ISADescription &_ISA) : ISA(_ISA) {}

		UArchDescription::UArchDescription(const char *filename, isa::ISADescription &_ISA) : ISA(_ISA), Type(UArchType::InOrder)
		{
			ParseUArchDescription(filename);
		}

		UArchDescription::~UArchDescription() {}

		bool UArchDescription::Validate(std::stringstream &errorstream) const
		{
			bool valid = true;

			for (std::map<std::string, InstructionDetails *>::const_iterator inst = Details.begin(); inst != Details.end(); ++inst) {

				for (std::map<uint32_t, PipelineDetails *>::const_iterator stage = inst->second->Pipeline.begin(); stage != inst->second->Pipeline.end(); ++stage) {
					PipelineDetails &details = *(stage->second);
					std::string error;

					if (details.default_latency != NULL && !CheckValidLatencyExpression(*details.default_latency, inst->first, error)) {
						valid = false;
						errorstream << error;
					}

					for (std::vector<PredicatedLatency>::const_iterator pred_latency = details.predicated_latency.begin(); pred_latency != details.predicated_latency.end(); ++pred_latency) {
						if (!CheckValidLatencyExpression(*(pred_latency->Predicate), inst->first, error)) {
							valid = false;
							errorstream << "Error in " << inst->first << " predicated latency predicate: " << error << std::endl;
						}
						if (!CheckValidLatencyExpression(*(pred_latency->Latency), inst->first, error)) {
							valid = false;
							errorstream << "Error in " << inst->first << " predicated latency: " << error << std::endl;
						}
					}

					for (std::map<std::string, gensim::uarch::expression::UArchExpression *>::const_iterator reg = details.Dest_Banked_Regs.begin(); reg != details.Dest_Banked_Regs.end(); ++reg) {
						if (!CheckValidLatencyExpression(*reg->second, inst->first, error)) {
							valid = false;
							errorstream << "Error in " << inst->first << " destination regs: " << error << std::endl;
						}
					}

					for (std::map<std::string, gensim::uarch::expression::UArchExpression *>::const_iterator reg = details.Source_Banked_Regs.begin(); reg != details.Source_Banked_Regs.end(); ++reg) {
						if (!CheckValidLatencyExpression(*reg->second, inst->first, error)) {
							valid = false;
							errorstream << "Error in " << inst->first << " source regs: " << error << std::endl;
						}
					}
				}
			}

			return valid;
		}

		bool UArchDescription::CheckValidLatencyExpression(const gensim::uarch::expression::UArchExpression &expr, const std::string &group, std::string &error) const
		{
			using namespace gensim::uarch::expression;

			switch (expr.NodeType) {
				case UARCH_ID:
					return true;
				case UARCH_CONST:
					return true;
				case UARCH_UNARY_OPERATOR:
					return CheckValidLatencyExpression(*expr.Node.Unary->Expr, group, error);
				case UARCH_BINARY_OPERATOR:
					return CheckValidLatencyExpression(*expr.Node.Binary->Left, group, error) && CheckValidLatencyExpression(*expr.Node.Binary->Right, group, error);

				// assume all function calls are valid
				case UARCH_FUNCTION:
					return true;

				// a property reference is only valid if it is a) for an instruction and b) for a field which exists in the instruction or all of the instructions in the group
				case UARCH_PROPERTY: {
					if (expr.Node.Property->Parent != "inst") {
						error = "Unknown parent";
						return false;
					}

					std::string child = expr.Node.Property->Child;

					// If we are dealing with an instruction group (and not an individual instruction)
					if (hasInstructionGroup(group)) {
						const std::list<const isa::InstructionDescription *> &group_list = Groups.at(group);
						for (std::list<const isa::InstructionDescription *>::const_iterator i = group_list.begin(); i != group_list.end(); ++i) {
							if (!(*i)->Format->hasChunk(child)) {
								error = (*i)->Name + " does not have field " + child;
								return false;
							}
						}
						return true;
					} else {
						if (!ISA.Instructions.at(group)->Format->hasChunk(child)) {
							error = group + " does not have field " + child;
							return false;
						}
						return true;
					}
				}
				default:
					error = "Unrecognized expression type";
					return false;
			}
		}

		void print_node(pANTLR3_BASE_TREE node, int depth)
		{
			uint32_t cdepth = depth;
			while (cdepth) {
				printf("\t");
				cdepth--;
			}
			printf("%s\n", node->getText(node)->chars);

			for (uint32_t i = 0; i < node->getChildCount(node); i++) {
				print_node((pANTLR3_BASE_TREE)(node->getChild(node, i)), depth + 1);
			}
		}

		void UArchDescription::ParseUArchDescription(const char *const filename)
		{
			struct stat sts;
			if (stat(filename, &sts) == -1) {
				fprintf(stderr, "Could not find uArch description %s\n", filename);
				exit(1);
			}

			pANTLR3_UINT8 fname_str = (pANTLR3_UINT8)filename;
			pANTLR3_INPUT_STREAM pts = antlr3AsciiFileStreamNew(fname_str);
			parchCuArchLexer lexer = archCuArchLexerNew(pts);
			pANTLR3_COMMON_TOKEN_STREAM tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
			parchCuArchParser parser = archCuArchParserNew(tstream);

			archCuArchParser_uarch_return uarch = parser->uarch(parser);

			if (parser->pParser->rec->getNumberOfSyntaxErrors(parser->pParser->rec) > 0 || lexer->pLexer->rec->getNumberOfSyntaxErrors(lexer->pLexer->rec)) {
				fprintf(stderr, "Syntax errors detected in uArch Description (%s)\n", filename);
				exit(1);
			}

			pANTLR3_COMMON_TREE_NODE_STREAM nodes = antlr3CommonTreeNodeStreamNewTree(uarch.tree, ANTLR3_SIZE_HINT);
			pANTLR3_BASE_TREE root = nodes->root;

			bool success = true;

			// Root node is ac_uarch
			for (uint16_t i = 0; i < root->getChildCount(root); ++i) {
				pANTLR3_BASE_TREE node = (pANTLR3_BASE_TREE)(root->getChild(root, i));
				switch (node->getType(node)) {
					case AC_UARCH_TYPE:
						success &= ParseUArchType(node);
						break;
					case AC_PIPELINE_STAGE:
						success &= ParsePipelineStage(node);
						break;
					case AC_FUNC_UNIT:
						success &= ParseFuncUnit(node);
						break;
					case AC_UNIT_TYPE:
						success &= ParseUnitType(node);
						break;
					case AC_GROUP:
						success &= ParseGroup(node);
						break;
					case SET_GROUP:
						success &= ParseSetGroup(node);
						break;
					case SET_SRC:
						success &= ParseSetSrc(node);
						break;
					case SET_DEST:
						success &= ParseSetDest(node);
						break;
					case SET_LATENCY_CONST:
						success &= ParseSetLatencyConst(node);
						break;
					case SET_LATENCY_PRED:
						success &= ParseSetLatencyPred(node);
						break;
					case SET_FLUSH_PIPELINE:
						success &= ParseFlushPipeline(node);
						break;
					case SET_DEFAULT_UNIT:
						success &= ParseSetDefaultUnit(node);
						break;
					case SET_FUNC_UNIT:
						success &= ParseSetFuncUnit(node);
						break;

					default:
						assert(false && "Encountered invalid node\n");
				}
			}
			std::stringstream str;
			Validate(str);
			std::cout << str.str();

			nodes->free(nodes);
			parser->free(parser);
			tstream->free(tstream);
			lexer->free(lexer);
			pts->free(pts);
		}

		bool UArchDescription::ParseUArchType(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == AC_UARCH_TYPE);
			assert(node->getChildCount(node) == 1);

			pANTLR3_BASE_TREE type_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);

			switch (type_node->getType(type_node)) {
				case UARCH_TYPE_INORDER:
					Type = UArchType::InOrder;
					fprintf(stderr, "uArch type set to In-Order\n");
					break;
				case UARCH_TYPE_SUPERSCALAR:
					Type = UArchType::Superscalar;
					;
					fprintf(stderr, "uArch type set to Superscalar\n");
					break;
				case UARCH_TYPE_OUTOFORDER:
					Type = UArchType::OutOfOrder;
					;
					fprintf(stderr, "uArch type set to Out-of-Order\n");
					break;
				default:
					fprintf(stderr, "uArch error: Unknown uArch type %s (line %u)\n", (char *)(type_node->getText(type_node)->chars), type_node->getLine(type_node));
					return false;
			}

			return true;
		}

		bool UArchDescription::ParseUnitType(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == AC_UNIT_TYPE);

			for (uint32_t i = 0; i < node->getChildCount(node); ++i) {
				pANTLR3_BASE_TREE type_node = (pANTLR3_BASE_TREE)node->getChild(node, i);
				AddUnitType((char *)(type_node->getText(type_node)->chars));
			}

			return true;
		}

		bool UArchDescription::ParseFuncUnit(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == AC_FUNC_UNIT);
			assert(node->getChildCount(node) > 1);

			pANTLR3_BASE_TREE stage_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
			uint8_t stage_idx = getPipelineStageIndex((char *)stage_node->getText(stage_node)->chars);
			PipelineStage &stage = Pipeline_Stages[stage_idx];

			for (uint32_t i = 1; i < node->getChildCount(node); ++i) {
				pANTLR3_BASE_TREE unit_node = (pANTLR3_BASE_TREE)node->getChild(node, i);
				std::string unit_type = (char *)(unit_node->getText(unit_node)->chars);
				stage.AddFuncUnit(GetUnitType(unit_type));
			}

			return true;
		}

		bool UArchDescription::ParseSetDefaultUnit(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == SET_DEFAULT_UNIT);
			assert(node->getChildCount(node) == 2);

			pANTLR3_BASE_TREE stage_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
			uint8_t stage_idx = getPipelineStageIndex((char *)stage_node->getText(stage_node)->chars);
			PipelineStage &stage = Pipeline_Stages[stage_idx];

			pANTLR3_BASE_TREE unit_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);
			std::string unit_type = (char *)(unit_node->getText(unit_node)->chars);

			stage.SetDefaultUnit(GetUnitType(unit_type));

			return true;
		}

		bool UArchDescription::ParseSetFuncUnit(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == SET_FUNC_UNIT);
			assert(node->getChildCount(node) == 3);

			pANTLR3_BASE_TREE insn_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
			std::string insn_name = (char *)(insn_node->getText(insn_node)->chars);

			if (!hasInstructionGroup(insn_name) && !ISA.hasInstruction(insn_name)) {
				fprintf(stderr, "uArch error: Could not find instruction or group %s (%u)\n", insn_name.c_str(), node->getLine(node));
				return false;
			}

			pANTLR3_BASE_TREE stage_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);

			uint8_t stage_idx = getPipelineStageIndex((char *)stage_node->getText(stage_node)->chars);

			pANTLR3_BASE_TREE unit_node = (pANTLR3_BASE_TREE)node->getChild(node, 2);
			std::string unit_type = (char *)(unit_node->getText(unit_node)->chars);

			PipelineDetails &pd = GetOrCreatePipelineDetails(insn_name, stage_idx);
			pd.SetFuncUnit(GetUnitType(unit_type));

			return true;
		}

		bool UArchDescription::ParsePipelineStage(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == AC_PIPELINE_STAGE);
			assert(node->getChildCount(node) == 2);

			pANTLR3_BASE_TREE name_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
			pANTLR3_BASE_TREE latency_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);

			std::string name = (char *)(name_node->getText(name_node)->chars);
			uint32_t latency = atoi((char *)(name_node->getText(latency_node)->chars));

			if (hasPipelineStage(name)) {
				fprintf(stderr, "uArch error: Duplicate pipeline stage %s (%u)\n", name.c_str(), node->getLine(node));
				return false;
			}

			Pipeline_Stages.push_back(PipelineStage(name, latency));
			return true;
		}

		bool UArchDescription::ParseGroup(pANTLR3_BASE_TREE const node)
		{
			assert(node->getType(node) == AC_GROUP);
			assert(node->getChildCount(node) == 1);

			pANTLR3_BASE_TREE name_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);

			std::string name = (char *)(name_node->getText(name_node)->chars);

			if (hasInstructionGroup(name)) {
				fprintf(stderr, "uArch error: Duplicate instruction group %s (%u)\n", name.c_str(), node->getLine(node));
				return false;
			}

			if (ISA.hasInstruction(name)) {
				fprintf(stderr, "uArch error: Group shares name with instruction %s (%u)\n", name.c_str(), node->getLine(node));
				return false;
			}

			Groups[name] = std::list<const isa::InstructionDescription *>();
			return true;
		}

		bool UArchDescription::ParseSetGroup(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == SET_GROUP);
			assert(node->getChildCount(node) == 2);

			pANTLR3_BASE_TREE insn_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
			pANTLR3_BASE_TREE group_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);

			std::string insn_name = (char *)(insn_node->getText(insn_node)->chars);
			std::string group_name = (char *)(group_node->getText(group_node)->chars);

			if (!hasInstructionGroup(group_name)) {
				fprintf(stderr, "uArch error: Unknown group %s (%u)\n", group_name.c_str(), node->getLine(node));
				return false;
			}

			if (!ISA.hasInstruction(insn_name)) {
				fprintf(stderr, "uArch error: Unknown instruction %s (%u)\n", insn_name.c_str(), node->getLine(node));
				return false;
			}

			Groups[group_name].push_back(ISA.Instructions.at(insn_name));

			return true;
		}

		bool UArchDescription::ParseSetSrc(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == SET_SRC);
			assert(node->getChildCount(node) == 3 || node->getChildCount(node) == 4);

			pANTLR3_BASE_TREE insn_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
			pANTLR3_BASE_TREE stage_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);
			pANTLR3_BASE_TREE field_node = (pANTLR3_BASE_TREE)node->getChild(node, 2);

			std::string insn_name = (char *)(insn_node->getText(insn_node)->chars);
			std::string stage_name = (char *)(stage_node->getText(stage_node)->chars);
			std::string field_name = (char *)(field_node->getText(field_node)->chars);

			if (!hasInstructionGroup(insn_name) && !ISA.hasInstruction(insn_name)) {
				fprintf(stderr, "uArch error: Could not find instruction or group %s (%u)\n", insn_name.c_str(), node->getLine(node));
				return false;
			}

			if (!hasPipelineStage(stage_name)) {
				fprintf(stderr, "uArch error: Could not find pipeline stage %s (%u)\n", stage_name.c_str(), node->getLine(node));
				return false;
			}

			uint32_t stage_num = getPipelineStageIndex(stage_name);

			PipelineDetails &details = GetOrCreatePipelineDetails(insn_name, stage_num);

			if (node->getChildCount(node) == 4) {
				pANTLR3_BASE_TREE num_node = (pANTLR3_BASE_TREE)node->getChild(node, 3);
				gensim::uarch::expression::UArchExpression *exp = gensim::uarch::expression::UArchExpression::Parse(num_node);

				std::string exp_error;
				if (!CheckValidLatencyExpression(*exp, insn_name, exp_error)) {
					std::cerr << "Error in availability expression (line " << num_node->getLine(num_node) << "): " << exp_error << std::endl;
					return false;
				}

				details.AddSourceBankedReg(field_name, exp);
			} else {
				details.AddSourceReg(field_name);
			}
			return true;
		}

		bool UArchDescription::ParseSetDest(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == SET_DEST);
			assert(node->getChildCount(node) == 3 || node->getChildCount(node) == 4);

			pANTLR3_BASE_TREE insn_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
			pANTLR3_BASE_TREE stage_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);
			pANTLR3_BASE_TREE field_node = (pANTLR3_BASE_TREE)node->getChild(node, 2);

			std::string insn_name = (char *)(insn_node->getText(insn_node)->chars);
			std::string stage_name = (char *)(stage_node->getText(stage_node)->chars);
			std::string field_name = (char *)(field_node->getText(field_node)->chars);

			if (!hasInstructionGroup(insn_name) && !ISA.hasInstruction(insn_name)) {
				fprintf(stderr, "uArch error: Could not find instruction or group %s (%u)\n", insn_name.c_str(), node->getLine(node));
				return false;
			}

			if (!hasPipelineStage(stage_name)) {
				fprintf(stderr, "uArch error: Could not find pipeline stage %s (%u)\n", stage_name.c_str(), node->getLine(node));
				return false;
			}

			uint32_t stage_num = getPipelineStageIndex(stage_name);

			PipelineDetails &details = GetOrCreatePipelineDetails(insn_name, stage_num);
			if (node->getChildCount(node) == 4) {
				pANTLR3_BASE_TREE num_node = (pANTLR3_BASE_TREE)node->getChild(node, 3);
				gensim::uarch::expression::UArchExpression *exp = gensim::uarch::expression::UArchExpression::Parse(num_node);

				std::string exp_error;
				if (!CheckValidLatencyExpression(*exp, insn_name, exp_error)) {
					std::cerr << "Error in availability expression (line " << num_node->getLine(num_node) << "): " << exp_error << std::endl;
					return false;
				}

				details.AddDestBankedReg(field_name, exp);
			} else {
				details.AddDestReg(field_name);
			}

			return true;
		}

		bool UArchDescription::ParseSetLatencyConst(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == SET_LATENCY_CONST);
			assert(node->getChildCount(node) == 3);

			pANTLR3_BASE_TREE insn_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
			pANTLR3_BASE_TREE stage_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);
			pANTLR3_BASE_TREE latency_node = (pANTLR3_BASE_TREE)node->getChild(node, 2);

			std::string insn_name = (char *)(insn_node->getText(insn_node)->chars);
			std::string stage_name = (char *)(stage_node->getText(stage_node)->chars);

			if (!hasInstructionGroup(insn_name) && !ISA.hasInstruction(insn_name)) {
				fprintf(stderr, "uArch error: Could not find instruction or group %s (%u)\n", insn_name.c_str(), node->getLine(node));
				return false;
			}

			if (!hasPipelineStage(stage_name)) {
				fprintf(stderr, "uArch error: Could not find pipeline stage %s (%u)\n", stage_name.c_str(), node->getLine(node));
				return false;
			}

			uint32_t stage_num = getPipelineStageIndex(stage_name);

			PipelineDetails &details = GetOrCreatePipelineDetails(insn_name, stage_num);
			if (details.default_latency != NULL) {
				fprintf(stderr, "uArch error: Multiple constant latencies defined for pipeline stage %s on instruction %s (%u)\n", stage_name.c_str(), insn_name.c_str(), node->getLine(node));
				return false;
			}

			gensim::uarch::expression::UArchExpression *Expression = gensim::uarch::expression::UArchExpression::Parse(latency_node);

			std::string exp_error;
			if (!CheckValidLatencyExpression(*Expression, insn_name, exp_error)) {
				std::cerr << "Error in expression (line " << latency_node->getLine(latency_node) << "): " << exp_error << std::endl;
				return false;
			}

			details.default_latency = Expression;

			return true;
		}

		bool UArchDescription::ParseSetLatencyPred(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == SET_LATENCY_PRED);
			assert(node->getChildCount(node) == 4);

			pANTLR3_BASE_TREE insn_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);
			pANTLR3_BASE_TREE stage_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);
			pANTLR3_BASE_TREE pred_node = (pANTLR3_BASE_TREE)node->getChild(node, 2);
			pANTLR3_BASE_TREE latency_node = (pANTLR3_BASE_TREE)node->getChild(node, 3);

			std::string insn_name = (char *)(insn_node->getText(insn_node)->chars);
			std::string stage_name = (char *)(stage_node->getText(stage_node)->chars);

			if (!hasInstructionGroup(insn_name) && !ISA.hasInstruction(insn_name)) {
				fprintf(stderr, "uArch error: Could not find instruction or group %s (%u)\n", insn_name.c_str(), node->getLine(node));
				return false;
			}

			if (!hasPipelineStage(stage_name)) {
				fprintf(stderr, "uArch error: Could not find pipeline stage %s (%u)\n", stage_name.c_str(), node->getLine(node));
				return false;
			}

			uint32_t stage_num = getPipelineStageIndex(stage_name);

			PipelineDetails &details = GetOrCreatePipelineDetails(insn_name, stage_num);

			gensim::uarch::expression::UArchExpression *PredExpression = gensim::uarch::expression::UArchExpression::Parse(pred_node);
			gensim::uarch::expression::UArchExpression *LatencyExpression = gensim::uarch::expression::UArchExpression::Parse(latency_node);

			std::string exp_error;
			if (!CheckValidLatencyExpression(*PredExpression, insn_name, exp_error)) {
				std::cerr << "Error in predicate expression (line " << pred_node->getLine(pred_node) << ")" << exp_error << std::endl;
				return false;
			}
			if (!CheckValidLatencyExpression(*LatencyExpression, insn_name, exp_error)) {
				std::cerr << "Error in latency expression (line " << latency_node->getLine(latency_node) << ")" << exp_error << std::endl;
				return false;
			}

			details.predicated_latency.push_back(PredicatedLatency(PredExpression, LatencyExpression));

			return true;
		}

		bool UArchDescription::ParseFlushPipeline(pANTLR3_BASE_TREE node)
		{
			assert(node->getType(node) == SET_FLUSH_PIPELINE);
			assert(node->getChildCount(node) == 1 || node->getChildCount(node) == 2);

			pANTLR3_BASE_TREE insn_node = (pANTLR3_BASE_TREE)node->getChild(node, 0);

			expression::UArchExpression *flush_expression;

			if (node->getChildCount(node) == 2) {

				pANTLR3_BASE_TREE predicate_node = (pANTLR3_BASE_TREE)node->getChild(node, 1);
				flush_expression = expression::UArchExpression::Parse(predicate_node);
			} else {
				flush_expression = expression::UArchExpression::CreateConst(1);
			}

			std::string insn_name = (char *)(insn_node->getText(insn_node)->chars);

			GetOrCreateInstructionDetails(insn_name).CausesPipelineFlush = flush_expression;

			return true;
		}
	}
}  // namespace gensim, uarch
