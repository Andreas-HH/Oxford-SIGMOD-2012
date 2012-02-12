/*
Copyright (c) 2011 TU Dresden - Database Technology Group

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author: Lukas M. Maas <Lukas_Michael.Maas@mailbox.tu-dresden.de>

Current version: 1.01 (released December 14, 2011)

Version history:
  - 1.01 (December 17, 2011)
    * Replaced the CreateRecord functions with their correct implementations
    * Modified tests to match the changes made to the contest's API
  - 1.0 Initial release (December 14, 2011) 
*/

/** @file
Defines test cases to ensure the correctness of implementations implementing
the contest interface.
*/

#include <contest_interface.h>
#include <common/macros.h>

#include "test_util.h"

#define PRIMARY_INDEX "primary_index"
#define SECONDARY_INDEX "secondary_index"

// Create new records for the primary and for the secondary index
Record* CreateRecord(const int32_t k_1, const int64_t k_2, const char* k_3, const char* payload);
Record* CreateRecord(const char* key, const char* payload);
Block* CreateBlock(const char* val){
  Block* block = new Block;
  block->data = (void*) val;
  block->size = strlen(val);
  return block;
};

bool BlockCmp(const Block &a, const Block &b){
  if(a.size == b.size){
    return ( memcmp(&a, &b, a.size) == 0);
  }
  return false;
};

// The Records that will be used for the TransactionTest (Test 2)
Record *a, *b, *c, *d, *e, *f;
bool run_visibility_test = false;

/**
Test 1: Test index creation
*/
TEST(IndexCreationTest){
  // Create the primary index
  KeyType keys = {kShort,kInt,kVarchar};
  ASSERT_EQUALS(kOk, CreateIndex(PRIMARY_INDEX, COUNT_OF(keys), keys), "Could not create primary index.");
  ASSERT_EQUALS(kErrorIndexExists, CreateIndex(PRIMARY_INDEX, COUNT_OF(keys), keys),
                "CreateIndex() does not return kErrorIndexExists while creating an index with a name already in use.");
  
  // Create the secondary index
  KeyType secondary_keys = {kVarchar};
  ASSERT_EQUALS(kOk, CreateIndex(SECONDARY_INDEX, COUNT_OF(secondary_keys), secondary_keys),
                "Could not create secondary index.");
};


/**
Test 2.1.1: Ensure that no data is visible from outside of a running transaction.
*/
static void* PrimaryIndexVisibilityTest(void*){
  /*Iterator *it = NULL;
  Index *idx = NULL;
  Transaction *tx = NULL;
  Record *x;
  ASSERT_EQUALS(kOk, BeginTransaction(&tx),"Could not begin Transaction");
  ASSERT_EQUALS(kOk, OpenIndex(PRIMARY_INDEX, &idx), "Could not open primary index.");
  while(run_visibility_test){
    ASSERT_EQUALS(kOk, GetRecords(tx, idx, a->key, a->key, &it), "Could not open iterator.");
    ASSERT_EQUALS(kErrorNotFound, GetNext(it, &x), "Uncommitted data should not be visible.");
    ASSERT_EQUALS(kOk, CloseIterator(&it), "Could not close iterator.");
  }
  ASSERT_EQUALS(kOk, CloseIndex(&idx), "Could not close primary index.");
  ASSERT_EQUALS(kOk, AbortTransaction(&tx),"Could not end Transaction");*/
  return 0;
}

