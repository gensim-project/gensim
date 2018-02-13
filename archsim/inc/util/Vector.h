/*
 * Vector.h
 *
 *  Created on: 25 May 2015
 *      Author: harry
 */

#ifndef INC_UTIL_VECTOR_H_
#define INC_UTIL_VECTOR_H_

template <typename ElementT, unsigned Width> class Vector
{
public:

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
#endif /* INC_UTIL_VECTOR_H_ */
