// This file defines some debugging tools.
//

#pragma once




//#ifndef cLox_Debug_h
//	#define cLox_Debug_h

// cLox includes.
#include "Chunk.h"

void DisassembleChunk(Chunk* chunk, const char* name);
int DisassembleInstruction(Chunk* chunk, int offset);

void PrintDebugOutputKey(); // I added this.

//#endif
