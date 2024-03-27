#include <stdlib.h>
#include <string.h>

// cLox includes.
#include "Memory.h"
#include "Object.h"
#include "Table.h"
#include "Value.h"




#define TABLE_MAX_LOAD		0.75 // How full a hash table must get before we expand it.




void InitTable(Table* table)
{
	table->Count = 0;
	table->Capacity = 0;
	table->Entries = NULL;
}


void FreeTable(Table* table)
{
	FREE_ARRAY(Entry, table->Entries, table->Capacity);
	InitTable(table);
}


/// <summary>
/// Searches for a specified key in the specified hash table.
/// </summary>
/// <param name="entries">A pointer to the entries array of the table to search.</param>
/// <param name="capacity">The capacity of the table being searched.</param>
/// <param name="key">The key to search for in the hash table.</param>
/// <returns>The entry if one was found, or NULL otherwise.</returns>
static Entry* FindEntry(Entry* entries, int capacity, ObjString* key)
{
	// This line got replaced with a new one that achieves the same thing using bitmasking.
	// This optimization was added in chapter 30 of the book, because the modulus operator
	// is pretty slow.
	//uint32_t index = key->Hash % capacity;
	uint32_t index = key->Hash & (capacity - 1);


	Entry* tombstone = NULL;


	// This forever loop can't get stuck, because we'll never have a full table.
	// Our MAX_LOAD_FACTOR constant ensures this.
	for (;;)
	{
		Entry* entry = &entries[index];
		if (entry->Key == NULL)
		{
			if (IS_NIL(entry->Value))
			{
				// Empty entry.
				return tombstone != NULL ? tombstone : entry;
			}
			else
			{
				// We found a tombstone.
				if (tombstone == NULL)
					tombstone = entry;
			}
		}
		else if (entry->Key == key)
		{
			// We found the key.
			return entry;
		}

		// This line got replaced with a new one that achieves the same thing using bitmasking.
		// This optimization was added in chapter 30 of the book, because the modulus operator
		// is pretty slow.
		// index = (index + 1) % capacity;
		index = (index + 1) & (capacity - 1);
	} // End for
}


/// <summary>
/// Gets a value from the specified hash table.
/// </summary>
/// <param name="table">The hash table to search.</param>
/// <param name="key">The key to search for.</param>
/// <param name="value">This pointer is used to return the value if the entry was found.</param>
/// <returns>True if the specified entry was found in the hash table, or false otherwise.</returns>
bool TableGet(Table* table, ObjString* key, Value* value)
{
	if (table->Count == 0)
		return false;

	Entry* entry = FindEntry(table->Entries, table->Capacity, key);
	if (entry->Key == NULL)
		return false;

	*value = entry->Value;
	return true;
}


/// <summary>
/// This function allocates or resizes a hash table.
/// </summary>
/// <param name="table">The table to allocate memory for, or resize.</param>
/// <param name="capacity">The max number of items the table should be able to hold.</param>
static void AdjustCapacity(Table* table, int capacity)
{
	// Allocate memory for the table.
	Entry* entries = ALLOCATE(Entry, capacity);


	// Set the newly allocated table to default values.
	for (int i = 0; i < capacity; i++)
	{
		entries[i].Key = NULL;
		entries[i].Value = NIL_VAL;
	} // End for


	// Copy entries into the newly allocated memory.
	// This is needed for when we're expanding an existing table, rather than creating
	// a new one.
	table->Count = 0;
	for (int i = 0; i < table->Capacity; i++)
	{
		Entry* entry = &table->Entries[i];
		if (entry->Key == NULL)
			continue;

		// Find the slot where the entry will go in the new expanded table.
		Entry* dest = FindEntry(entries, capacity, entry->Key);

		// Copy the entry to that location.
		dest->Key = entry->Key;
		dest->Value = entry->Value;
		table->Count++;

	} // End for


	// Free the memory used by the old, smaller table.
	FREE_ARRAY(Entry, table->Entries, table->Capacity);


	// Set fields of the table struct.
	table->Entries = entries;
	table->Capacity = capacity;
}


