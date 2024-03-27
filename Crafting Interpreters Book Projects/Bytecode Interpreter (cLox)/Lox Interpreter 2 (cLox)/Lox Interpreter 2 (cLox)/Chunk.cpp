#include <stdlib.h>

// cLox includes.
#include "Chunk.h"
#include "Memory.h"
#include "VM.h"




void InitChunk(Chunk* chunk)
{
	chunk->Count = 0;
	chunk->Capacity = 0;
	chunk->Code = NULL;
	chunk->Lines = NULL;
	InitValueArray(&chunk->Constants);
}


void FreeChunk(Chunk* chunk)
{
	FREE_ARRAY(uint8_t, chunk->Code, chunk->Capacity);
	FREE_ARRAY(int, chunk->Lines, chunk->Capacity);
	FreeValueArray(&chunk->Constants);
	InitChunk(chunk); // Leave the chunk in well-known state.
}


void WriteChunk(Chunk* chunk, uint8_t byte, int line)
{
	// Expand the dynamic array if necessary.
	if (chunk->Capacity < chunk->Count + 1)
	{
		int oldCapacity = chunk->Capacity;
		chunk->Capacity = GROW_CAPACITY(oldCapacity);
		chunk->Code = GROW_ARRAY(uint8_t, 
								 chunk->Code,
								 oldCapacity,
								 chunk->Capacity);
		chunk->Lines = GROW_ARRAY(int,
								  chunk->Lines,
								  oldCapacity,
								  chunk->Capacity);
	}


	// Add the new byte to the bytecode dynamic array.
	chunk->Code[chunk->Count] = byte;
	chunk->Lines[chunk->Count] = line;
	chunk->Count++;
}


int AddConstant(Chunk* chunk, Value value)
{
	Push(value);
	WriteValueArray(&chunk->Constants, value);
	Pop();

	// Return the index where the new constant was added into the array.
	return chunk->Constants.Count - 1;
}