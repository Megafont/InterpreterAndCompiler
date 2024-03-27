// This file contains code that abstracts how Lox data values are represented in our bytecode.
//

#pragma once

//#ifndef cLox_Value_h
//	#define cLox_Value_h

#include <string.h>

// cLox includes.
#include "Common.h"




// The forward declarations below are needed, because if you add an include of "Object.h"
// above, then you'll get a bunch of compiler screaming. I think this is due to cyclic
// dependencies.
// 
// Forward declaration of struct used to represent objects in the Value struct (things like
// strings and class instances)
struct Obj;

// Forward declaration of struct for storing Lox strings.
struct ObjString;




// This conditional compilation is part of the NAN boxing optimization added in chapter 30 in the book.
// See the comments on the NAN_BOXING constant in Common.h for more.
// Also see chapter 30 in the book, as some of the code in the first half of this #ifdef block (not the #else clause)
// is rather confusing when you first see it.
#ifdef NAN_BOXING

#define SIGN_BIT	((uint64_t)0x8000000000000000)
#define QNAN		((uint64_t)0x7ffc000000000000)


// The comments on these lines show the binary representation of the numeric value.
#define TAG_NIL		1 // 01.
#define TAG_FALSE	2 // 10.
#define TAG_TRUE	3 // 11.


typedef uint64_t Value;


// These macros produce certain static values.
#define FALSE_VAL			((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL			((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL				((Value)(uint64_t)(QNAN | TAG_NIL))


// These macros allow us to check the type of the value stored in a Value struct.
#define IS_BOOL(value)		(((value) | 1) == TRUE_VAL)
#define IS_NIL(value)		((value) == NIL_VAL)
#define IS_NUMBER(value)	(((value) & QNAN) != QNAN)
#define IS_OBJ(value)		(((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))


// These macros convert a Lox value back into a native C++ value.
#define AS_BOOL(value)		((value) == TRUE_VAL)
#define AS_NUMBER(value)	ValueToNum(value)
#define AS_OBJ(value)		((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))


// These macros work in the opposite direction. They convert a native C++ value into a Lox value.
#define BOOL_VAL(b)			((b) ? TRUE_VAL : FALSE_VAL)
#define NUMBER_VAL(num)		NumToValue(num)
#define OBJ_VAL(obj)		(Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))



static inline double ValueToNum(Value value)
{
	double num;
	memcpy(&num, &value, sizeof(Value));
	return num;
}


static inline Value NumToValue(double num)
{
	Value value;
	memcpy(&value, &num, sizeof(double));
	return value;
}

#else

enum ValueType
{
	VAL_BOOL,
	VAL_NIL, // Null
	VAL_NUMBER,
	VAL_OBJ, // Object. This value type stores a pointer to larger objects on the heap, such as strings or class instances.
};


struct Value
{
	// I had to add this set of static creation methods to get the macros that
	// convert C++ values to Lox values to work. For example, NUMBER_VAL().
	// This is because of the fact that the book uses C, but I'm using C++
	// since I don't know how to get that working.
	// ====================================================================================================

	static Value CreateValue_Boolean(bool value)
	{
		Value v;

		v.Type = VAL_BOOL;
		v.As.Boolean = value;

		return v;
	}

	static Value CreateValue_NIL()
	{
		Value v;

		v.Type = VAL_NIL;
		v.As.Number = 0;

		return v;
	}

	static Value CreateValue_Number(double value)
	{
		Value v;

		v.Type = VAL_NUMBER;
		v.As.Number = value;

		return v;
	}

	static Value CreateValue_Object(Obj* obj)
	{
		Value v;

		v.Type = VAL_OBJ;
		v.As.Obj = obj;

		return v;
	}

	// ====================================================================================================


	ValueType Type;

	// We name this union "As", because it makes the code read nicer, "almost like a cast".
	// We're using a union to save memory since it stores all of its fields in the same memory
	// (overlapping each other). This works well because only one field should ever be in use
	// at a time. For example, if the value stored in a Value struct is type boolean, then we'd
	// only ever be using the boolean field of the union. 
	union As
	{
		bool Boolean;
		double Number;
		Obj* Obj;
	}; 

	As As;
};


// These macros allow us to check the type of the value stored in a Value struct.
#define IS_BOOL(value)			((value).Type == VAL_BOOL)
#define IS_NIL(value)			((value).Type == VAL_NIL)
#define IS_NUMBER(value)		((value).Type == VAL_NUMBER)
#define IS_OBJ(value)			((value).Type == VAL_OBJ)


// These macros convert a Lox value back into a native C++ value.
#define AS_BOOL(value)			((value).As.Boolean)
#define AS_NUMBER(value)		((value).As.Number)
#define AS_OBJ(value)			((value).As.Obj)


// These macros work in the opposite direction. They convert a native C++ value into a Lox value.
// Each one sets the Value struct's type flag, and then the value using the appropriate field
// of the Value struct's union field (called "as").
//
// NOTE: I had to change these macros to use special static methods I added to the Value struct
//		 to get them working in C++.
#define BOOL_VAL(value)			(Value::CreateValue_Boolean(value))
#define NIL_VAL					(Value::CreateValue_NIL())
#define NUMBER_VAL(value)		(Value::CreateValue_Number(value))
#define OBJ_VAL(object)			(Value::CreateValue_Object((Obj*) object))

#endif




struct ValueArray
{
	int Capacity;
	int Count;
	Value* Values;
};



bool ValuesEqual(Value a, Value b);

void InitValueArray(ValueArray* array);
void WriteValueArray(ValueArray* array, Value value);
void FreeValueArray(ValueArray* array);

void PrintValue(Value value);

//#endif