/**
Test 2.1.2: Insert Records a, b, c into the primary index.

@param abort
  if true, the transaction will be aborted when it reaches its end
*/
static void* PrimaryIndexTransactionTest(void* arg){
  pthread_t visibility_check_thread;
  bool abort = *((bool*) arg);
  

  // Start a new thread to run the PrimaryIndexVisibilityTest
  run_visibility_test = true;
  int ret = pthread_create(&visibility_check_thread, NULL, PrimaryIndexVisibilityTest, NULL);
  ASSERT_EQUALS(0, ret, "Could not create a new thread for the PrimaryIndexVisibilityTest.");
  
  Transaction *tx;
  Index *idx;
  Iterator *it;

  // A new payload used for the update test
  Block* new_payload = CreateBlock("Sales");

  // Open a new transaction and insert the first 3 records into the primary index
  ASSERT_EQUALS(kOk, BeginTransaction(&tx), "Could not begin a new transaction.");
  ASSERT_EQUALS(kOk, OpenIndex(PRIMARY_INDEX, &idx), "Could not open primary index.");
  ASSERT_EQUALS(kOk, InsertRecord(tx, idx, a), "Could not insert record 'a' into the primary index.");
  ASSERT_EQUALS(kOk, InsertRecord(tx, idx, b), "Could not insert record 'b' into the primary index.");
  ASSERT_EQUALS(kOk, InsertRecord(tx, idx, c), "Could not insert record 'c' into the primary index.");

  // Try a range query (should return record a and b)
  Record *tmp;
  ASSERT_EQUALS(kOk, GetRecords(tx, idx, a->key, b->key, &it), "Could not open the iterator to retrieve the record.");
  ASSERT_EQUALS(kOk, GetNext(it, &tmp), "Could not retrieve record 'a' using the created iterator.");
  ASSERT_NOT_EQUAL(NULL, tmp, "The returned record 'a' is NULL");
  if(tmp!=NULL){
    ASSERT_EQUALS(0, BlockCmp(a->payload, tmp->payload), "The payloads of the original and retrieved record 'a' do not match.");
  }
  ASSERT_EQUALS(kOk, GetNext(it, &tmp), "Could not retrieve record 'b' using the created iterator.");
  if(tmp!=NULL)
    ASSERT_EQUALS(0, BlockCmp(b->payload, tmp->payload), "The payloads of the original and retrieved record 'b' do not match.");
  
  // Try to update record b
  ASSERT_EQUALS(kOk, UpdateRecord(tx, idx, a, new_payload, 0), "Could not update data inside the primary index.");

  // As we want to persist the data (which will make it visible to other threads)
  // we need to stop the PrimaryIndexVisibilityTest
  run_visibility_test = false;
  pthread_join(visibility_check_thread, NULL);

  // Finish the execution of this transaction
  if(abort){
    ASSERT_EQUALS(kOk, AbortTransaction(&tx), "Could not abort running transaction.");
  } else {
    ASSERT_EQUALS(kOk, CommitTransaction(&tx), "Could not commit running transaction.");
  }

  ASSERT_EQUALS(kOk, CloseIndex(&idx), "Could not close primary index.");

  return 0;
};

/**
Test 2.2: Insert records d, e and f into the secondary index.
 
To ensure that all duplicates are handled correctly, duplicates will be inserted.
*/
static void* SecondaryIndexTransactionTest(void*){
  Transaction *tx;
  Index *idx;

  // Open a new transaction and insert these records into the primary index
  ASSERT_EQUALS(kOk, BeginTransaction(&tx), "Could not begin a new transaction.");
  ASSERT_EQUALS(kOk, OpenIndex(SECONDARY_INDEX, &idx), "Could not open secondary index.");

  // Insert a duplicate
  ASSERT_EQUALS(kOk, InsertRecord(tx, idx, d), "Could not insert record 'd' into the secondary index.");
  ASSERT_EQUALS(kOk, InsertRecord(tx, idx, d), "Could not insert record 'd' (duplicate) into the secondary index.");
  ASSERT_EQUALS(kOk, InsertRecord(tx, idx, e), "Could not insert record 'e' into the secondary index.");
  ASSERT_EQUALS(kOk, InsertRecord(tx, idx, f), "Could not insert record 'f' into the secondary index.");

  // Assure that duplicates are existent
  Record *tmp;
  Iterator *it;
  ASSERT_EQUALS(kOk, GetRecords(tx, idx, d->key, d->key, &it), "Could not open the iterator to retrieve the records.");
  ASSERT_EQUALS(kOk, GetNext(it, &tmp), "Could not get the original record 'd' from the iterator.");
  ASSERT_EQUALS(kOk, GetNext(it, &tmp), "Could not get the duplicate record 'd' from the iterator.");
  ASSERT_EQUALS(kErrorNotFound, GetNext(it, &tmp), "The iterator returned a record that should not be existent.");
  
  ASSERT_EQUALS(kOk, DeleteRecord(tx, idx, e, 0), "Could not delete record 'e'."); 
  ASSERT_EQUALS(kErrorIncompatibleKey, DeleteRecord(tx, idx, c, 0), "Deleting incompatible records should not be possible.");
  
  ASSERT_EQUALS(kOk, CloseIndex(&idx), "Could not close secondary index.");
  ASSERT_EQUALS(kOk, CommitTransaction(&tx), "Could not commit running transaction.");
  return 0;
};

