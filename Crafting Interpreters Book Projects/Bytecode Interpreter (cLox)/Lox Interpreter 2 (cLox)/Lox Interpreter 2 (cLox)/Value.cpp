#include <stdio.h>
#include <string.h>

// cLox includes.
#include "Object.h"
#include "Memory.h"
#include "Value.h"




void InitValueArray(ValueArray* array)
{
	array->Values = NULL;
	array->Capacity = 0;
	array->Count = 0;
}


void WriteValueArray(ValueArray* array, Value value)
{
	// Expand the dynamic array if necessary.
	if (array->Capacity < array->Count + 1)
	{
		int oldCapacity = array->Capacity;
		array->Capacity = GROW_CAPACITY(oldCapacity);
		array->Values = GROW_ARRAY(Value,
			   					   array->Values,
								   oldCapacity,
								   array->Capacity);
	}

	// Add the new Value to the dynamic array of Values.
	array->Values[array->Count] = value;
	array->Count++;
}


void FreeValueArray(ValueArray* array)
{
	FREE_ARRAY(Value, array->Values, array->Capacity);
	InitValueArray(array); // Leave the Values array in well-known state.
}


void PrintValue(Value value)
{

	// This conditional compilation is part of the NAN boxing optimization added in chapter 30 in the book.
	// See the comments on the NAN_BOXING constant in Common.h for more.
#ifdef NAN_BOXING

	//if (IS_BOOL(value))
	if (IS_BOOL(value))
	{
		printf(AS_BOOL(value) ? "true" : "false");
	}
	else if (IS_NIL(value))
	{
		printf("nil");
	}
	else if (IS_NUMBER(value))
	{
		printf("%g", AS_NUMBER(value));
	}
	else if (IS_OBJ(value))
	{
		PrintObject(value);
	}

#else

	switch (value.Type)
	{
		case VAL_BOOL:
			printf(AS_BOOL(value) ? "true" : "false");
			break;

		case VAL_NIL:
			printf("nil");
			break;

		case VAL_NUMBER:
			printf("%g", AS_NUMBER(value));
			break;

		case VAL_OBJ:
			PrintObject(value);
			break;
	} // End switch

#endif

}


bool ValuesEqual(Value a, Value b)
{

	// This conditional compilation is part of the NAN boxing optimization added in chapter 30 in the book.
	// See the comments on the NAN_BOXING constant in Common.h for more.
#ifdef NAN_BOXING

	if (IS_NUMBER(a) && IS_NUMBER(b))
	{
		return AS_NUMBER(a) == AS_NUMBER(b);
	}

#else

	if (a.Type != b.Type)
		return false;

	switch (a.Type)
	{
		case VAL_BOOL:
			return AS_BOOL(a) == AS_BOOL(b);
		case VAL_NIL:
			return true;
		case VAL_NUMBER:
			return AS_NUMBER(a) == AS_NUMBER(b);
		case VAL_OBJ:
			return AS_OBJ(a) == AS_OBJ(b);

		default:
			return false; // Unreachable
	} // End switch

#endif

}

