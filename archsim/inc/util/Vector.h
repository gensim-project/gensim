/* This file is Copyright University of Edinburgh 2018. For license details, see LICENSE. */

/*
 * Vector.h
 *
 *  Created on: 25 May 2015
 *      Author: harry
 */

#ifndef INC_UTIL_VECTOR_H_
#define INC_UTIL_VECTOR_H_

#include <type_traits>

template <typename ElementT, unsigned Width> class Vector
{
public:

	Vector() {}

	// extend vector to integer type
	Vector(const Vector<bool, Width> &other)
	{
		static_assert(std::is_integral<ElementT>::value, "Cannot extend bool to non integral type");

		for(unsigned i = 0; i < Width; ++i) {
			elements[i] = other.ExtractElement(i) ? (ElementT)-1 : 0;
		}
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

private:
	ElementT elements[Width];
};


#define OPERATE(op) \
template <typename ElementT, unsigned Width> Vector<ElementT, Width> operator op(const Vector<ElementT, Width> &v1, const Vector<ElementT, Width> &v2) { \
	Vector<ElementT, Width> result; \
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

#define OPERATE_COMPARE(op) \
template <typename ElementT, unsigned Width> Vector<bool, Width> operator op(const Vector<ElementT, Width> &v1, const Vector<ElementT, Width> &v2) { \
	Vector<bool, Width> result; \
	for(unsigned i = 0; i < Width; ++i) { result[i] = v1[i] op v2[i]; } \
	return result; \
}

OPERATE_COMPARE(<)

#undef OPERATE_COMPARE
#endif /* INC_UTIL_VECTOR_H_ */
