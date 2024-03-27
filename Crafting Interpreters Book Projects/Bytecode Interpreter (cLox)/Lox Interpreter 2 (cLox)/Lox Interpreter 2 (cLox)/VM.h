// This file contains code for the virtual machine.
//

#pragma once

// #ifndef cLox_VM_h
//	#define cLox_VM_h

// cLox includes.
#include "Object.h"
#include "Table.h"
#include "Value.h"




#define FRAMES_MAX	 64 // This is the maximum allowed depth for our call stack.
#define STACK_MAX	(FRAMES_MAX * UINT8_COUNT) // The maximum size of the values stack in this virtual machine.




/// <summary>
/// This struct represents an executing function call.
/// </summary>
struct CallFrame
{
	ObjClosure* Closure; // The closure containing the function being executed. See chapter 25 in the book.
	uint8_t* IP; // An instruction pointer for the function being executed. It points to the currently executing instruction.
				 //		"We use a real C pointer pointing right into the middle of the bytecode array
				 //		instead of something like an integer index because it’s faster to dereference a pointer
				 //		than look up an element in an array by index."		
	Value* Slots; // Points to the first slot in the stack that this function can use.
};


struct VM
{
	CallFrame Frames[FRAMES_MAX]; // The call frame stack. This keeps track of executing function calls.
	int FrameCount; // The number of CallFrames currently in the call frame stack.
	
	Value Stack[STACK_MAX]; // The value stack for holding temporary values in memory as code executes.
	Value* StackTop; // A pointer to the value at the top of the value stack.
					 // We use a direct pointer here, just like with the instruction pointer, because it's
					 // faster than accessing the value via an array index.
					 // Also just like the instruction pointer, this pointer always points to the array element
					 // just beyond the top value on the stack.
	Table Globals; // Stores global variables defined by the Lox script being executed.
	Table Strings; // Stores interned strings. See the "String Interning" section of chapter 20 in the
				   // book: https://craftinginterpreters.com/hash-tables.html
	ObjString* InitString; // The name class initializer methods will use internally.
	ObjUpValue* OpenUpValues; // Linked list of UpValues that have not been moved to the heap yet (in other words, they refer to variables that are
							  // still alive on the stack).

	size_t BytesAllocated; // Tracks how much heap memory the VM has allocated.
	size_t NextGC; // When BytesAllocated reaches this threshold, the garbage collector is triggered and this threshold gets updated.
				   // See chapter 26 in the book. Note that the garbage collector gets run every time something is allocated if the DEBUG_STRESS_GC
				   // symbol is defined in Common.h.
	Obj* Objects; // Keeps references to all Lox objects that we still have in memory.

	// These are used by the cLox garbage collector. See chapter 26 in the book.
	int GrayCount; // Number of objects in the gray stack.
	int GrayCapacity; // Max number of objects that can fit in the gray stack.
	Obj** GrayStack; // Holds references to all "reachable" objects the garbage collector has found. We need to look at references inside them find
				     // more "reachable" objects that should not be garbage collected.
};


enum InterpretResult
{
	INTERPRET_OK,
	INTERPRET_COMPILE_ERROR,
	INTERPRET_RUNTIME_ERROR,
};




extern VM vm;




void InitVM();
void FreeVM();

InterpretResult Interpret(const char* source);

// Value stack operations.
void Push(Value value);
Value Pop();

// #endif