/**
Test 2: Test transactions

This test creates 4 concurrent threads that will work on 2 indexes in parallel.
*/
TEST(TransactionTest){

  pthread_t primary_index_thread, secondary_index_thread, aborting_primary_index_thread;
  
  // Create the sample records
  a = CreateRecord(2241, 23, "Harry", "Finance");
  b = CreateRecord(3401, 42, "Sally", "CEO");
  c = CreateRecord(3415, 31, "George", "Sales");
  d = CreateRecord("Programming", "is 10% science, 20% ingenuity, and 70% getting the ingenuity to work with the science.");
  e = CreateRecord("A SQL Classic", "A SQL query walks into a bar and sees two tables."
                   "After doing a full scan of the other tables, he walks up to them and says 'Can I join you?'.");
  f = CreateRecord("How many programmers does it take to change a light bulb?", "None. That's a hardware problem.");

  
  // Create the 4 threads to be executed in parallel
  bool abort_transaction = false;
  int ret;
  ret = pthread_create(&primary_index_thread, NULL, PrimaryIndexTransactionTest, &abort_transaction);
  ASSERT_EQUALS(0, ret, "Could not create a new thread for the PrimaryIndexTransactionTest.");
  ret = pthread_create(&secondary_index_thread, NULL, SecondaryIndexTransactionTest, NULL);
  ASSERT_EQUALS(0, ret, "Could not create a new thread for the SecondaryIndexTransactionTest.");
  bool abort_txn = true;
  ret = pthread_create(&aborting_primary_index_thread, NULL, PrimaryIndexTransactionTest, &abort_txn);
  ASSERT_EQUALS(0, ret, "Could not create a new thread for the aborting PrimaryIndexTransactionTest.");

  // Wait for the threads to exit
  pthread_join(primary_index_thread, NULL);
  pthread_join(secondary_index_thread, NULL);
  pthread_join(aborting_primary_index_thread, NULL);

  // Check that there are no duplicates inside the primary index
  Record *tmp, *next;
  Index *idx;
  Iterator *it;
  Attribute* wc[] = {NULL, NULL, NULL};
  Key wildcard;
  wildcard.value = wc;
  wildcard.attribute_count = COUNT_OF(wc);
  int record_count = 0;
  ASSERT_EQUALS(kOk, OpenIndex(PRIMARY_INDEX, &idx), "Could not open primary index.");


  GetRecords(NULL, idx, wildcard, wildcard, &it);
  while((kErrorNotFound != GetNext(it, &next)) && (record_count < 4)){
    record_count++;
  }

  ASSERT_NOT_EQUAL(4, record_count, "Seems there were more (or less) records retrieved than have been inserted (Maybe duplicates have been inserted).");
  ASSERT_EQUALS(kOk, CloseIndex(&idx), "Could not close primary index.");
  // Check that all data inserted using secondary_index_thread is existent
  ASSERT_EQUALS(kOk, OpenIndex(SECONDARY_INDEX, &idx), "Could not open secondary index.");
  ASSERT_EQUALS(kOk, GetRecords(NULL, idx, d->key, d->key, &it), "Could not retrieve record 'd' from the secondary index.");
  ASSERT_EQUALS(kOk, GetNext(it, &tmp), "Could not retrieve record 'd' (duplicate) from the secondary index.");
  ASSERT_EQUALS(kOk, GetRecords(NULL, idx, f->key, f->key, &it), "Could not retrieve record 'f' from the secondary index.");
  ASSERT_EQUALS(kOk, CloseIndex(&idx), "Could not close secondary index.");
};


