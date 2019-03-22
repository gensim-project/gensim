/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

#pragma once

#include "define.h"

#include <vector>
#include <map>
#include <string>
#include <stdint.h>
#include <stdexcept>

namespace gensim
{
	namespace genc
	{
		class IRType;
		class IRStructMap;
		class IRConstantVector;

		class IRConstant
		{
		public:
			enum ValueType {
				Type_Invalid,
				Type_Integer,
				Type_BigInteger,
				Type_Float_Single,
				Type_Float_Double,
				Type_Float_LongDouble,
				Type_Struct,
				Type_Vector
			};
			static std::string GetValueTypeName(ValueType type);

			IRConstant();

			IRConstant(const IRConstant &other);
			IRConstant & operator=(const IRConstant &other);

			~IRConstant();

			static IRConstant GetDefault(const gensim::genc::IRType &type);

			static IRConstant Integer(uint64_t i)
			{
				IRConstant v;
				v.type_ = Type_Integer;
				v.integer_ = i;
				return v;
			}
			static IRConstant BigInteger(uint64_t l, uint64_t h)
			{
				IRConstant v;
				v.type_ = Type_BigInteger;
				v.integer128_.integer_l_= l;
				v.integer128_.integer_h_ = h;
				return v;
			}
			static IRConstant Float(float i)
			{
				IRConstant v;
				v.type_ = Type_Float_Single;
				v.float_ = i;
				return v;
			}
			static IRConstant Double(double i)
			{
				IRConstant v;
				v.type_ = Type_Float_Double;
				v.double_ = i;
				return v;
			}
			static IRConstant LongDouble(long double i)
			{
				IRConstant v;
				v.type_ = Type_Float_LongDouble;
				v.long_double_ = i;
				return v;
			}
			static IRConstant Struct(const IRStructMap * struc)
			{
				IRConstant v;
				v.type_ = Type_Struct;
				v.struct_ = struc;
				return v;
			}
			static IRConstant Vector(int width, const IRConstant &def);
			static IRConstant Vector(const IRConstantVector &vector);

			uint64_t Int() const
			{
				GASSERT(Type() == Type_Integer);
				return integer_;
			}
			long double LongDbl() const
			{
				GASSERT(Type() == Type_Float_LongDouble);
				return long_double_;
			}
			double Dbl() const
			{
				GASSERT(Type() == Type_Float_Double);
				return double_;
			}
			uint64_t DblBits() const
			{
				GASSERT(Type() == Type_Float_Double);
				return *(uint64_t*)&double_;
			}
			float Flt() const
			{
				GASSERT(Type() == Type_Float_Single);
				return float_;
			}
			uint32_t FltBits() const
			{
				GASSERT(Type() == Type_Float_Single);
				return *(uint32_t*)&float_;
			}
			uint64_t POD() const
			{
				GASSERT(Type() == Type_Integer || Type() == Type_Float_Single || Type() == Type_Float_Double || Type() == Type_Float_LongDouble);
				return integer_;
			}

			const IRStructMap &Struct() const
			{
				GASSERT(Type() == Type_Struct);
				return *struct_;
			}

			ValueType Type() const
			{
				return type_;
			}

			bool operator==(const IRConstant &other)
			{
				GASSERT(type_ != Type_Struct);
				return type_ == other.type_ && integer_ == other.integer_;
			}
			bool operator!=(const IRConstant &other)
			{
				return !(*this == other);
			}

			explicit operator bool()
			{
				switch(Type()) {
					case Type_Integer:
						return Int() != 0;
					case Type_Float_Single:
						return Flt() != 0.0f;
					case Type_Float_Double:
						return Dbl() != 0.0;
					case Type_Float_LongDouble:
						return LongDbl() != 0.0;
					default:
						throw std::logic_error("Boolean operator on IRConstant of unsupported type");
				}
			}

			// Extra operators
			static IRConstant SSR(const IRConstant &lhs, const IRConstant &rhs);
			static IRConstant ROL(const IRConstant &lhs, const IRConstant &rhs, int width);
			static IRConstant ROR(const IRConstant &lhs, const IRConstant &rhs, int width);

			// Vector operators
			IRConstantVector &GetVector()
			{
				GASSERT(type_ == Type_Vector);
				return *vector_;
			}
			const IRConstantVector &GetVector() const
			{
				GASSERT(type_ == Type_Vector);
				return *vector_;
			}

		private:
			ValueType type_;

			union {
				uint64_t integer_;
				float float_;
				double double_;
				long double long_double_;
				struct {
					uint64_t integer_l_;
					uint64_t integer_h_;
				} integer128_;
			};
			const IRStructMap * struct_;
			IRConstantVector * vector_;
		};


		class IRConstantVector
		{
		public:
			IRConstantVector(int width, IRConstant def_elem) : storage_(width, def_elem) {}

			const IRConstant &GetElement(int i) const
			{
				return storage_.at(i);
			}
			void SetElement(int i, const IRConstant &elem)
			{
				GASSERT(TypeMatches(elem));
				storage_.at(i) = elem;
			}
			int Width() const
			{
				return storage_.size();
			}

		protected:
			bool TypeMatches(const IRConstant &e)
			{
				return storage_.at(0).Type() == e.Type();
			}

		private:
			std::vector<IRConstant> storage_;
		};

		class IRStructMap
		{
		public:
			const IRConstant &Get(const std::string &name) const
			{
				return values_.at(name);
			}
			void Set(const std::string &name, const IRConstant &constant)
			{
				values_[name] = constant;
			}
		private:
			std::map<std::string, IRConstant> values_;
		};
	}
}

gensim::genc::IRConstant operator+(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator-(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator-(const gensim::genc::IRConstant &value);
gensim::genc::IRConstant operator*(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator/(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator%(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);

gensim::genc::IRConstant operator>(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator>=(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator<(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator<=(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);

gensim::genc::IRConstant operator&(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator|(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator^(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);

gensim::genc::IRConstant operator>>(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator<<(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);

gensim::genc::IRConstant operator&&(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
gensim::genc::IRConstant operator||(const gensim::genc::IRConstant &lhs, const gensim::genc::IRConstant &rhs);
