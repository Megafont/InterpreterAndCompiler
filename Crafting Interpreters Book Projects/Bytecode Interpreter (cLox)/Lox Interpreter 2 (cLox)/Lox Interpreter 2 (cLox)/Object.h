// This file defines our sturct used for storing larger Lox values like strings and instances
// on the heap.
//

#pragma once

//#ifndef cLox_Object_h
//	#define cLox_Object_h

// cLox includes.
#include "Common.h"
#include "Chunk.h"
#include "Table.h"
#include "Value.h"




// This macro gets the type of an object value.
#define OBJ_TYPE(value)		(AS_OBJ(value)->Type)

// Macros for checking the type of a Lox object value.
#define IS_BOUND_METHOD(value)		IsObjType(value, OBJ_BOUND_METHOD)
#define IS_CLASS(value)				IsObjType(value, OBJ_CLASS)
#define IS_CLOSURE(value)			IsObjType(value, OBJ_CLOSURE)
#define IS_FUNCTION(value)			IsObjType(value, OBJ_FUNCTION)
#define IS_INSTANCE(value)			IsObjType(value, OBJ_INSTANCE)
#define IS_NATIVE_FUNCTION(value)	IsObjType(value, OBJ_NATIVE_FUNCTION)
#define IS_STRING(value)			IsObjType(value, OBJ_STRING)


// These macros cast an Obj pointer to another type. See the comments for the Obj struct.
#define AS_BOUND_METHOD(value)		((ObjBoundMethod*) AS_OBJ(value))
#define AS_CLASS(value)				((ObjClass*) AS_OBJ(value))
#define AS_CLOSURE(value)			((ObjClosure*) AS_OBJ(value))
#define AS_FUNCTION(value)			((ObjFunction*) AS_OBJ(value))
#define AS_INSTANCE(value)			((ObjInstance*) AS_OBJ(value))
#define AS_NATIVE_FUNCTION(value)	(((ObjNativeFunction*) AS_OBJ(value))->Function)
#define AS_STRING(value)			((ObjString*) AS_OBJ(value))
#define AS_CSTRING(value)			(((ObjString*) AS_OBJ(value))->Chars)




// An enumeration of the object types that can be stored in the Value struct.
enum ObjType
{
	OBJ_BOUND_METHOD, // Represents a cLox class method bound to the parent class.
	OBJ_CLASS, // Represents a cLox class.
	OBJ_CLOSURE, // Represents a closure. See chapter 25 in the book.
	OBJ_FUNCTION, // Represents a user-defined Lox function
	OBJ_INSTANCE, // Represents an instance of a cLox class.
	OBJ_NATIVE_FUNCTION, // Represents a native C/C++ function
	OBJ_STRING, // Represents a string.
	OBJ_UPVALUE, // See chapter 25 in the book.
};


// A basic struct for representing an object in memory.
//
// NOTE: A pointer to a more specific type, like ObjString, can be cast to
//       type Obj, because they both begin with the same data. This is because the
//		 first field of ObjString is an Obj struct. So its memory footprint begins with
//		 an Obj struct. Likewise, an Obj can be cast to an ObjString AS LONG AS the
//		 object actually is an ObjString. So basically, this is a clever way of using
//		 pointers to fake struct inheritance.
struct Obj
{
	ObjType Type; // The type of this cLox object.
	bool IsMarked; // Indicates if the garbage collector has marked this object as "reachable". In other words, the cLox program still has access to it, and therefore it should not be garbage collected. See chapter 26 in the book.
	struct Obj* Next; // The next object in the linked list.
};


// A more specialized object struct for representing a Lox function.
struct ObjFunction
{
	Obj Obj; // The cLox Obj struct representing this cLox function.
	int Arity; // The number of parameters the function expects.
	int UpValueCount; // The number of UpValues this function has. See chapter 25 in the book.
	Chunk Chunk; // Holds the compiled bytecode of the function since we decided not to compile the entire Lox program into a single monolithic bytecode chunk.
	ObjString* Name; // The function's name.
};


typedef Value(*NativeFn)(int argCount, Value* args);

// A more specialized object struct representing a native function.
struct ObjNativeFunction
{
	Obj Obj; // The cLox Obj struct representing this cLox native function.
	NativeFn Function; // The native C/C++ function to call.
};


// A more specialized object struct for representing a Lox string in memory.
struct ObjString
{
	Obj Obj; // The cLox Obj struct representing this cLox string.
	int Length; // The length of this cLox string.
	char* Chars; // The character array storing this cLox string's raw text.
	uint32_t Hash; // The hash code for this cLox string.
};


// A more specialized object struct that represents an upvalue. See chapter 25 in the book.
// An upvalue represents a variable that gets captured by one or more closures.
// When this happens, the variable gets stored in the 'Closed' field of this struct
// to keep it around after it goes out of scope and thus no longer exists on the stack.
struct ObjUpValue
{
	Obj Obj; // The cLox Obj struct representing this upvalue.
	Value* Location; // A pointer to the variable this upvalue represents.
	Value Closed; // Stores the value of the variable if it no longer lives on the stack. In other words, it went out of scope. So its stored in this field now if there is still a reference to it in one or more closures. Thus it is on the heap now.
	ObjUpValue* Next; // The next UpValue in the linked list.
};


// A more specialized object struct that represents a closure. See chapter 25 in the book.
struct ObjClosure
{
	Obj Obj; // The cLox Obj struct representing this cLox closure.
	ObjFunction* Function; // The function this closure is associated with.
	ObjUpValue** UpValues; // The upvalues in this closure. These are not owned by the closure since they may be referenced by multiple closures.
	int UpValueCount; // The number of upvalues in this closure.
};


// Represents a method bound to a class.
struct ObjBoundMethod
{
	Obj Obj; // The cLox Obj struct representing this class.
	Value Receiver;
	ObjClosure* Method; // The closure for this method.
};


struct ObjClass
{
	Obj Obj; // The cLox Obj struct representing this class.
	ObjString* Name; // The name of this class.
	Table Methods; // The methods in this class.
};


struct ObjInstance
{
	Obj Obj; // The cLox Obj struct representing this class.
	ObjClass* Klass; // The class the instance is a member of.
	Table Fields; // The fields this cLox class instance contains.
};




ObjBoundMethod* NewBoundMethod(Value receiver, ObjClosure* method);
ObjClass* NewClass(ObjString* name);
ObjInstance* NewInstance(ObjClass* klass);

ObjClosure* NewClosure(ObjFunction* function);
ObjUpValue* NewUpValue(Value* slot);

ObjFunction* NewFunction();
ObjNativeFunction* NewNativeFunction(NativeFn function);

ObjString* TakeString(char* chars, int length);
ObjString* CopyString(const char* chars, int length);

void PrintObject(Value value);


static inline bool IsObjType(Value value, ObjType type)
{
	return IS_OBJ(value) && AS_OBJ(value)->Type == type;
}

//#endif