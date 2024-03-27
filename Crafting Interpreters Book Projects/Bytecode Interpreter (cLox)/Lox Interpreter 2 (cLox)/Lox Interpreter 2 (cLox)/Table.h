// This file contains code for our hash table (think Dictionary in C#)
// implementation in chapter 20.
//

#pragma once

//#ifndef cLox_Table_h
//	#define cLox_Table_h

// cLox includes.
#include "Common.h"
#include "Value.h"




/// <summary>
/// This struct represents a key/value pair that we can store in our hash tables.
/// </summary>
struct Entry
{
	ObjString* Key;
	Value Value;
};


/// <summary>
/// This struct stores the data of a hash table.
/// </summary>
struct Table
{
	int Count; // The number of entries in the table.
	int Capacity; // The max number of entries that can fit in the table.
	Entry* Entries; // The table entries.
};




void InitTable(Table* table);
void FreeTable(Table* table);

bool TableGet(Table* table, ObjString* key, Value* value); // Gets a value from the specified hash table.
bool TableSet(Table* table, ObjString* key, Value value); // Add's an entry to the specified hash table.
bool TableDelete(Table* table, ObjString* key); // Deletes an entry from the specified hash table.
void TableAddAll(Table* from, Table* to); // Adds all entries in the first table to the second table.
ObjString* TableFindString(Table* table, const char* chars, int length, uint32_t hash); // Similar to FindEntry(), but with more robust string comparison.

void TableRemoveWhite(Table* table); // Used by the cLox garbage collector. See chapter 26 in the book.
void MarkTable(Table* table); // Used by the cLox garbage collector. See chapter 26 in the book.

// #endif
