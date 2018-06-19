/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#ifndef UARCHDESCRIPTION_H
#define UARCHDESCRIPTION_H

#include "isa/ISADescription.h"

namespace gensim
{
	namespace uarch
	{
		namespace expression
		{

			enum UArchExpressionNodeType {
				UARCH_UNARY_OPERATOR,
				UARCH_BINARY_OPERATOR,
				UARCH_FUNCTION,
				UARCH_PROPERTY,
				UARCH_ID,
				UARCH_INDEX,
				UARCH_CONST
			};

			enum UArchExpressionOperator {
				UARCH_NOT,
				UARCH_COMPLEMENT,
				UARCH_PLUS,
				UARCH_MINUS,
				UARCH_MULTIPLY,
				UARCH_DIVIDE,
				UARCH_MODULO,
				UARCH_LSHIFT,
				UARCH_RSHIFT,
				UARCH_GT,
				UARCH_GE,
				UARCH_LT,
				UARCH_LE,
				UARCH_EQ,
				UARCH_NE,
				UARCH_B_AND,
				UARCH_B_OR,
				UARCH_B_XOR,
				UARCH_L_AND,
				UARCH_L_OR
			};

			struct UArchExpression;
			struct UArchExpression_Unary;
			struct UArchExpression_Binary;
			struct UArchExpression_Function;
			struct UArchExpression_Property;
			struct UArchExpression_Index;
			struct UArchExpression_ID;
			struct UArchExpression_Const;

			class UArchExpression
			{
			public:
				UArchExpressionNodeType NodeType;

				union {
					UArchExpression_Unary *Unary;
					UArchExpression_Binary *Binary;
					UArchExpression_Function *Function;
					UArchExpression_Property *Property;
					UArchExpression_Index *Index;
					UArchExpression_ID *ID_Node;
					UArchExpression_Const *Const;
				} Node;

				std::string Print();

			private:
				UArchExpression() {};

			public:
				~UArchExpression();

				static UArchExpression *Parse(pANTLR3_BASE_TREE);
				static UArchExpressionOperator ParseOperator(pANTLR3_BASE_TREE);
				static UArchExpression *CreateConst(uint32_t);
				static std::string PrintOperator(UArchExpressionOperator);
			};

			struct UArchExpression_Unary {
				UArchExpression *Expr;
				UArchExpressionOperator Operator;

				UArchExpression_Unary(UArchExpressionOperator, UArchExpression *);
			};

			struct UArchExpression_Binary {
				UArchExpressionOperator Operator;
				UArchExpression *Left, *Right;

				UArchExpression_Binary(UArchExpressionOperator, UArchExpression *_Left, UArchExpression *_Right);
			};

			struct UArchExpression_Function {
				std::string Function;
				std::vector<UArchExpression *> Arguments;

				UArchExpression_Function(std::string);
			};

			struct UArchExpression_Property {
				std::string Parent;
				std::string Child;

				UArchExpression_Property(std::string, std::string);
			};

			struct UArchExpression_Index {
				std::string Parent;
				UArchExpression *Index;

				UArchExpression_Index(std::string, UArchExpression *);
			};

			struct UArchExpression_ID {
				std::string Name;

				UArchExpression_ID(std::string);
			};

			struct UArchExpression_Const {
				uint32_t Const;

				UArchExpression_Const(uint32_t);
			};

		}  // namespace expression

		namespace UArchType
		{

			enum UArchType {
				InOrder,
				Superscalar,
				OutOfOrder
			};
		}

// Struct representing a type of functional unit

		class FunctionalUnitType
		{
		public:
			const std::string Name;

			FunctionalUnitType(std::string Name) : Name(Name) {}
		};

		class PipelineStage
		{
		public:
			std::string Name;
			uint32_t Default_Latency;

			std::vector<FunctionalUnitType *> Functional_Units;
			FunctionalUnitType *Default_Unit;