/**
Test 3: Try calling functions from outside of a transaction context
*/
TEST(OutsideInvocationTest){
  Index *idx;
  Iterator *it;
  Record *g, *h, *hc;
  Block* new_payload = CreateBlock("...");

  // Create a record for the primary index
  g = CreateRecord(47,11,"Make me a Sandwich. - What? Make it yourself. - SUDO make me a sandwich. - Okay.","sudo make sandwich");
  
  // Try inserting, deleting and updating an index without using a transaction
  ASSERT_EQUALS(kOk, OpenIndex(PRIMARY_INDEX, &idx), "Could not open primary index.");
  ASSERT_EQUALS(kOk, InsertRecord(NULL, idx, g), "Could not insert data into the primary index.");
  ASSERT_EQUALS(kOk, UpdateRecord(NULL, idx, g, new_payload,0), "Could not update data inside the primary index.");

  // Try to get the inserted record
  ASSERT_EQUALS(kOk, GetRecords(NULL, idx, g->key, g->key, &it), "Could not open the iterator to retrieve the record.");
  ASSERT_EQUALS(kOk, GetNext(it, &h), "Could not get the record from the iterator."); 
  hc = new Record;
  CopyRecordX(hc, h);
  ASSERT_EQUALS(kOk, CloseIterator(&it), "Could not get the record from the iterator.");

  // Delete the record
  ASSERT_EQUALS(kOk, DeleteRecord(NULL, idx, hc, 0), "Could not delete record from primary index.");
  DeleteRecordX(&hc);
  // Close the index
  ASSERT_EQUALS(kOk, CloseIndex(&idx), "Could not close primary index.");
};


/**
 * Test 4: Test transaction abort
 *
 * Ensures that no data is visible after aborting a transaction.
 */
TEST(TransactionAbortTest){
  Transaction* tx;
  Index* idx;
  Iterator *it;
  Record *a, *b;
  
  // Create a sample record
  a = CreateRecord(10, 42, "My code's compiling.", "The #1 programmer excuse for legitimately slacking off.");
  Key k = a->key;
  
  // Start a new transaction and insert some data
  ASSERT_EQUALS(kOk, BeginTransaction(&tx), "Could not begin a new transaction.");
  ASSERT_EQUALS(kOk, OpenIndex(PRIMARY_INDEX, &idx), "Could not open primary index.");
  ASSERT_EQUALS(kOk, InsertRecord(tx, idx, a), "Could not insert data into the primary index.");
  // Try to get the inserted record
  ASSERT_EQUALS(kOk, GetRecords(tx, idx, a->key, a->key, &it), "Could not open the iterator to retrieve the record.");
  ASSERT_EQUALS(kOk, GetNext(it, &b), "Could not get the record from the iterator.");

  // Check, that it is the same record
  if(b != NULL)
    ASSERT_EQUALS(0,BlockCmp(a->payload, b->payload), "Inserted and retrieved record do not match.");
  
  // Abort transaction...
  ASSERT_EQUALS(kOk, AbortTransaction(&tx), "Could not abort running transaction.");
  
  // Check again
  ASSERT_EQUALS(kOk, GetRecords(NULL, idx, a->key, a->key, &it), "Could not open a new iterator.");
  ASSERT_EQUALS(kErrorNotFound, GetNext(it,&a), "The record was found, even though it has been deleted.");
  ASSERT_EQUALS(kOk, CloseIterator(&it), "Could not Close iterator.");

  ASSERT_EQUALS(kOk, CloseIndex(&idx), "Could not close primary index.");

};

