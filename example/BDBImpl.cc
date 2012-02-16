/*
Copyright (c) 2011 TU Dresden - Database Technology Group

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author: Lukas M. Maas <Lukas_Michael.Maas@mailbox.tu-dresden.de>

Current version: 1.0 (released December 14, 2011)

Version history:
  - 1.0 Initial release (December 14, 2011) 
*/

/** @file
An implementation of the contest interface using Oracle's Berkeley DB (http://www.oracle.com/technology/products/berkeley-db).

It uses a naive approach to map multidimensional keys to one dimensional keys (it concatenates all key attributes to form a one dimensional key). Hence range and partial-match queries are really slow. 
*/
#define HAVE_CXX_STDHEADERS 1
#include <db_cxx.h>

#include <stdio.h>
#include <iostream>
#include <map>
#include <pthread.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <sstream>

#include <contest_interface.h>

#include "ConnectionManager.h"
//#include "Mutex.h"
#include "Index.h"
#include "Iterator.h"
#include "Util.h"

#define LINE(x) //std::cout<<x<<std::endl<<std::flush
/**
Starts a new transaction and sets the corresponding handle (tx).

@see contest_interface.h for details
*/
ErrorCode BeginTransaction(Transaction **tx){
  LINE("BEGIN");
  if(tx == NULL)
    return kErrorGenericFailure;

  try{
    // Start the new transaction with isolation level read committed
		ConnectionManager::getInstance().env()->txn_begin(NULL, (DbTxn**) tx, DB_READ_COMMITTED );
	} catch(DbMemoryException &e){
    return kErrorOutOfMemory;
  } catch (DbException &e) {
			return kErrorGenericFailure;
	}

  return kOk;
};

/**
 Aborts the given transaction and rolls back all changes
 made during the course of this transaction.
 
 @see contest_interface.h for details
 */
ErrorCode AbortTransaction(Transaction **tx){
  LINE("ABORT");

  // Check that the given transaction is valid
  if((tx == NULL) || (*tx == NULL))
	  return kErrorTransactionClosed;
  
  try{
    // Abort the transaction and reset the handle
	  ((DbTxn*) (*tx))->abort();
    IndexManager::getInstance().CloseTransaction(*tx);
    (*tx) = NULL;
  } catch(DbException &e){
	  return kErrorGenericFailure;
  }
  
  return kOk;
};

/**
 Ends the given transaction and persists all changes
 made during the course of this transaction.

 @see contest_interface.h for details
 */
ErrorCode CommitTransaction(Transaction **tx){
  LINE("COMMIT");

  // Check that the given transaction is valid
  if((tx == NULL) || (*tx == NULL))
	  return kErrorTransactionClosed;
  
  try{
    // Commit the transaction
	  ((DbTxn*) *tx)->commit(0);
    IndexManager::getInstance().CloseTransaction(*tx);
    (*tx) = NULL;
  } catch(DbDeadlockException &e){
    return kTransactionAborted;
  }
  catch(DbException &e) {
    if(AbortTransaction(tx) == kOk)
      return kTransactionAborted;
        
		return kErrorGenericFailure;
	}
  return kOk;
};


/*
Creates an empty index.

@see contest_interface.h for details
*/
ErrorCode CreateIndex(const char* name, uint8_t column_count, KeyType types){
  LINE("CREATE");

  // Check that the input values are valid
  if((name == NULL) || (strlen(name) == 0) || (column_count == 0) || (types == NULL))
    return kErrorGenericFailure;

  try{
  	Db db(ConnectionManager::getInstance().env(),0);
	  
    // Allow duplicates for this index
    db.set_flags(DB_DUP);

    // Create the new index
    db.open(NULL, 		        // Transaction pointer
				NULL, 					      // File name (NULL, because we want the index to be in-memory)
				name,					        // Logical db name
				DB_BTREE,				      // Database type (we use b-tree)
				DB_CREATE | DB_EXCL,  // Open flags
				0                     // File mode (defaults)
				);
  } catch (DbException &e){
	  if(e.get_errno() == EEXIST)
		  return kErrorIndexExists;
    else if(e.get_errno() == ENOMEM)
      return kErrorOutOfMemory;
	  else
		  return kErrorGenericFailure;
  }
  
  // Insert new Index into the index map
  IndexManager::getInstance().Insert(name,new IndexStructure(column_count, types));
  return kOk;
}

