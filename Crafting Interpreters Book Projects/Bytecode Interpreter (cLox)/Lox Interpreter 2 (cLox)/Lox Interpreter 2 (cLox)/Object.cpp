#include <stdio.h>
#include <string.h>

// cLox includes.
#include "Memory.h"
#include "Object.h"
#include "Table.h"
#include "Value.h"
#include "VM.h"




#define ALLOCATE_OBJ(type, objectType) \
	(type*)AllocateObject(sizeof(type), objectType)




static Obj* AllocateObject(size_t size, ObjType type)
{
	Obj* object = (Obj*)Reallocate(NULL, 0, size);
	
	object->Type = type;
	object->IsMarked = false;
	object->Next = vm.Objects;

	vm.Objects = object;


#ifdef DEBUG_LOG_GC
	printf("%p allocate %zu bytes for object of type %d\n", (void*)object, size, type);	
#endif

	return object;
}


static ObjString* AllocateString(char* chars, int length, uint32_t hash)
{
	ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
	
	string->Length = length;
	string->Chars = chars;
	string->Hash = hash;

	Push(OBJ_VAL(string));
	// Intern this new string. See the "String Interning" section of chapter 20 in the book:
	// https://craftinginterpreters.com/hash-tables.html
	TableSet(&vm.Strings, string, NIL_VAL);
	Pop();
	
	return string;
}


ObjBoundMethod* NewBoundMethod(Value receiver, ObjClosure* method)
{
	ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod, OBJ_BOUND_METHOD);

	bound->Receiver = receiver;
	bound->Method = method;

	return bound;
}


ObjClass* NewClass(ObjString* name)
{
	ObjClass* klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
	klass->Name = name;
	InitTable(&klass->Methods);

	return klass;
}


ObjInstance* NewInstance(ObjClass* klass)
{
	ObjInstance* instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
	instance->Klass = klass;
	InitTable(&instance->Fields);

	return instance;
}


ObjClosure* NewClosure(ObjFunction* function)
{
	ObjUpValue** upValues = ALLOCATE(ObjUpValue*, function->UpValueCount);

	for (int i = 0; i < function->UpValueCount; i++)
	{
		upValues[i] = NULL;
	}


	ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
	closure->Function = function;
	closure->UpValues = upValues;
	closure->UpValueCount = function->UpValueCount;

	return closure;
}


ObjUpValue* NewUpValue(Value* slot)
{
	ObjUpValue* upValue = ALLOCATE_OBJ(ObjUpValue, OBJ_UPVALUE);
	upValue->Closed = NIL_VAL;
	upValue->Location = slot;
	upValue->Next = NULL;

	return upValue;
}


/// <summary>
/// Creates and initializes a new Lox function object.
/// </summary>
/// <returns>A new, blank Lox function object.</returns>
ObjFunction* NewFunction()
{
	ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
	
	function->Arity = 0;
	function->UpValueCount = 0;
	function->Name = NULL;
	InitChunk(&function->Chunk);

	return function;
}


ObjNativeFunction* NewNativeFunction(NativeFn function)
{
	ObjNativeFunction* native = ALLOCATE_OBJ(ObjNativeFunction, OBJ_NATIVE_FUNCTION);
	native->Function = function;
	return native;
}


/// <summary>
/// This function generates hash codes for strings.
/// The book says we are using a hash function called FNV-1a:
/// http://www.isthe.com/chongo/tech/comp/fnv/
/// </summary>
/// <param name="key">The string to generate a hash code for.</param>
/// <param name="length">The length of the passed in string.</param>
/// <returns>The generated hash code.</returns>
static uint32_t HashString(const char* key, int length)
{
	uint32_t hash = 2166136261u;
	for (int i = 0; i < length; i++)
	{
		hash ^= (uint8_t)key[i];
		hash *= 16777619;
	}

	return hash;
}


/// <summary>
/// This does the same thing as CopyString, but without allocating new memory first.
/// </summary>
/// <param name="chars">The char array containing the characters to put in the string.</param>
/// <param name="length">The length of the string.</param>
/// <returns></returns>
ObjString* TakeString(char* chars, int length)
{
	uint32_t hash = HashString(chars, length);

	// Check if this string has already been interned.
	// See the "String Interning" section in chapter 20 of the book:
	// https://craftinginterpreters.com/hash-tables.html
	ObjString* interned = TableFindString(&vm.Strings, chars, length, hash);
	if (interned != NULL)
	{
		FREE_ARRAY(char, chars, length + 1);
		return interned;
	}

	return AllocateString(chars, length, hash);
}


ObjString* CopyString(const char* chars, int length)
{
	uint32_t hash = HashString(chars, length);

	// Check if this string has already been interned.
	// See the "String Interning" section in chapter 20 of the book:
	// https://craftinginterpreters.com/hash-tables.html
	ObjString* interned = TableFindString(&vm.Strings, chars, length, hash);
	if (interned != NULL)
		return interned;

	char* heapChars = ALLOCATE(char, length + 1);
	memcpy(heapChars, chars, length);
	heapChars[length] = '\0'; // Add a null character on the end of the string.
	return AllocateString(heapChars, length, hash);
}


static void PrintFunction(ObjFunction* function)
{
	// Is this the automatically generated main function that contains Lox code that is not
	// in a user-defined function?
	if (function->Name == NULL)
	{
		printf("<script>");
		return;
	}

	printf("<fn %s>", function->Name->Chars);
}


void PrintObject(Value value)
{
	switch (OBJ_TYPE(value))
	{
		case OBJ_BOUND_METHOD:
		{
			ObjBoundMethod* bound = AS_BOUND_METHOD(value);
			PrintFunction(bound->Method->Function);
			break;
		}

		case OBJ_CLASS:
			printf("%s class", AS_CLASS(value)->Name->Chars);
			break;

		case OBJ_CLOSURE:
			// We display closures in debug output the same way as for functions. The book
			// says this is because:
			// "From the user’s perspective, the difference between ObjFunction and ObjClosure is purely
			// a hidden implementation detail."
			PrintFunction(AS_CLOSURE(value)->Function);
			break;

		case OBJ_FUNCTION:
			PrintFunction(AS_FUNCTION(value));
			break;

		case OBJ_INSTANCE:
			printf("%s instance", AS_INSTANCE(value)->Klass->Name->Chars);
			break;

		case OBJ_NATIVE_FUNCTION:
			printf("<native fn>");
			break;

		case OBJ_STRING:
			printf("%s", AS_CSTRING(value));
			break;

		case OBJ_UPVALUE: // See chapter 25 in the book. This case should never run, but is here to keep the compiler happy.
			printf("upvalue");
			break;

	} // End switch
}

