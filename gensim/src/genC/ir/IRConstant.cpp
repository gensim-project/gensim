/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#include "genC/ir/IRConstant.h"
#include <functional>

using namespace gensim::genc;

IRConstant::IRConstant() : type_(Type_Invalid), integer_(0), vector_(nullptr), struct_(nullptr)
{
}


IRConstant::IRConstant(const IRConstant& other) : type_(other.type_), integer_(other.integer_), struct_(nullptr), vector_(nullptr)
{
	switch (Type()) {
		case Type_Struct:
			struct_ = new IRStructMap(*other.struct_);
			break;
		case Type_Vector:
			vector_ = new std::vector<IRConstant>(*other.vector_);
			break;
		default:
			// nothing extra to do
			break;
	}
}

IRConstant & IRConstant::operator=(const IRConstant& other)
{
	if(this == &other) {
		return *this;
	}
	type_ = other.type_;
	integer_ = other.integer_;

	if(struct_ != nullptr) {
		delete struct_;
		struct_ = nullptr;
	}
	if(vector_ != nullptr) {
		delete vector_;
		vector_ = nullptr;
	}

	switch(type_) {
		case Type_Struct:
			struct_ = new IRStructMap(*other.struct_);
			break;
		case Type_Vector:
			vector_ = new std::vector<IRConstant>(*other.vector_);
			break;
		default:
			// nothing to do
			break;
	}

	return *this;
}


IRConstant IRConstant::VGet(int idx) const
{
	GASSERT(Type() == Type_Vector);
	return vector_->at(idx);
}

void IRConstant::VPut(int idx, const IRConstant& val)
{
	GASSERT(Type() == Type_Vector);
	GASSERT(vector_->front().Type() == val.Type());

	vector_->at(idx) = val;
}

size_t IRConstant::VSize() const
{
	GASSERT(Type() == Type_Vector);
	return vector_->size();
}


IRConstant IRConstant::ROL(const IRConstant& lhs, const IRConstant& rhs, int width_in_bits)
{
	GASSERT(lhs.Type() == Type_Integer);
	GASSERT(rhs.Type() == Type_Integer);

	uint64_t lhs_value = lhs.Int();
	uint64_t lowbits = lhs_value << rhs.Int();
	uint64_t highbits = lhs_value >> (width_in_bits - rhs.Int());

	return IRConstant::Integer((lowbits | highbits)  & ((1ULL << width_in_bits)-1));
}

IRConstant IRConstant::ROR(const IRConstant& lhs, const IRConstant& rhs, int width_in_bits)
{
	GASSERT(lhs.Type() == Type_Integer);
	GASSERT(rhs.Type() == Type_Integer);

	uint64_t lhs_value = lhs.Int();
	uint64_t lowbits = lhs_value >> rhs.Int();
	uint64_t highbits = lhs_value << (width_in_bits - rhs.Int());

	return IRConstant::Integer((lowbits | highbits)  & ((1ULL << width_in_bits)-1));
}

IRConstant IRConstant::SSR(const IRConstant& lhs, const IRConstant& rhs)
{
	GASSERT(lhs.Type() == rhs.Type());
	GASSERT(lhs.Type() == IRConstant::Type_Integer);

	return IRConstant::Integer((int64_t)lhs.Int() >> rhs.Int());
}

#define MAPVECTOR(lhs, rhs, op) [lhs, rhs](){IRConstant v = IRConstant::Vector(lhs.VSize(), lhs.VGet(0)); for(unsigned i = 0; i < lhs.VSize(); ++i) { v.VPut(i, lhs.VGet(i) op rhs.VGet(i)); } return v; }()

IRConstant operator+(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() + rhs.Int());
		case IRConstant::Type_Float_Single:
			return IRConstant::Float(lhs.Flt() + rhs.Flt());
		case IRConstant::Type_Float_Double:
			return IRConstant::Double(lhs.Dbl() + rhs.Dbl());
		case IRConstant::Type_Vector:
			return MAPVECTOR(lhs, rhs, +);
		default:
			UNEXPECTED;
	}
}

IRConstant operator-(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() - rhs.Int());
		case IRConstant::Type_Float_Single:
			return IRConstant::Float(lhs.Flt() - rhs.Flt());
		case IRConstant::Type_Float_Double:
			return IRConstant::Double(lhs.Dbl() - rhs.Dbl());
		case IRConstant::Type_Vector:
			return MAPVECTOR(lhs, rhs, -);
		default:
			UNEXPECTED;
	}
}

