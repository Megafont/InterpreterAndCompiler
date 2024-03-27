#include <stdlib.h>

// cLox includes.
#include "Compiler.h"
#include "Memory.h"
#include "VM.h"


#ifdef DEBUG_LOG_GC
	#include <stdio.h>
	#include "Debug.h"
#endif




#define GC_HEAP_GROW_FACTOR		2




void* Reallocate(void* pointer, size_t oldSize, size_t newSize)
{
	vm.BytesAllocated += newSize - oldSize;


	// Quoted from the book:
	// "Whenever we call reallocate() to acquire more memory, we force a collection to
	// run. The if check is because reallocate() is also called to free or shrink an
	// allocation. We don’t want to trigger a GC for that—in particular because the GC itself
	// will call reallocate() to free memory."
	if (newSize > oldSize)
	{
#ifdef DEBUG_STRESS_GC
		CollectGarbage();
#endif

		if (vm.BytesAllocated > vm.NextGC)
		{
			CollectGarbage();
		}
	}


	if (newSize == 0)
	{
		free(pointer);
		return NULL;
	}

	void* result = realloc(pointer, newSize);

	// Check if memory allocation failed.
	if (result == NULL)
		exit(1);

	return result;
}


/// <summary>
/// This function is used by the cLox garbage collector. See chapter 26 in the book.
/// </summary>
/// <param name="object">The object to mark.</param>
void MarkObject(Obj* object)
{
	if (object == NULL)
		return;
	if (object->IsMarked) // If the object is already marked, then skip it since that means we've already looked at it.
		return;


#ifdef DEBUG_LOG_GC
	printf("%p mark object: ", (void*)object);
	PrintValue(OBJ_VAL(object));
	printf("\n");
#endif


	object->IsMarked = true;


	if (vm.GrayCapacity < vm.GrayCount + 1)
	{
		vm.GrayCapacity = GROW_CAPACITY(vm.GrayCapacity);
		vm.GrayStack = (Obj**)realloc(vm.GrayStack,
									  sizeof(Obj*) * vm.GrayCapacity);;

		// Check that memory was allocated successfully. Otherwise, the cLox interpreter exits to the operating system with an error code.
		if (vm.GrayStack == NULL)
			exit(1);
	}

	vm.GrayStack[vm.GrayCount++] = object;
}


/// <summary>
/// This function is used by the cLox garbage collector. See chapter 26 in the book.
/// </summary>
/// <param name="value">The Value to mark.</param>
void MarkValue(Value value)
{
	// Is the value actually an object on the heap?
	// If not, then the garbage collector can ignore it since its on the stack.
	if (IS_OBJ(value))
		MarkObject(AS_OBJ(value));
}


/// <summary>
/// This function is used by the cLox garbage collector. See chapter 26 in the book.
/// </summary>
static void MarkArray(ValueArray* array)
{
	for (int i = 0; i < array->Count; i++)
	{
		MarkValue(array->Values[i]);
	}
}


/// <summary>
/// This function is used by the cLox garbage collector. See chapter 26 in the book.
/// When an object is "blackened", it means we've traversed its internal references
/// to find all of the ones that are still "reachable", and thus should not be garbage collected.
/// </summary>
static void BlackenObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
	printf("%p blacken object: ", (void*)object);
	PrintValue(OBJ_VAL(object));
	printf("\n");
#endif


	switch (object->Type)
	{
		case OBJ_BOUND_METHOD:
		{
			ObjBoundMethod* bound = (ObjBoundMethod*)object;
			MarkValue(bound->Receiver);
			MarkObject((Obj*)bound->Method);
			break;
		}

		case OBJ_CLASS:
		{
			ObjClass* klass = (ObjClass*)object;
			MarkObject((Obj*)klass->Name);
			MarkTable(&klass->Methods);
			break;
		}

		case OBJ_CLOSURE:
		{
			ObjClosure* closure = (ObjClosure*)object;
			MarkObject((Obj*)closure->Function);
			for (int i = 0; i < closure->UpValueCount; i++)
			{
				MarkObject((Obj*)closure->UpValues[i]);
			}
			break;
		}

		case OBJ_FUNCTION:
		{
			ObjFunction* function = (ObjFunction*)object;
			MarkObject((Obj*)function->Name);
			MarkArray(&function->Chunk.Constants);
			break;
		}

		case OBJ_INSTANCE:
		{
			ObjInstance* instance = (ObjInstance*)object;
			MarkObject((Obj*)instance->Klass);
			MarkTable(&instance->Fields);
			break;
		}

		case OBJ_UPVALUE:
		{
			MarkValue(((ObjUpValue*)object)->Closed);
			break;
		}

		case OBJ_NATIVE_FUNCTION:
		case OBJ_STRING:
			break;

	} // End switch
}


void FreeObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
	printf("%p free object: ", (void*)object);
	PrintValue(OBJ_VAL(object));
	printf("\n");
