/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * genC/Intrinsics.h
 *
 * GenSim
 * Copyright (C) University of Edinburgh.  All Rights Reserved.
 *
 * Harry Wagstaff <hwagstaf@inf.ed.ac.uk>
 * Tom Spink <tspink@inf.ed.ac.uk>
 */
#pragma once

#include "arch/ArchDescription.h"
#include "genC/ssa/statement/SSAIntrinsicStatement.h"

#include <functional>

namespace gensim
{	
	namespace genc
	{
		enum class IntrinsicID {
			UNKNOWN,

#define Intrinsic(name, id, ...) id,
#include "Intrinsics.def"
#undef Intrinsic

			END
		};

		class IRCallExpression;

		class IntrinsicDescriptor {
		public:
			using SignatureFactory = std::function<IRSignature(const IntrinsicDescriptor *, const IRCallExpression *)>;
			using SSAEmitter = std::function<genc::ssa::SSAStatement *(const IntrinsicDescriptor *, const IRCallExpression *, ssa::SSABuilder &)>;
			using FixednessResolver = std::function<bool(const genc::ssa::SSAIntrinsicStatement *, const IntrinsicDescriptor *)>;

			IntrinsicDescriptor(const std::string &name, IntrinsicID id, const arch::ArchDescription &arch, const SignatureFactory &factory, const SSAEmitter &emitter, const FixednessResolver &resolver);

			const std::string &GetName() const { return name_;}
			IntrinsicID GetID() const { return id_; }

			IRSignature GetSignature(const IRCallExpression *call) const { return sig_fact_(this, call); }
			bool GetFixedness(const genc::ssa::SSAIntrinsicStatement *intrinsicStmt) const { return resolver_(intrinsicStmt, this); }
			ssa::SSAStatement *EmitSSA(const IRCallExpression *expr, ssa::SSABuilder &bldr) const { return emitter_(this, expr, bldr); }

			const arch::ArchDescription &GetArch() const { return arch_; }

		private:
			std::string name_;
			IntrinsicID id_;
			SignatureFactory sig_fact_;
			SSAEmitter emitter_;
			FixednessResolver resolver_;
			const arch::ArchDescription &arch_;
		};

		class IntrinsicManager {
		public:
			using BaseCollection = std::vector<IntrinsicDescriptor*>;
			using iterator = BaseCollection::iterator;
			using const_iterator = BaseCollection::const_iterator;

			IntrinsicManager(const gensim::arch::ArchDescription &arch);

			const IntrinsicDescriptor *GetByID(IntrinsicID id) const;
			const IntrinsicDescriptor *GetByName(const std::string &name) const;

			const arch::ArchDescription &GetArch() { return arch_; }

			iterator begin() { return intrinsics_.begin(); }
			iterator end() { return intrinsics_.end(); }
			
			const_iterator begin() const { return intrinsics_.begin(); }
			const_iterator end() const { return intrinsics_.end(); }

		private:
			bool LoadIntrinsics();
			bool AddIntrinsic(const std::string &name, IntrinsicID id, const IntrinsicDescriptor::SignatureFactory &factory, const IntrinsicDescriptor::SSAEmitter &emitter, const IntrinsicDescriptor::FixednessResolver &resolver);

			const gensim::arch::ArchDescription &arch_;

			std::map<IntrinsicID, IntrinsicDescriptor*> id_to_descriptor_;
			std::map<std::string, IntrinsicDescriptor*> name_to_descriptor_;
			std::vector<IntrinsicDescriptor*> intrinsics_;
		};
	}
}
