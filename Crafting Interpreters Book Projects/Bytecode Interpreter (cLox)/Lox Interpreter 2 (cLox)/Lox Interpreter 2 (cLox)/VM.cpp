#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// cLox includes.
#include "Common.h"
#include "Compiler.h"
#include "Debug.h"
#include "Object.h"
#include "Memory.h"
#include "VM.h"




VM vm;




static void ResetStack()
{
	vm.StackTop = vm.Stack;
	vm.FrameCount = 0;
	vm.OpenUpValues = NULL;
}


static void RuntimeError(const char* format, ...)
{
	printf("RUNTIME ERROR: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fputs("\n", stderr);

	for (int i = vm.FrameCount - 1; i >= 0; i--)
	{
		CallFrame* frame = &vm.Frames[i];
		ObjFunction* function = frame->Closure->Function;
		size_t instruction = frame->IP - function->Chunk.Code - 1; // The instruction pointer is pointing at the next instruction to be executed. So we subtract one here to reference the previous one (the instruction that failed to cause the runtime error).	
		fprintf(stderr, "    [Line %d] in ", function->Chunk.Lines[instruction]);
		if (function->Name == NULL)
		{
			fprintf(stderr, "script\n");
		}
		else
		{
			fprintf(stderr, "%s()\n", function->Name->Chars);
		}
	} // End for

	ResetStack();
}


static void DefineNativeFunction(const char* name, NativeFn function)
{
	Push(OBJ_VAL(CopyString(name, (int)strlen(name))));
	Push(OBJ_VAL(NewNativeFunction(function)));
	TableSet(&vm.Globals, AS_STRING(vm.Stack[0]), vm.Stack[1]);
	Pop();
	Pop();
}


static Value ClockNative(int argCount, Value* args)
{
	return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}


void InitVM()
{
	ResetStack();
	vm.Objects = NULL;
	vm.BytesAllocated = 0;
	vm.NextGC = 1024 * 1024;

	// This group of items is used by the cLox garbage collector. See chapter 26 in the book.
	vm.GrayCount = 0;
	vm.GrayCapacity = 0;
	vm.GrayStack = NULL;

	InitTable(&vm.Globals);
	InitTable(&vm.Strings);

	vm.InitString = NULL;
	vm.InitString = CopyString("init", 4);

	// Define native functions. When invoked in Lox, these just call native C/C++ functions.
	DefineNativeFunction("clock", ClockNative);
}


void FreeVM()
{
	FreeTable(&vm.Globals);
	FreeTable(&vm.Strings);

	vm.InitString = NULL;

	FreeObjects();
}


void Push(Value value)
{
	*vm.StackTop = value;
	vm.StackTop++;
}


Value Pop()
{
	vm.StackTop--;
	return *vm.StackTop;
}


// Gets a value from the stack without popping it off.
static Value Peek(int distance)
{
	return vm.StackTop[-1 - distance];
}


/// <summary>
/// Creates a new call stack frame for a function being called.
/// </summary>
/// <param name="closure">The closure containing the function being called. See chapter 25 in the book.</param>
/// <param name="argCount">The number of arguments the function expects.</param>
/// <returns></returns>
static bool Call(ObjClosure* closure, int argCount)
{
	if (argCount != closure->Function->Arity)
	{
		RuntimeError("Expected %d arguments, but got %d.", closure->Function->Arity, argCount);
		return false;
	}

	if (vm.FrameCount == FRAMES_MAX)
	{
		RuntimeError("Stack overflow.");
		return false;
	}


	CallFrame* frame = &vm.Frames[vm.FrameCount++];

	frame->Closure = closure;
	frame->IP = closure->Function->Chunk.Code;

	// Quoted from the book:
	// "The funny little - 1 is to account for stack slot zero which the compiler set aside for
	// "when we add methods later. The parameters start at slot one so we make the window
	// start one slot earlier to align them with the arguments."
	frame->Slots = vm.StackTop - argCount - 1;

	return true;
}


static bool CallValue(Value callee, int argCount)
{
	if (IS_OBJ(callee))
	{
		switch (OBJ_TYPE(callee))
		{
			case OBJ_BOUND_METHOD:
			{
				ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
				vm.StackTop[-argCount - 1] = bound->Receiver;
				return Call(bound->Method, argCount);
			}

			case OBJ_CLASS:
			{
				ObjClass* klass = AS_CLASS(callee);
				vm.StackTop[-argCount - 1] = OBJ_VAL(NewInstance(klass));

				Value initializer;
				if (TableGet(&klass->Methods, vm.InitString, &initializer))
				{
					return Call(AS_CLOSURE(initializer), argCount);
				}
				else if (argCount != 0) // This class has no initializer so it is not allowed for it to have arguments.
				{
					RuntimeError("Expected 0 arguments but got %d.", argCount);
					return false;
				}

				return true;
			}

			case OBJ_CLOSURE: // This replaced OBJ_FUNCTION here in chapter 25 of the book.
			{
				return Call(AS_CLOSURE(callee), argCount);
			}

			case OBJ_NATIVE_FUNCTION:
			{
				NativeFn native = AS_NATIVE_FUNCTION(callee);
				Value result = native(argCount, vm.StackTop - argCount);
				vm.StackTop -= argCount + 1;
				Push(result);
				return true;
			}

			default:
				break; // Non-callable object type.

		} // End switch
	} // End if


	RuntimeError("You can only call functions and classes.");
	return false;
}


static bool InvokeFromClass(ObjClass* klass, ObjString* name, int argCount)
{
	Value method;
	if (!TableGet(&klass->Methods, name, &method))
	{
		RuntimeError("Undefined property '%s'.", name->Chars);
		return false;
	}

	return Call(AS_CLOSURE(method), argCount);
}


static bool Invoke(ObjString* name, int argCount)
{
	Value receiver = Peek(argCount);

	if (!IS_INSTANCE(receiver))
	{
		RuntimeError("Only instances have methods.");
		return false;
	}

	ObjInstance* instance = AS_INSTANCE(receiver);

	Value value;
	if (TableGet(&instance->Fields, name, &value))
	{
		vm.StackTop[-argCount - 1] = value;
		return CallValue(value, argCount);
	}

	return InvokeFromClass(instance->Klass, name, argCount);
}


static bool BindMethod(ObjClass* klass, ObjString* name)
{
	Value method;

	if (!TableGet(&klass->Methods, name, &method))
	{
		RuntimeError("Undefined property '%s'.", name->Chars);
		return false;
	}

	ObjBoundMethod* bound = NewBoundMethod(Peek(0), AS_CLOSURE(method));
	Pop();
	Push(OBJ_VAL(bound));
	return true;
}


static ObjUpValue* CaptureUpValue(Value* local)
{
	ObjUpValue* prevUpValue = NULL;
	ObjUpValue* upValue = vm.OpenUpValues;
	
	while (upValue != NULL && upValue->Location > local)
	{
		prevUpValue = upValue;
		upValue = upValue->Next;
	}

	if (upValue != NULL && upValue->Location == local)
	{
		return upValue;
	}


	ObjUpValue* createdUpValue = NewUpValue(local);
	createdUpValue->Next = upValue;

	if (prevUpValue == NULL)
	{
		vm.OpenUpValues = createdUpValue;
	}
	else
	{
		prevUpValue->Next = createdUpValue;
	}

	return createdUpValue;
}


static void CloseUpValues(Value* last)
{
	while (vm.OpenUpValues != NULL &&
		   vm.OpenUpValues->Location >= last)
	{
		ObjUpValue* upValue = vm.OpenUpValues;

		upValue->Closed = *upValue->Location;
		upValue->Location = &upValue->Closed;

		vm.OpenUpValues = upValue->Next;
	}
}


static void DefineClassMethod(ObjString* name)
{
	Value method = Peek(0);
	ObjClass* klass = AS_CLASS(Peek(1));
	TableSet(&klass->Methods, name, method);
	Pop();
}


static bool IsFalsey(Value value)
{
	return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}


static void Concatenate()
{
	ObjString* b = AS_STRING(Peek(0));
	ObjString* a = AS_STRING(Peek(1));

	int length = a->Length + b->Length;
	char* chars = ALLOCATE(char, length + 1);
	memcpy(chars, a->Chars, a->Length);
	memcpy(chars + a->Length, b->Chars, b->Length);
	chars[length] = '\0'; // Add a null character on the end.

	ObjString* result = TakeString(chars, length);
	Pop();
	Pop();
	Push(OBJ_VAL(result));
}


static InterpretResult Run()
{

	CallFrame* frame = &vm.Frames[vm.FrameCount - 1];


#define READ_BYTE() (*frame->IP++) // A macro that returns and then increments the value of the instruction pointer.
#define READ_SHORT() (frame->IP += 2, (uint16_t)((frame->IP[-2] << 8) | frame->IP[-1])) // Reads a two-byte (16-bit) operand from the byte code.
#define READ_CONSTANT() (frame->Closure->Function->Chunk.Constants.Values[READ_BYTE()]) // A macro to read a constant.
#define READ_STRING() AS_STRING(READ_CONSTANT())

// A macro that executes binary operations.
// We pop B off the stack first on purpose because it was pushed on after A was.
#define BINARY_OP(valueType, op) \
	do \
	{ \
		if (!IS_NUMBER(Peek(0)) || !IS_NUMBER(Peek(1))) \
		{ \
			RuntimeError("Operands must be numbers."); \
			return INTERPRET_RUNTIME_ERROR; \
		} \
		double b = AS_NUMBER(Pop()); \
		double a = AS_NUMBER(Pop()); \
		Push(valueType(a op b)); \
	} while (false)


#ifdef DEBUG_TRACE_EXECUTION
	printf("\n\n== Runtime Debug Output ==\n");
#endif


	// This is the most performance critical part of the virtual machine. The book keeps it
	// simple rather than going for top speed, hence the giant switch statement.
	// There are techniques to make it faster. The book suggests looking up
	// "direct threaded code", "jump table", or "computed goto" to learn about some of them.
	for (;;)
	{



#ifdef DEBUG_TRACE_EXECUTION
		
	printf("          ");


	#ifdef DEBUG_PRINT_STACK
		
		for (Value* slot = vm.Stack; slot < vm.StackTop; slot++)
		{
			printf("[ ");
			PrintValue(*slot);
			printf(" ]");
		}
		printf("\n");

	#endif

		DisassembleInstruction(&frame->Closure->Function->Chunk,
							   (int)(frame->IP - frame->Closure->Function->Chunk.Code)); // This expression passed as the second parameter calculates the relative offset of the instruction from the start of the bytecode chunk.

#endif



		uint8_t instruction;
		switch (instruction = READ_BYTE())
		{
			case OP_CONSTANT:
			{
				Value constant = READ_CONSTANT();
				Push(constant); // Push the value onto the value stack.
				break;
			}

			case OP_NIL:
			{
				Push(NIL_VAL);
				break;
			}

			case OP_TRUE:
			{
				Push(BOOL_VAL(true));
				break;
			}

			case OP_FALSE:
			{
				Push(BOOL_VAL(false));
				break;
			}

			case OP_POP:
			{
				Pop();
				break;
			}

			case OP_GET_LOCAL:
			{
				uint8_t slot = READ_BYTE();
				Push(frame->Slots[slot]);
				break;
			}

			case OP_SET_LOCAL:
			{
				uint8_t slot = READ_BYTE();
				frame->Slots[slot] = Peek(0);
				break;
			}

			case OP_GET_GLOBAL:
			{
				ObjString* name = READ_STRING();
				Value value;
				if (!TableGet(&vm.Globals, name, &value))
				{
					RuntimeError("Undefined variable '%s'.", name->Chars);
					return INTERPRET_RUNTIME_ERROR;
				}

				Push(value);
				break;
			}

			case OP_DEFINE_GLOBAL:
			{
				ObjString* name = READ_STRING();
				TableSet(&vm.Globals, name, Peek(0));
				Pop();
				break;
			}

			case OP_SET_GLOBAL:
			{
				ObjString* name = READ_STRING();
				if (TableSet(&vm.Globals, name, Peek(0)))
				{
					TableDelete(&vm.Globals, name);
					RuntimeError("Undefined variable '%s'.", name->Chars);
					return INTERPRET_RUNTIME_ERROR;
				}

				break;
			}

			case OP_GET_UPVALUE:
			{
				uint8_t slot = READ_BYTE();
				Push(*frame->Closure->UpValues[slot]->Location);
				break;
			}

			case OP_SET_UPVALUE:
			{
				uint8_t slot = READ_BYTE();
				*frame->Closure->UpValues[slot]->Location = Peek(0);
				break;
			}

			case OP_GET_PROPERTY:
			{
				if (!IS_INSTANCE(Peek(0)))
				{
					RuntimeError("Only instances have properties.");
					return INTERPRET_RUNTIME_ERROR;
				}

				ObjInstance* instance = AS_INSTANCE(Peek(0));
				ObjString* name = READ_STRING();

				Value value;
				if (TableGet(&instance->Fields, name, &value))
				{
					Pop(); // Instance
					Push(value);
					break;
				}

				if (!BindMethod(instance->Klass, name))
				{
					return INTERPRET_RUNTIME_ERROR;
				}

				break;
			}

			case OP_SET_PROPERTY:
			{
				if (!IS_INSTANCE(Peek(1)))
				{
					RuntimeError("Only instances have fields.");
					return INTERPRET_RUNTIME_ERROR;
				}

				ObjInstance* instance = AS_INSTANCE(Peek(1));
				TableSet(&instance->Fields, READ_STRING(), Peek(0));
				Value value = Pop();
				Pop();
				Push(value);
				break;
			}

			case OP_GET_SUPER:
			{
				ObjString* name = READ_STRING();
				ObjClass* superClass = AS_CLASS(Pop());

				if (!BindMethod(superClass, name))
				{
					return INTERPRET_RUNTIME_ERROR;
				}

				break;
			}

			case OP_EQUAL:
			{
				Value b = Pop();
				Value a = Pop();
				Push(BOOL_VAL(ValuesEqual(a, b)));
				break;
			}
			
			case OP_GREATER:
			{
				BINARY_OP(BOOL_VAL, >);
				break;
			}

			case OP_LESS:
			{
				BINARY_OP(BOOL_VAL, <);
				break;
			}
			
			case OP_ADD:
			{
				if (IS_STRING(Peek(0)) && IS_STRING(Peek(1)))
				{
					Concatenate();
				}
				else if (IS_NUMBER(Peek(0)) && IS_NUMBER(Peek(1)))
				{
					double b = AS_NUMBER(Pop());
					double a = AS_NUMBER(Pop());
					Push(NUMBER_VAL(a + b));
				}
				else
				{
					RuntimeError("Operands must be two numbers or two strings.");
					return INTERPRET_RUNTIME_ERROR;
				}

				break;
			}

			case OP_SUBTRACT:
			{
				BINARY_OP(NUMBER_VAL, -);
				break;
			}

			case OP_MULTIPLY:
			{
				BINARY_OP(NUMBER_VAL, *);
				break;
			}

			case OP_DIVIDE:
			{
				BINARY_OP(NUMBER_VAL, /);
				break;
			}

			case OP_NOT:
			{
				Push(BOOL_VAL(IsFalsey(Pop())));
				break;
			}

			case OP_NEGATE:
			{
				if (!IS_NUMBER(Peek(0)))
				{
					RuntimeError("Operand must be a number.");
					return INTERPRET_RUNTIME_ERROR;
				}

				Push(NUMBER_VAL(-AS_NUMBER(Pop())));
				break;
			}

			case OP_PRINT:
			{
				PrintValue(Pop());
				printf("\n");
				break;
			}

			case OP_JUMP:
			{
				uint16_t offset = READ_SHORT();
				frame->IP += offset;
				break;
			}

			case OP_JUMP_IF_FALSE:
			{
				uint16_t offset = READ_SHORT();

				if (IsFalsey(Peek(0)))
				{
					frame->IP += offset;
				}

				break;
			}

			case OP_LOOP:
			{
				uint16_t offset = READ_SHORT();

				frame->IP -= offset;

				break;
			}

			case OP_CALL:
			{
				int argCount = READ_BYTE();
				if (!CallValue(Peek(argCount), argCount))
				{
					return INTERPRET_RUNTIME_ERROR;
				}
				frame = &vm.Frames[vm.FrameCount - 1];
				break;
			}

			case OP_INVOKE:
			{
				ObjString* method = READ_STRING();
				int argCount = READ_BYTE();

				if (!Invoke(method, argCount))
				{
					return INTERPRET_RUNTIME_ERROR;
				}

				frame = &vm.Frames[vm.FrameCount - 1];
				break;
			}

			case OP_SUPER_INVOKE:
			{
				ObjString* method = READ_STRING();
				int argCount = READ_BYTE();
				ObjClass* superClass = AS_CLASS(Pop());

				if (!InvokeFromClass(superClass, method, argCount))
				{
					return INTERPRET_RUNTIME_ERROR;
				}

				frame = &vm.Frames[vm.FrameCount - 1];
				break;
			}

			case OP_CLOSURE:
			{
				ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
				ObjClosure* closure = NewClosure(function);
				Push(OBJ_VAL(closure));

				for (int i = 0; i < closure->UpValueCount; i++)
				{
					uint8_t isLocal = READ_BYTE();
					uint8_t index = READ_BYTE();
					if (isLocal)
					{
						closure->UpValues[i] = CaptureUpValue(frame->Slots + index);
					}
					else
					{
						closure->UpValues[i] = frame->Closure->UpValues[index];
					}
				}

				break;
			}


			case OP_CLOSE_UPVALUE:
			{
				CloseUpValues(vm.StackTop - 1);
				Pop();
				break;
			}

			case OP_RETURN:
			{
				// Grab the return value of the function that just finished from the stack and cache it in 'result'.
				Value result = Pop();
				CloseUpValues(frame->Slots);
				vm.FrameCount--;
				
				// Are we exiting out of the top level Lox code (in other words, is the program ending)?
				if (vm.FrameCount == 0)
				{
					Pop();
					return INTERPRET_OK;
				}

				vm.StackTop = frame->Slots;
				Push(result); // Now that the finished function's stuff has been removed from the stack, pop its return value back on.
				frame = &vm.Frames[vm.FrameCount - 1];
				break;				
			}

			case OP_CLASS:
			{
				Push(OBJ_VAL(NewClass(READ_STRING())));
				break;
			}

			case OP_INHERIT:
			{
				Value superClass = Peek(1);
				if (!IS_CLASS(superClass))
				{
					RuntimeError("Superclass must be a class.");
					return INTERPRET_RUNTIME_ERROR;
				}

				ObjClass* subClass = AS_CLASS(Peek(0));
				TableAddAll(&AS_CLASS(superClass)->Methods,
							&subClass->Methods);
				Pop(); // Subclass
				break;
			}

			case OP_METHOD: 
			{
				DefineClassMethod(READ_STRING());
				break;
			}

		} // end switch

	} // end for


#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP

} // end Run()


InterpretResult Interpret(const char* source)
{
	ObjFunction* function = Compile(source);
	
	if (function == NULL)
		return INTERPRET_COMPILE_ERROR;


	// You may notice we seem to do some pointless stack stuff here.
	// Namely, we push the function on the stack only to pop it back off, and then
	// push the full closure onto the stack instead. The book says in chapter 25:
	// "The code looks a little silly because we still push the original ObjFunction onto the stack. Then we
	// pop it after creating the closure, only to then push the closure. Why put the ObjFunction on there
	// at all? As usual, when you see weird stack stuff going on, it’s to keep the forthcoming garbage
	// collector aware of some heap-allocated objects."
	// NOTE: That refers to the garbage collector implemented into cLox in subsequent chapters.
	Push(OBJ_VAL(function));
	ObjClosure* closure = NewClosure(function);
	Pop();
	Push(OBJ_VAL(closure));
	Call(closure, 0);


	return Run();
}