/**
Opens an index specified by its name to be used by the current thread

@see contest_interface.h for details
*/
ErrorCode OpenIndex(const char* name, Index **idx){
  LINE("OPEN INDEX");
  // Check that the given name is valid
  if((name == NULL) || (strlen(name) == 0))
    return kErrorGenericFailure;
  
	try{
    // Open the index
    if(Index::Open(name,idx) == kErrorUnknownIndex){
      return kErrorUnknownIndex;
    };
  } catch (DbException &e) {
    // Clean up...
    delete *idx;
    *idx = NULL;
		// And return an appropriate error
    if(e.get_errno() == ENOENT)
			return kErrorUnknownIndex;
    else if(e.get_errno() == ENOMEM)
      return kErrorOutOfMemory;
		else
			return kErrorGenericFailure;
  }

  return kOk;
}

/**
Closes an specified index on this thread.

@see contest_interface.h for Details
*/
ErrorCode CloseIndex(Index **idx){
  LINE("CLOSE INDEX");
  // Check that the given index handle is valid
  if((idx == NULL) || (*idx == NULL))
	  return kErrorUnknownIndex;
  
  ErrorCode result = kOk;
  
  // If the index handle has been closed already,
  // but was not deleted, delete it
  if(((*idx)->closed()))
    result = kErrorUnknownIndex;

  try {
	  // Close the index
    delete *idx;
	  *idx = NULL;
  } catch (DbException &e) {
		if(e.get_errno() == EINVAL){
      *idx = NULL;
			return kErrorUnknownIndex;
		} else {
			return kErrorGenericFailure;
    }
	}
  LINE("$CLOSE");
  return result;
}

/*
Deletes an given index and frees all its resources.

@see contest_interface.h for details
*/
ErrorCode DeleteIndex(const char* name){
  LINE("DELETE INDEX");
  // Check that the given name is valid
  if((name == NULL) || (strlen(name) == 0))
    return kErrorGenericFailure;
  
  try{
    ErrorCode err;
    // Try to erase the index structure (closes open db handles)
    if((err = IndexManager::getInstance().Remove(name)) != kOk)
      return err;

    // And remove the respective Berkeley DB database
    int res = ConnectionManager::getInstance().env()->dbremove(NULL, NULL,
          name, DB_NOSYNC | DB_AUTO_COMMIT | DB_LOG_NO_DATA);
  } catch (DbException &e){
	  if(e.get_errno() == ENOENT)
		  return kErrorUnknownIndex;
	  else
		  return kErrorGenericFailure;
  }

  return kOk;
}

/**
Inserts a record (representing a multidimensional key and an associated payload)
into the index.

@see contest_interface.h for details
*/
ErrorCode InsertRecord(Transaction *tx,Index *idx, Record *record){
  LINE("INSERT RECORD");
  // Check that all input values are valid
  if((idx == NULL) || (idx->closed()))
		return kErrorUnknownIndex;
  
  if(!idx->Compatible(record))
    return kErrorIncompatibleKey;
	try {
		ErrorCode err;
    // Perform the database put
    if((err = idx->Insert(tx,record)) != kOk)
      return err;
 	}catch (DbDeadlockException &e){
    return kErrorDeadlock;
  } catch (DbException &e){
    if(e.get_errno() == ENOMEM)
      return kErrorOutOfMemory;
	  return kErrorGenericFailure;
	}

  return kOk;
}

