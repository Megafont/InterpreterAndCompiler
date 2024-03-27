// This file defines some memory management stuff.
//

#pragma once

//#ifndef cLox_Memory_h
//	#define cLox_Memory_h

// cLox includes.
#include "Common.h"
#include "Object.h"




// A macro to allocate a new array on the heap.
#define ALLOCATE(type, count) \
	(type*)Reallocate(NULL, 0, sizeof(type) * (count))


// A macro that frees a piece of memory by resizing it to 0 bytes.
#define FREE(type, pointer)		Reallocate(pointer, sizeof(type), 0);


// A macro used to determine the new size of a dynamic array that is being expanded.
#define GROW_CAPACITY(capacity) \
	((capacity) < 8 ? 8 : (capacity) * 2)


// A macro to deallocate an array.
#define FREE_ARRAY(type, pointer, oldCount) \
	Reallocate(pointer, sizeof(type) * (oldCount), 0)


// A macro used to reallocate a dynamic array. It calls our Reallocate() function and casts
// the returned void* back to the correct type.
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
	(type*)Reallocate(pointer, sizeof(type) * (oldCount), \
		sizeof(type) * (newCount))




/// <summary>
/// This function resizes a dynamic array.
/// </summary>
/// <remarks>
/// This function behaves in the following ways depending on the oldSize and newSize parameters:
/// ----------------------------------------------------------------------------------------------------
/// oldSize		newSize					Operation
/// ----------------------------------------------------------------------------------------------------
///	0			Non-zero				Allocate new block.
///	Non-zero	0						Free allocation.
///	Non-zero	Smaller than oldSize	Shrink existing allocation.	(shrink the dynamic array)
///	Non-zero	Larger than oldSize		Grow existing allocation.	(expand the dynamic array)
/// ----------------------------------------------------------------------------------------------------
/// </remarks>
/// <param name="pointer">Pointer to the array.</param>
/// <param name="oldsize">The current size of the array.</param>
/// <param name="newSize">The new size of the array.</param>
/// <returns>A pointer to the reallocated array.</returns>
void* Reallocate(void* pointer, size_t oldSize, size_t newSize);


void MarkObject(Obj* object);
void MarkValue(Value value);

/// <summary>
/// This function is essentially the brain of the garbage collector. See chapter 26 in the book.
/// </summary>
void CollectGarbage();


/// <summary>
/// Free's all objects that are still in memory.
/// </summary>
void FreeObjects();

//#endif