IRConstant operator-(const IRConstant &value)
{
	switch(value.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(- value.Int());
		case IRConstant::Type_Float_Single:
			return IRConstant::Float(- value.Flt());
		case IRConstant::Type_Float_Double:
			return IRConstant::Double(- value.Dbl());
		default:
			UNEXPECTED;
	}
}

IRConstant operator*(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() * rhs.Int());
		case IRConstant::Type_Float_Single:
			return IRConstant::Float(lhs.Flt() * rhs.Flt());
		case IRConstant::Type_Float_Double:
			return IRConstant::Double(lhs.Dbl() * rhs.Dbl());
		case IRConstant::Type_Vector:
			return MAPVECTOR(lhs, rhs, *);
		default:
			UNEXPECTED;
	}
}

IRConstant operator/(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() / rhs.Int());
		case IRConstant::Type_Float_Single:
			return IRConstant::Float(lhs.Flt() / rhs.Flt());
		case IRConstant::Type_Float_Double:
			return IRConstant::Double(lhs.Dbl() / rhs.Dbl());
		case IRConstant::Type_Vector:
			return MAPVECTOR(lhs, rhs, /);
		default:
			UNEXPECTED;
	}
}

IRConstant operator%(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() % rhs.Int());
		default:
			UNEXPECTED;
	}
}

IRConstant operator>(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() > rhs.Int());
		case IRConstant::Type_Float_Single:
			return IRConstant::Integer(lhs.Flt() > rhs.Flt());
		case IRConstant::Type_Float_Double:
			return IRConstant::Integer(lhs.Dbl() > rhs.Dbl());
		case IRConstant::Type_Vector:
			return MAPVECTOR(lhs, rhs, >);
		default:
			UNEXPECTED;
	}
}
IRConstant operator>=(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() >= rhs.Int());
		case IRConstant::Type_Float_Single:
			return IRConstant::Integer(lhs.Flt() >= rhs.Flt());
		case IRConstant::Type_Float_Double:
			return IRConstant::Integer(lhs.Dbl() >= rhs.Dbl());
		case IRConstant::Type_Vector:
			return MAPVECTOR(lhs, rhs, >=);
		default:
			UNEXPECTED;
	}
}

IRConstant operator<(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() < rhs.Int());
		case IRConstant::Type_Float_Single:
			return IRConstant::Integer(lhs.Flt() < rhs.Flt());
		case IRConstant::Type_Float_Double:
			return IRConstant::Integer(lhs.Dbl() < rhs.Dbl());
		case IRConstant::Type_Vector:
			return MAPVECTOR(lhs, rhs, <);
		default:
			UNEXPECTED;
	}
}

IRConstant operator<=(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() <= rhs.Int());
		case IRConstant::Type_Float_Single:
			return IRConstant::Integer(lhs.Flt() <= rhs.Flt());
		case IRConstant::Type_Float_Double:
			return IRConstant::Integer(lhs.Dbl() <= rhs.Dbl());
		case IRConstant::Type_Vector:
			return MAPVECTOR(lhs, rhs, <=);
		default:
			UNEXPECTED;
	}
}

IRConstant operator&(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() & rhs.Int());
		default:
			UNEXPECTED;
	}
}

IRConstant operator|(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() | rhs.Int());
		default:
			UNEXPECTED;
	}
}

IRConstant operator^(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() ^ rhs.Int());
		default:
			UNEXPECTED;
	}
}

IRConstant operator>>(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() >> rhs.Int());
		default:
			UNEXPECTED;
	}
}

IRConstant operator<<(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() << rhs.Int());
		default:
			UNEXPECTED;
	}
}

IRConstant operator&&(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() && rhs.Int());
		default:
			UNEXPECTED;
	}
}

IRConstant operator||(const IRConstant &lhs, const IRConstant &rhs)
{
	GASSERT(lhs.Type() == rhs.Type());

	switch(lhs.Type()) {
		case IRConstant::Type_Integer:
			return IRConstant::Integer(lhs.Int() || rhs.Int());
		default:
			UNEXPECTED;
	}
}