			PipelineStage(std::string _Name, uint32_t _Default_Latency) : Name(_Name), Default_Latency(_Default_Latency) {}

			void AddFuncUnit(FunctionalUnitType &unit_type)
			{
				Functional_Units.push_back(&unit_type);
			}

			void SetDefaultUnit(FunctionalUnitType &unit_type)
			{
				// TODO: Make sure unit_type is in Functional_Units

				Default_Unit = &unit_type;
			}
		};

		struct ConstLatency {
			gensim::uarch::expression::UArchExpression *Latency;
		};

		struct PredicatedLatency {
			gensim::uarch::expression::UArchExpression *Predicate, *Latency;

			PredicatedLatency(gensim::uarch::expression::UArchExpression *, gensim::uarch::expression::UArchExpression *);
		};

// Struct representing the details for one stage of one instruction or isntruction group

		struct PipelineDetails {
			gensim::uarch::expression::UArchExpression *default_latency;

			std::vector<PredicatedLatency> predicated_latency;

			std::vector<std::string> Source_Single_Regs;
			std::multimap<std::string, gensim::uarch::expression::UArchExpression *> Source_Banked_Regs;

			std::vector<std::string> Dest_Single_Regs;
			std::multimap<std::string, gensim::uarch::expression::UArchExpression *> Dest_Banked_Regs;

			void AddSourceReg(std::string source);
			void AddSourceBankedReg(std::string bank, gensim::uarch::expression::UArchExpression *reg);

			void AddDestReg(std::string dest);
			void AddDestBankedReg(std::string bank, gensim::uarch::expression::UArchExpression *reg);

			std::vector<FunctionalUnitType *> Func_Units;

			inline void SetFuncUnit(FunctionalUnitType &Unit)
			{
				Func_Units.push_back(&Unit);
			}
		};

// Struct representing the pipeline information for a single instruction or instruction group

		struct InstructionDetails {
			const isa::InstructionDescription *Description;
			std::map<uint32_t, PipelineDetails *> Pipeline;

			gensim::uarch::expression::UArchExpression *CausesPipelineFlush;

		public:
			InstructionDetails() : Description(NULL), CausesPipelineFlush(NULL) {};
		};

		class UArchDescription
		{
		public:
			UArchDescription(const char *const filename, isa::ISADescription &ISA);
			virtual ~UArchDescription();

			void ParseUArchDescription(const char *const filename);

			bool Validate(std::stringstream &errorstream) const;

			UArchType::UArchType Type;

			isa::ISADescription &ISA;

			std::vector<PipelineStage> Pipeline_Stages;
			std::vector<std::string> Func_Units;  // todo: we probably need a functional unit struct

			std::map<std::string, std::list<const isa::InstructionDescription *> > Groups;

			std::map<std::string, InstructionDetails *> Details;

			std::map<std::string, FunctionalUnitType *> Functional_Unit_Types;

			inline void AddUnitType(const std::string UnitType)
			{
				assert(Functional_Unit_Types.find(UnitType) == Functional_Unit_Types.end());
				Functional_Unit_Types[UnitType] = new FunctionalUnitType(UnitType);
			}

			inline FunctionalUnitType &GetUnitType(const std::string UnitType)
			{
				assert(Functional_Unit_Types.find(UnitType) != Functional_Unit_Types.end());
				return *(Functional_Unit_Types[UnitType]);
			}

			inline int getPipelineStageIndex(std::string stage_name) const
			{
				int idx = 0;
				for (std::vector<PipelineStage>::const_iterator i = Pipeline_Stages.begin(); i != Pipeline_Stages.end(); ++i) {
					if (i->Name == stage_name) return idx;
					idx++;
				}
				assert(false);
				UNEXPECTED;
			}

			inline bool hasPipelineStage(std::string n) const
			{
				for (std::vector<PipelineStage>::const_iterator i = Pipeline_Stages.begin(); i != Pipeline_Stages.end(); ++i) {
					if (i->Name == n) return true;
				}

				return false;
			}

