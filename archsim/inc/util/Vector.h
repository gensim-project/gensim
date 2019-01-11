/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * Vector.h
 *
 *  Created on: 25 May 2015
 *      Author: harry
 */

#ifndef INC_UTIL_VECTOR_H_
#define INC_UTIL_VECTOR_H_


#include <initializer_list>
#include <stdexcept>
#include <type_traits>

namespace archsim
{

	template <unsigned Size> struct VDT {
		using Type = uint64_t;
	};
	template<> struct VDT<1> {
		using Type = uint8_t;
	};
	template<> struct VDT<2> {
		using Type = uint16_t;
	};
	template<> struct VDT<4> {
		using Type = uint32_t;
	};
	template<> struct VDT<8> {
		using Type = uint64_t;
	};
	template<> struct VDT<16> {
		using Type = __int128;
	};

	template <typename ElementT, unsigned Width> class Vector
	{
	public:
		using VectorDataType = typename VDT<sizeof(ElementT) * Width>::Type;

		Vector()
		{
			for(unsigned i = 0; i < Width; ++i) {
				new (&elements[i]) ElementT();
			}
		}

		// extend vector to integer type
		Vector(const Vector<bool, Width> &other)
		{
			static_assert(std::is_integral<ElementT>::value, "Cannot extend bool to non integral type");

			for(unsigned i = 0; i < Width; ++i) {
				elements[i] = other.ExtractElement(i) ? (ElementT)-1 : 0;
			}
		}

		~Vector()
		{
			for(unsigned i = 0; i < Width; ++i) {
				elements[i].~ElementT();
			}
		}

		// 'Splat' constructor
		Vector(const ElementT &element)
		{
			for(auto &i : elements) {
				i = element;
			}
		}

		// Element cast constructor
		template<typename OtherTy> Vector(const Vector<OtherTy, Width> &otherVector)
		{
			assign(otherVector);
		}

		Vector(const std::initializer_list<ElementT> &elems)
		{
			if(elems.size() != Width) {
				throw std::range_error("Incorrect vector width");
			}
			unsigned i = 0;
			for(auto elem : elems) {
				elements[i++] = elem;
			}
		}

		template<typename OtherElementT> void operator=(const Vector<OtherElementT, Width> &other)
		{
			assign(other);
		}

		template<typename OtherElementT> operator Vector<OtherElementT, Width>()
		{
			Vector<OtherElementT, Width> newvector;
			newvector.assign(*this);
			return newvector;
		}

#define OPERATEANDSET(op) void operator op(const Vector<ElementT, Width> &other) { for(unsigned i = 0; i < Width; ++i) elements[i] op other.elements[i]; }

		OPERATEANDSET(+=)
		OPERATEANDSET(-=)
		OPERATEANDSET(*=)
		OPERATEANDSET(/=)

		OPERATEANDSET(&=)
		OPERATEANDSET(|=)
		OPERATEANDSET(^=)
#undef OPERATEANDSET

		void InsertElement(unsigned index, ElementT value)
		{
			elements[index] = value;
		}

		const ElementT &ExtractElement(unsigned index) const
		{
			return elements[index];
		}
		ElementT &ExtractElement(unsigned index)
		{
			return elements[index];
		}

		ElementT &operator[](unsigned i)
		{
			return ExtractElement(i);
		}
		const ElementT &operator[](unsigned i) const
		{
			return ExtractElement(i);
		}

		template<typename T> Vector<T, Width> Select(const Vector<T, Width> &a,  const Vector<T, Width> &b)
		{
			Vector<T, Width> out;
			for(unsigned i = 0; i < Width; ++i) {
				out.InsertElement(i, ExtractElement(i) ? a.ExtractElement(i) : b.ExtractElement(i));
			}
			return out;
		}

		void *data()
		{
			return (void*)&elements[0];
		}

		VectorDataType toData()
		{
			return *(VectorDataType*)data();
		}


		template<typename OtherElementT> void assign(const Vector<OtherElementT, Width> &other)
		{
			if(std::is_same<OtherElementT, bool>::value) {
				for(unsigned i = 0; i < Width; ++i) {
					elements[i] = other.ExtractElement(i) ? (ElementT)-1 : 0;
				}
			} else {
				for(unsigned i = 0; i < Width; ++i) {
					elements[i] = other.ExtractElement(i);
				}
			}
		}

	private:
		ElementT elements[Width];

	};

	template<typename ElementT, unsigned Width1, unsigned Width2, unsigned Width3> static Vector<ElementT, Width3> VectorShuffle(const Vector<ElementT, Width1> &v1, const Vector<ElementT, Width2> &v2, const Vector<uint32_t, Width3> &indices)
	{
		Vector<ElementT, Width3> output;
		for(int i = 0; i < Width3; ++i) {
			uint32_t index = indices.ExtractElement(i);
			const ElementT element = index < Width1 ? v1.ExtractElement(index) : v2.ExtractElement(index - Width1);
			output.InsertElement(i, element);
		}
		return output;
	}
}

#define OPERATE(op) \
template <typename ElementT, unsigned Width> archsim::Vector<ElementT, Width> operator op(const archsim::Vector<ElementT, Width> &v1, const archsim::Vector<ElementT, Width> &v2) { \
	archsim::Vector<ElementT, Width> result; \
	for(unsigned i = 0; i < Width; ++i) result[i] = v1[i] op v2[i]; \
	return result; \
}

OPERATE(+)
OPERATE(-)
OPERATE(*)
OPERATE(/)

OPERATE(&)
OPERATE(|)
OPERATE(^)

#undef OPERATE

#define OPERATE_SHIFT(op) \
template<typename ElementT, typename ElementCount, unsigned Width> archsim::Vector<ElementT, Width> operator op(const archsim::Vector<ElementT, Width> &v1, const archsim::Vector<ElementCount, Width> &v2) { \
archsim::Vector<ElementT, Width> result;\
for(unsigned i = 0; i < Width; ++i) { result[i] = v1[i] op v2[i]; } \
return result; \
}

OPERATE_SHIFT(<<)
OPERATE_SHIFT(>>)

#undef OPERATE_SHIFT

#define OPERATE_COMPARE(op) \
template <typename ElementT1, typename ElementT2, unsigned Width> archsim::Vector<bool, Width> operator op(const archsim::Vector<ElementT1, Width> &v1, const archsim::Vector<ElementT2, Width> &v2) { \
	archsim::Vector<bool, Width> result; \
	for(unsigned i = 0; i < Width; ++i) { result[i] = v1[i] op v2[i]; } \
	return result; \
}

OPERATE_COMPARE(<)
OPERATE_COMPARE(<=)
OPERATE_COMPARE(>=)
OPERATE_COMPARE(>)
OPERATE_COMPARE(==)
OPERATE_COMPARE(!=)

#undef OPERATE_COMPARE

#define OPERATE_UNARY(op) \
template<typename ElementT, unsigned Width> archsim::Vector<ElementT, Width> operator op(const archsim::Vector<ElementT, Width> &v) { \
archsim::Vector<ElementT, Width> result; \
for(unsigned i = 0; i < Width; ++i) { result[i] = op v[i]; } \
return result; \
}

OPERATE_UNARY(!)
OPERATE_UNARY(~)

#undef OPERATE_UNARY
#endif /* INC_UTIL_VECTOR_H_ */