#endif


	switch (object->Type)
	{
		case OBJ_BOUND_METHOD:
		{
			FREE(ObjBoundMethod, object);
			break;
		}

		case OBJ_CLASS:
		{
			ObjClass* klass = (ObjClass*)object;
			FreeTable(&klass->Methods);
			FREE(ObjClass, object);
			break;
		}

		case OBJ_CLOSURE:
		{
			ObjClosure* closure = (ObjClosure*)object;
			FREE_ARRAY(ObjUpValue*, closure->UpValues, closure->UpValueCount);
			FREE(ObjClosure, object);

			// We don't clear the function pointed to by the closure here. This is because
			// it may still be referenced by other elements in the Lox program.

			break;
		}

		case OBJ_FUNCTION:
		{
			ObjFunction* function = (ObjFunction*)object;
			FreeChunk(&function->Chunk);
			FREE(ObjFunction, object);
			break;
		}

		case OBJ_INSTANCE:
		{
			ObjInstance* instance = (ObjInstance*)object;
			FreeTable(&instance->Fields);
			FREE(ObjInstance, object);
			break;
		}

		case OBJ_NATIVE_FUNCTION:
		{
			FREE(ObjNativeFunction, object);
			break;
		}

		case OBJ_STRING:
		{
			ObjString* string = (ObjString*)object;
			FREE_ARRAY(char, string->Chars, string->Length + 1);
			FREE(ObjString, object);
			break;
		}

		case OBJ_UPVALUE:
		{
			FREE(ObjUpValue, object);
			break;
		}

	} // End switch
}


/// <summary>
/// This function is used by the cLox garbage collector. See chapter 26 in the book.
/// </summary>
static void MarkRoots()
{
	for (Value* slot = vm.Stack; slot < vm.StackTop; slot++)
	{
		MarkValue(*slot);
	}


	for (int i = 0; i < vm.FrameCount; i++)
	{
		MarkObject((Obj*)vm.Frames[i].Closure);
	}


	for (ObjUpValue* upValue = vm.OpenUpValues; upValue != NULL; upValue = upValue->Next)
	{
		MarkObject((Obj*)upValue);
	}


	MarkTable(&vm.Globals);
	MarkCompilerRoots();
	MarkObject((Obj*)vm.InitString);
}


/// <summary>
/// This function is used by the cLox garbage collector. See chapter 26 in the book.
/// </summary>
static void TraceReferences()
{
	while (vm.GrayCount > 0)
	{
		Obj* object = vm.GrayStack[--vm.GrayCount];
		BlackenObject(object);
	}
}


/// <summary>
/// This function is used by the cLox garbage collector. See chapter 26 in the book.
/// </summary>
static void Sweep()
{
	Obj* previous = NULL;
	Obj* object = vm.Objects;


	// Iterate through all heap objects in the VM's linked list.
	while (object != NULL)
	{
		if (object->IsMarked)
		{
			object->IsMarked = false; // Reset this flag so it's ready to go the next time the garbage collector runs.
			previous = object;
			object = object->Next;
		}
		else // The IsMarked flag is not set, meaning this object was never reached. Therefore, it is no longer "reachable" and needs to be garbage collected.
		{
			Obj* unreached = object;
			object = object->Next;
			if (previous != NULL)
			{
				previous->Next = object;
			}
			else // If previous is NULL, it means we're freeing the first object in the linked list. Therefore, the VM's pointer needs to be updated to point to the new first object in the list.
			{
				vm.Objects = object;
			}

			FreeObject(unreached);
		}
	} // End while
}


/// <summary>
/// This function is essentially the brain of the garbage collector. See chapter 26 in the book.
/// </summary>
void CollectGarbage()
{
#ifdef DEBUG_LOG_GC
	printf("-- gc (garbage collector) begin\n");
	size_t before = vm.BytesAllocated;
#endif


	MarkRoots(); // Find all "reachable" objects in the stack, and in the VM's internal references as well as in compiler ones.
	TraceReferences(); // Scan through all references contained in the "reachable" objects we just found to find more "reachable" objects.
	TableRemoveWhite(&vm.Strings); // Clean up strings in the VM's string table that are no longer "reachable".
	Sweep(); // Clean up objects that are no longer "reachable" and which should therefore be garbage collected.


	// Adjust the threshold for the next garbage collection. See chapter 26 in the book.
	vm.NextGC = vm.BytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
	printf("-- gc end\n");
	printf("   collected %zu bytes (from %zu to %zu). Next garbage collection when heap size reaches %zu bytes.\n",
		before - vm.BytesAllocated,
		before,
		vm.BytesAllocated,
		vm.NextGC);
#endif
}


void FreeObjects()
{
	Obj* object = vm.Objects;

	while (object != NULL)
	{
		Obj* next = object->Next;
		FreeObject(object);
		object = next;
	} // End while

	free(vm.GrayStack);
}