			inline bool hasFunctionalUnit(std::string n) const
			{
				for (std::vector<std::string>::const_iterator i = Func_Units.begin(); i != Func_Units.end(); ++i) {
					if (*i == n) return true;
				}
				return false;
			}

			inline bool hasInstructionGroup(std::string n) const
			{
				return Groups.find(n) != Groups.end();
			}

			inline const InstructionDetails *GetInstructionDetails(std::string n) const
			{
				assert(hasInstructionGroup(n) || ISA.hasInstruction(n));

				std::map<std::string, InstructionDetails *>::const_iterator d = Details.find(n);
				if (d == Details.end()) return NULL;
				return (d->second);
			}

			inline InstructionDetails &GetOrCreateInstructionDetails(std::string n)
			{
				assert(hasInstructionGroup(n) || ISA.hasInstruction(n));

				InstructionDetails *details;

				std::map<std::string, InstructionDetails *>::iterator d = Details.find(n);
				if (d == Details.end()) {
					details = new InstructionDetails();
					if (ISA.hasInstruction(n)) details->Description = ISA.Instructions.at(n);

					Details.insert(std::pair<std::string, InstructionDetails *>(n, details)).first;
				} else
					details = d->second;
				return *details;
			}

			inline const PipelineDetails *GetPipelineDetails(std::string n, uint32_t stage_idx) const
			{
				const InstructionDetails *insn = GetInstructionDetails(n);
				if (insn == NULL) return NULL;
				std::map<uint32_t, PipelineDetails *>::const_iterator i = insn->Pipeline.find(stage_idx);
				if (i == insn->Pipeline.end()) return NULL;
				return (i->second);
			}

			inline PipelineDetails &GetOrCreatePipelineDetails(std::string n, uint32_t stage_idx)
			{
				InstructionDetails &insn = GetOrCreateInstructionDetails(n);

				std::map<uint32_t, PipelineDetails *>::iterator i = insn.Pipeline.find(stage_idx);
				if (i == insn.Pipeline.end()) {
					PipelineDetails *details = new PipelineDetails();
					insn.Pipeline.insert(std::pair<uint32_t, PipelineDetails *>(stage_idx, details));
					return *details;
				}
				return *(i->second);
			}

		private:
			UArchDescription(isa::ISADescription &);
			UArchDescription(const UArchDescription &orig);

			bool CheckValidLatencyExpression(const gensim::uarch::expression::UArchExpression &expr, const std::string &group, std::string &error) const;

			// ----- Out of order extensions -----
			bool ParseUArchType(pANTLR3_BASE_TREE);       // Set the uArch type e.g. in order, superscalar, etc
			bool ParseUnitType(pANTLR3_BASE_TREE);	// Create a new type of functional unit
			bool ParseFuncUnit(pANTLR3_BASE_TREE);	// Instantiate one or more functional units for a specific pipeline stage
			bool ParseSetDefaultUnit(pANTLR3_BASE_TREE);  // Set the default functional unit type used by an instruction in a given pipeline stage
			bool ParseSetFuncUnit(pANTLR3_BASE_TREE);     // Set the functional unit used for a specific instruction or instruction group in a given pipeline stage

			// ----- Standard In-Order Modeling -----
			bool ParsePipelineStage(pANTLR3_BASE_TREE);
			bool ParseGroup(pANTLR3_BASE_TREE);
			bool ParseSetGroup(pANTLR3_BASE_TREE);
			bool ParseSetSrc(pANTLR3_BASE_TREE);
			bool ParseSetDest(pANTLR3_BASE_TREE);
			bool ParseSetLatencyConst(pANTLR3_BASE_TREE);
			bool ParseSetLatencyPred(pANTLR3_BASE_TREE);
			bool ParseFlushPipeline(pANTLR3_BASE_TREE);
		};
	}
}  // namespace gensim, uarch

#endif /* UARCHDESCRIPTION_H */