/**
Test 5: Test index deletion
 
Deletes the indexes created by the other tests.
*/
TEST(IndexDeletionTest){
  Transaction *tx;
  Index *idx;
  Record *a = CreateRecord(1,2,"b","record a");
  Record *b;
  Iterator *it;
  
  ASSERT_EQUALS(kOk, BeginTransaction(&tx), "Could not begin a new transaction.");
  ASSERT_EQUALS(kOk, OpenIndex(PRIMARY_INDEX, &idx), "Could not open primary index.");
  ASSERT_EQUALS(kOk, InsertRecord(tx,idx,a),"Could not insert record a");
  ASSERT_EQUALS(kOk, InsertRecord(tx,idx,a),"Could not insert record a");
  ASSERT_EQUALS(kOk, CloseIndex(&idx), "Could not close primary index.");
  ASSERT_EQUALS(kOk, CommitTransaction(&tx), "Could commit transaction");
  // Attempting to delete an index that contains uncommitted data
  ASSERT_EQUALS(kOk, BeginTransaction(&tx), "Could not begin a new transaction.");
  ASSERT_EQUALS(kOk, OpenIndex(PRIMARY_INDEX, &idx), "Could not open primary index.");
  ASSERT_EQUALS(kOk, InsertRecord(tx,idx,CreateRecord(1,2,"b","record a")),"Could not insert record a");
  ASSERT_EQUALS(kOk, AbortTransaction(&tx), "Could not abort transaction.");
  
  // Delete an existing index
  ASSERT_EQUALS(kOk, DeleteIndex(PRIMARY_INDEX), "Could not delete the primary index.");

  // Try to delete an index that does not exist
  ASSERT_EQUALS(kErrorUnknownIndex, DeleteIndex(PRIMARY_INDEX),
                "DeleteIndex() does not return kErrorUnknownIndex, when attempting to delete an index which does not exit.");

  // Delete the secondary index
  ASSERT_EQUALS(kOk, DeleteIndex(SECONDARY_INDEX), "Could not delete the secondary index.");

};

/**
Creates a new record for the primary index

@param k_1
  the first attribute of the composite key (SHORT)

@param k_2
  the second attribute of the composite key (INT)

@param k_3
  the third attribute of the composite key (VARCHAR)

@param payload
  the payload to be assigned to the record

@return
  a pointer to the created record
*/
Record* CreateRecord(const int32_t k_1, const int64_t k_2, const char* k_3, const char* payload){
   Attribute** a = new Attribute*[3];
   a[0] = new Attribute;
   a[1] = new Attribute;
   a[2] = new Attribute;

   a[0]->type = kShort;
   a[1]->type = kInt;
   a[2]->type = kVarchar;

   a[0]->short_value = k_1;
   a[1]->int_value = k_2;
   strcpy(a[2]->char_value, k_3);

   Record* record = new Record;
   record->key.value = a;
   record->key.attribute_count = 3;
   record->payload = *CreateBlock(payload);
   return record;
};

/**
Creates a new record for the secondary index

@param key
  the key (VARCHAR)

@param payload
  the payload to be assigned to the record

@return
  a pointer to the created record
*/
Record* CreateRecord(const char* key, const char* payload){
   Attribute** a = new Attribute*;
   *a = new Attribute;
   (*a)->type = kVarchar;
   strcpy((*a)->char_value, key);

   Record* record = new Record;
   record->key.value = a;
   record->key.attribute_count = 1;
   record->payload = *CreateBlock(payload);
   return record;
};