/**
Searches for a key/value combination given as a record and updates its value.

@see contest_interface.h for details
*/
ErrorCode UpdateRecord(Transaction *tx, Index *idx, Record *record, Block *new_payload, uint8_t flags){
    LINE("UPDATE RECORD");
  // Check that all input values are valid
  if((idx == NULL) || (idx->closed()))
		return kErrorUnknownIndex;
  
  if(!idx->Compatible(record))
    return kErrorIncompatibleKey;
  
  if(new_payload == NULL)
    return kErrorGenericFailure;
  
  try {
    // Update the record
    ErrorCode err = idx->Update(tx, record, new_payload, flags);
    if(err != 0)
      return err;
  } catch (DbDeadlockException &de) {
    return kErrorDeadlock;
  } catch (DbException &e) {
    if(e.get_errno() == ENOMEM)
      return kErrorOutOfMemory;
    return kErrorGenericFailure;
  }
  LINE("$UPDATE");
  return kOk;
}

/**
Searches for a record and removes it from the index

@see contest_interface.h for details
*/
ErrorCode DeleteRecord(Transaction *tx, Index *idx, Record *record, uint8_t flags){
    LINE("DELETE RECORD");
  // Check that all input values are valid
  if((idx == NULL) || (idx->closed()))
		return kErrorUnknownIndex;
  
  if(!idx->Compatible(record))
    return kErrorIncompatibleKey;

  try {
    // Delete the record
    ErrorCode err = idx->Delete(tx, record, flags);
    if(err != 0)
      return err;
  } catch (DbDeadlockException &de) {
    return kErrorDeadlock;
  } catch (DbException &e) {
    return kErrorGenericFailure;
  }
  return kOk;

}

/**
Returns an \ref Iterator that starts at the first value of the given minimum
multidimensional key.

@see contest_interface.h for details
*/
ErrorCode GetRecords(Transaction *tx,Index *idx, Key min_keys, Key max_keys, Iterator **it){
  LINE("GET RECORDS");  
  // Check that all input values are valid
  if((idx == NULL) || (idx->closed()))
		return kErrorUnknownIndex;
  
  if(!idx->Compatible(min_keys) || !idx->Compatible(max_keys))
    return kErrorIncompatibleKey;
  
  if(it == NULL)
    return kErrorGenericFailure;

  try {
    // Create the new Iterator
    *it = new Iterator(tx,idx,min_keys,max_keys);
  } catch (DbDeadlockException &de) {
    if(*it)
      (*it)->Close();
    return kErrorDeadlock;
  } catch (DbException &e) {
    if(*it)
      (*it)->Close();
    return kErrorGenericFailure;
  }
  
  return kOk;
}

/**
Moves the iterator to the next record or reports an end of range (by returning
an \ref kErrorNotFound status), if the maximum multidimensional key is exceeded.

@see contest_interface.h for details
*/
ErrorCode GetNext(Iterator *it, Record** record){
  LINE("GET NEXT");
  // Check that all input values are valid
  if(record == NULL)
    return kErrorGenericFailure;

  if((it == NULL) || (it->closed()))
    return kErrorIteratorClosed;
    
  try {
    // Fetch the next record
    if(it->Next()){
      // If the iterator reached it's end, no record was found
      if(it->end()){
        //delete *record;
        *record = NULL;
        return kErrorNotFound;
      } else {
        // Set the record to the retrieved record
        *record = it->value();
      }
    } else {
      return kErrorGenericFailure;
    }
  } catch (DbDeadlockException &de) {
    return kErrorDeadlock;
  } catch (DbException &e) {
    return kErrorGenericFailure;
  }
  return kOk;
}

/**
Closes the given iterator and frees all of its resources.

@see contest_interface.h for details
*/
ErrorCode CloseIterator(Iterator **it){
  LINE("CLOSE ITERATOR");
  // Check that all input values are valid
  if((it == NULL) || (*it == NULL)){
    return kErrorIteratorClosed;
  }
  
  ErrorCode result = kOk;

  // If the record is already closed we just clean up
  if((*it)->closed())
    result = kErrorIteratorClosed;
  else
    (*it)->Close();

  delete *it;
  *it = NULL;

  return result;
}







