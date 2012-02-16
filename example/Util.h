#ifndef _UTIL_H_
#define _UTIL_H_

//TODO: Replace with forward decleration?
#include <contest_interface.h>
#include <db_cxx.h>

// Compares two attributes
int attcmp(const Attribute &a, const Attribute &b);

// Compares two keys
int keycmp(const Key &a, const Key &b);

// Compares two Berkeley DB keys (used for the b-tree)
int keycmp(Db *db, const Dbt *a, const Dbt *b);

// Returns 0 if every attribute of key a is smaller than or equal to key b
// or a value > 0 if the attribute with index i in a is greater than in b
bool CheckBounds(const Key &a, const Key &b, int *index = NULL);

// Copys key b to key a
void CopyKey(Key &a, const Key &b);

void CopyRecord(Record *dst, Record *src);

// Deletes a key
void DeleteKey(Key *key);
  
// Deletes a record
void DeleteRecord(Record **record);

// Frees all memory that is used by dbt
void release(const Dbt *dbt);

#endif // _UTIL_H_
