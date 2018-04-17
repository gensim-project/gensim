/*
 * genC/ssa/SSAValue.h
 *
 * Copyright (C) University of Edinburgh 2017.  All Rights Reserved.
 *
 * Harry Wagstaff	<hwagstaf@inf.ed.ac.uk>
 * Tom Spink		<tspink@inf.ed.ac.uk>
 */
#pragma once

#include "genC/ssa/SSAType.h"
#include "genC/ssa/SSAValueNamespace.h"
#include "genC/ssa/metadata/SSAMetadata.h"
#include "genC/DiagNode.h"
#include "util/Disposable.h"

#include <vector>
#include <sstream>
#include <stdexcept>

namespace gensim
{
	namespace genc
	{
		namespace ssa
		{
			class SSAContext;
			class SSAMetadata;

			class SSAValue : public util::Disposable
			{
			public:
				typedef std::vector<SSAValue *> use_list_t;
				typedef std::vector<SSAMetadata *> metadata_list_t;

				SSAValue(SSAContext& context, SSAValueNamespace &ns);
				virtual ~SSAValue();

				SSAContext& GetContext() const
				{
					return _context;
				}

				virtual const SSAType GetType() const = 0;

				virtual std::string GetName() const;
				SSAValueNamespace::value_name_t GetValueName() const
				{
					return name_;
				}

				const use_list_t &GetUses() const;
				void AddUse(SSAValue *user);
				void RemoveUse(SSAValue *user);
                                bool HasDynamicUses() const;

				/**
				 * Determines whether or not this SSA statement contains valid diagnostic metadata.
				 * @return Returns TRUE if this SSA statement contains valid diagnostic metadata, or FALSE otherwise.
				 */
				bool HaveDiag() const
				{
					return GetMetadataByType<DiagnosticNodeMetadata>() != nullptr;
				}

				/**
				 * Get the diagnostic metadata for this SSA statement, if it exists.  If it does not, return a
				 * default value.
				 * @return Returns the associated diagnostic metadata for this SSA statement, or a default value
				 * if it does not exist.
				 */
				const DiagNode GetDiag() const
				{
					auto dmd = GetMetadataByType<DiagnosticNodeMetadata>();
					if (dmd == nullptr) return DiagNode();
					return dmd->GetDiagNode();
				}

				/**
				 * Set the diagnostic metadata for this SSA statement, replacing it if it already exists.
				 * @param diag The diagnostic node to associate with this statement.
				 */
				void SetDiag(const DiagNode& diag)
				{
					// Remove existing diagnostic node metadata (if it exists)
					auto dmd = GetMetadataByType<DiagnosticNodeMetadata>();
					if (dmd != nullptr) {
						RemoveMetadata(dmd);
						delete dmd;
					}

					AddMetadata(new DiagnosticNodeMetadata(diag));
				}

				const metadata_list_t &GetMetadata() const;
				void AddMetadata(SSAMetadata *metadata);
				void RemoveMetadata(SSAMetadata *metadata);

				template<typename TMetadata> TMetadata *GetMetadataByType() const
				{
					for (auto md : _metadata) {
						if (auto mdt = dynamic_cast<TMetadata *>(md)) return mdt;
					}

					return nullptr;
				}

				virtual void Unlink() = 0;
				virtual std::string ToString() const;

				void Dump() const;
				void Dump(std::ostream &output) const;

			protected:
				void PerformDispose() override;

			private:
				SSAContext& _context;
				use_list_t _uses;
				metadata_list_t _metadata;
				SSAValueNamespace::value_name_t name_;
			};

			struct SSAValueLess {
				bool operator()(const SSAValue *op1, const SSAValue *op2) const
				{
					GASSERT(op1 != nullptr);
					GASSERT(op2 != nullptr);
					GASSERT(op1->GetValueName() != op2->GetValueName() || op1 == op2);
//					return op1->GetValueName() < op2->GetValueName();
					return op1 < op2;
				}
			};
		}
	}
}
