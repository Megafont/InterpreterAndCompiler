// This file defines stuff involved with representing chunks of bytecode.
//

#pragma once

//#ifndef cLox_Chunk_h
//	#define cLox_Chunk_h

// cLox includes.
#include "Common.h"
#include "Value.h"



enum OpCodes
{
	OP_CONSTANT,
	OP_NIL,
	OP_TRUE,
	OP_FALSE,
	OP_POP,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_GET_GLOBAL,
	OP_DEFINE_GLOBAL,
	OP_SET_GLOBAL,
	OP_GET_UPVALUE,
	OP_SET_UPVALUE,
	OP_GET_PROPERTY,
	OP_SET_PROPERTY,
	OP_GET_SUPER,
	OP_EQUAL,
	OP_GREATER,
	OP_LESS,
	OP_ADD,
	OP_SUBTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,
	OP_NOT,	
	OP_NEGATE,
	OP_PRINT,
	OP_JUMP,
	OP_JUMP_IF_FALSE,
	OP_LOOP,
	OP_CALL,
	OP_INVOKE, // This is essentially a combination of the OP_GET_PROPERTY and OP_CALL instructions. It was added as an optimization in chapter 28 of the book.
	OP_SUPER_INVOKE, // This is essentially a combination of the OP_GET_SUPER and OP_CALL instructions. It was added as an optimization in chapter 29 of the book.
	OP_CLOSURE,
	OP_CLOSE_UPVALUE,
	OP_RETURN,
	OP_CLASS,
	OP_INHERIT,
	OP_METHOD,
};


// Used to store a chunk of bytecode in a dynamic array.
struct Chunk
{
	int Capacity; // The total number of elements in the array.
	int Count; // The number of elements in the array that are currently in use.
	uint8_t* Code; // A dynamic array that stores the instructions in this chunk of bytecode.
	int* Lines; // Stores the source code line number for each opcode in the 'code' array. This is very memory inefficient, because not every byte in the 'code' array is an opcode. Thus, many elements in this 'lines' array end up unused.
	ValueArray Constants; // A dynamic array that stores constant data values used by the instructions, such as numeric literals.
};




void InitChunk(Chunk* chunk);
void FreeChunk(Chunk* chunk);
void WriteChunk(Chunk* chunk, uint8_t byte, int line);
int AddConstant(Chunk* chunk, Value value);

//#endif







