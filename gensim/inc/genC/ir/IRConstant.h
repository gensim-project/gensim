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
		class IRStructMap;

		class IRConstant
		{
		public:
			enum ValueType {
				Type_Invalid,
				Type_Integer,
				Type_Float_Single,
				Type_Float_Double,
				Type_Float_LongDouble,
				Type_Struct,
				Type_Vector
			};

			IRConstant();

			IRConstant(const IRConstant &other);
			IRConstant & operator=(const IRConstant &other);

			~IRConstant()
			{
				if(Type() == Type_Vector) {
					delete vector_;
				}
			}

			static IRConstant Integer(uint64_t i)
			{
				IRConstant v;
				v.type_ = Type_Integer;
				v.integer_ = i;
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
			static IRConstant Vector(int width, const IRConstant &def)
			{
				IRConstant v;
				v.type_ = Type_Vector;
				v.vector_ = new std::vector<IRConstant>(width, def);
				return v;
			}

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
			float Flt() const
			{
				GASSERT(Type() == Type_Float_Single);
				return float_;
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
						throw std::logic_error("");
				}
			}

			// Extra operators
			static IRConstant SSR(const IRConstant &lhs, const IRConstant &rhs);
			static IRConstant ROL(const IRConstant &lhs, const IRConstant &rhs, int width);
			static IRConstant ROR(const IRConstant &lhs, const IRConstant &rhs, int width);

			// Vector operators
			IRConstant VGet(int idx) const;
			void VPut(int idx, const IRConstant &val);
			size_t VSize() const;

		private:
			ValueType type_;

			union {
				uint64_t integer_;
				float float_;
				double double_;
				long double long_double_;
			};
			const IRStructMap * struct_;
			std::vector<IRConstant> * vector_;
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