/// <summary>
/// This function adds an entry to the specified hash table.
/// NOTE: If the key already exists in the table, then the value of that entry is overwritten
///		  with the passed in value.
/// </summary>
/// <param name="table">The table to add an entry to.</param>
/// <param name="key">The key of the key/value pair to add to the table.</param>
/// <param name="value">The value of the key/value pair to add to the table.</param>
/// <returns>True if a new entry was added.</returns>
bool TableSet(Table* table, ObjString* key, Value value)
{
	// Expand the hash table if necessary.
	if (table->Count + 1 > table->Capacity * TABLE_MAX_LOAD)
	{
		int capacity = GROW_CAPACITY(table->Capacity);
		AdjustCapacity(table, capacity);
	}

	Entry* entry = FindEntry(table->Entries, table->Capacity, key);
	bool isNewKey = entry->Key == NULL;
	if (isNewKey && IS_NIL(entry->Value)) // Increment table count only if the new item was put into a truly empty slot as opposed to a tombstone slot. See the comments in the TableDelete() implementation for more on this.
		table->Count++;

	entry->Key = key;
	entry->Value = value;

	return isNewKey;
}


bool TableDelete(Table* table, ObjString* key)
{
	if (table->Count == 0)
		return false;

	// Find the entry;
	Entry* entry = FindEntry(table->Entries, table->Capacity, key);
	if (entry->Key == NULL)
		return false;

	// Place a tombstone in the slot of the deleted entry.
	// This is necessary to stop the removal of the item from breaking the hash table.
	// See the "Deleting Entries" section in Chapter 20 of the book for more:
	// https://craftinginterpreters.com/hash-tables.html
	entry->Key = NULL;
	entry->Value = BOOL_VAL(true);
	return true;
}


/// <summary>
/// This function adds all entries in the first table to the second table.
/// </summary>
/// <param name="from">The table to copy from.</param>
/// <param name="to">The table to add to.</param>
void TableAddAll(Table* from, Table* to)
{
	for (int i = 0; i < from->Capacity; i++)
	{
		Entry* entry = &from->Entries[i];
		if (entry->Key != NULL)
		{
			TableSet(to, entry->Key, entry->Value);
		}
	} // End for
}


/// <summary>
/// This function is similar to FindEntry(), but is used in a more specialized scenario.
/// Specifically, this function does more robust comparison when searching than
/// FindEntry() does.
/// See the "Interning Strings" section in chapter 20 of the book for more:
/// https://craftinginterpreters.com/hash-tables.html
/// </summary>
/// <param name="table">The table to search.</param>
/// <param name="chars">The character array containing the raw string data.</param>
/// <param name="length">The length of the string.</param>
/// <param name="hash">The hash code for the string.</param>
/// <returns>The found string.</returns>
ObjString* TableFindString(Table* table, const char* chars, int length, uint32_t hash)
{
	if (table->Count == 0)
		return NULL;
		
	// This line got replaced with a new one that achieves the same thing using bitmasking.
	// This optimization was added in chapter 30 of the book, because the modulus operator
	// is pretty slow.
	// uint32_t index = hash % table->Capacity;
	uint32_t index = hash & (table->Capacity - 1);

	for (;;)
	{
		Entry* entry = &table->Entries[index];
		if (entry->Key == NULL)
		{
			// Stop if we find an empty non-tombstone entry. See the comments in the TableDelete() implementation for more.
			if (IS_NIL(entry->Value))
				return NULL;
		}
		else if (entry->Key->Length == length &&
			entry->Key->Hash == hash &&
			memcmp(entry->Key->Chars, chars, length) == 0)
		{
			// We found it.
			return entry->Key;
		}

		// This line got replaced with a new one that achieves the same thing using bitmasking.
		// This optimization was added in chapter 30 of the book, because the modulus operator
		// is pretty slow.
		// index = (index + 1) % table->Capacity;
		index = (index + 1) & (table->Capacity - 1);

	} // End for
}


void TableRemoveWhite(Table* table)
{
	for (int i = 0; i < table->Capacity; i++)
	{
		Entry* entry = &table->Entries[i];
		if (entry->Key != NULL && !entry->Key->Obj.IsMarked)
		{
			TableDelete(table, entry->Key);
		}
	}
}


void MarkTable(Table* table)
{
	for (int i = 0; i < table->Capacity; i++)
	{
		Entry* entry = &table->Entries[i];
		MarkObject((Obj*)entry->Key);
		MarkValue(entry->Value);
	}
}