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

Current version: 1.02 (released December 18, 2011)

Version history:
  - 1.02 (December 18, 2011)
    * Clarified requirements for concurrency control
    * Updated documentation for CloseIndex()
    * Clarified how duplicates need to be handled
  - 1.01 (December 17, 2011)
    * Removed scdb namespace to add C support to the interface
    * Updated all transactional functions to expect a pointer to the currently
      running transaction (this removes the dependency on a specific thread
      API like POSIX threads from implementations of this interface)
  - 1.0 Initial release (December 14, 2011) 
*/

/** @file
Defines the API that participants of the ACM SIGMOD 2012 Programming Contest
must implement.
See http://wwwdb.inf.tu-dresden.de/sigmod2012contest/ for details.
*/

#ifndef _CONTEST_INTERFACE_H_
#define _CONTEST_INTERFACE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
Specifies the maximum payload size in bytes that needs to be supported.
*/
#define MAX_PAYLOAD_LENGTH 4096

/**
Specifies the maximum length for a varchar attribute that needs to be supported.
*/
#define MAX_VARCHAR_LENGTH 512

/**
Status codes used to indicate the outcome of an API call.
*/
typedef enum ErrorCodes {
  /// The operation completed successfully
  kOk = 0,
  /// The transaction has been aborted
  kTransactionAborted,
  /// The operation was not completed because of a lack of free memory 
  kErrorOutOfMemory,
  /// The operation ran into a deadlock and had to be aborted
  kErrorDeadlock,
  /// An index with the given name already exists
  kErrorIndexExists,
  /// The requested index does not exist or has been closed already
  kErrorUnknownIndex,
  /// Could not access the given iterator as it has been closed already
  kErrorIteratorClosed,
  /// The requested record was not found
  kErrorNotFound,
  /// The operation was aborted because the transaction has been closed already
  kErrorTransactionClosed,
  /// The record that should be inserted or updated is not compatible with the given index
  kErrorIncompatibleRecord,
  /// Error code used to indicate an unspecified error behavior
  kErrorGenericFailure
} ErrorCode;

/**
There are three attribute types that need to be supported. 
*/
typedef enum AttributeTypes{
  /// A 32-bit integer type attribute - INT(4)
  kShort,
  /// A 64-bit integer type attribute - INT(8)
  kInt,
  /// A varchar with a length defined by \ref MAX_VARCHAR_LENGTH - VARCHAR(<MAX_VARCHAR_LENGTH>)
  kVarchar
} AttributeType;

/**
Represents an attribute of a specified type.

Multiple attributes (values of a defined type) form a key.
*/
typedef struct Attribute{
  /// Defines the type of this attribute
  AttributeType type; 

  /// Defines the value of this attribute
  union{
    int32_t short_value;
    int64_t int_value;
    char char_value[MAX_VARCHAR_LENGTH+1];
  } value;

} Attribute;

/**
A multidimensional key is defined by an array of one or more attributes.
*/
typedef Attribute** Key;

/**
Defines the type of a multidimensional key.

As a multidimensional key is formed by multiple attributes, the type of
a composite key is defined by an array of attribute types.
*/
typedef AttributeType KeyType[];

/**
Represents a record (key/value-combinations) of an index.

It is important that changes on a record variable will not change the values of the corresponding
record inside the index (to update a record inside an index you have to use UpdateRecord()).
Therefore your implementation is responsible for copying all required data when modifying an index.

Records are not necessarily unique, which means there might
be multiple records with the same key/value-combination. In this case it is
important, that an UpdateRecord()-call only updates on of these records, while all other
records with the same key and value will remain unchanged. The same also applies to DeleteRecord().
*/
typedef struct Record{
  /// Defines an array holding the multidimensional key of the record
  Key key;

  /// The value stored under that key as a null-terminated C string of not more than MAX_PAYLOAD_LENGTH bytes.
  char payload[MAX_PAYLOAD_LENGTH+1];
} Record;

/**
Represents an iterator and stores any necessary information
needed to identify the iterator across the whole system.

An iterator will be initialized with a set of record using a GetRecords() call
and is used to iterate over these records using GetNext().

\note
  This struct is implementation-specific and therefore not defined in this file.
*/
typedef struct Iterator Iterator;

/**
Represents a Transaction and stores any necessary information
to identify the transaction across the whole system.

Transactions are used to combine multiple operations to ensure all ACID properties are met.

As multiple transactions may be performed concurrently, it is in your responsibility to 
ensure that the data integrity of the respective indexes is not violated.

\note
  This struct is implementation-specific and therefore not defined in this file.
*/
typedef struct Transaction Transaction;

/**
Represents an Index and stores any necessary information
to identify the index across the whole system.

An index is defined by a name and a set of attribute types that form the type of the key.
It can hold an unlimited amount of records as long as they are compatible with that key type.

\note
  This struct is implementation-specific and therefore not defined in this file.
*/
typedef struct Index Index;

/**
Starts a new transaction and sets the corresponding handle (tx).

This function needs to allocate all necessary memory that is needed to
create the new transaction instance.


@param[out] tx 
  returns the newly initiated transaction

@return ErrorCode
  - \ref kOk
         if the transaction was successfully started
  - \ref kErrorDeadlock
         if the transaction ran into a deadlock and had to be aborted
  - \ref kErrorGenericFailure
         if the transaction could not be started for some other reason
*/
ErrorCode BeginTransaction(Transaction **tx);

/** Aborts the current transaction and rolls back all changes
made during the course of this transaction.

@param[in,out] tx 
  the transaction to be aborted

@return ErrorCode
  - \ref kOk
         if the transaction was successfully aborted
  - \ref kErrorDeadlock
         if the transaction was not aborted because of a deadlock
  - \ref kErrorTransactionClosed
         if there was no transaction to abort
  - \ref kErrorGenericFailure
         if the transaction could not be aborted for some other reason
*/
ErrorCode AbortTransaction(Transaction **tx);

/**
Ends the current transaction and persists all changes
made during the course of this transaction. Abort if
this is not possible.

@param[in,out] tx 
  the transaction to be commited

@return ErrorCode
  - \ref kOk
         if the transaction was successfully commited
  - \ref kTransactionAborted
         if the transaction could not be commited and had to be aborted
  - \ref kErrorDeadlock
         if the transaction was not commited because of a deadlock
  - \ref kErrorTransactionClosed
         if there was no transaction to commit
  - \ref kErrorGenericFailure
         if the transaction could not be commited for some other reason
*/
ErrorCode CommitTransaction(Transaction **tx);

/**
Creates an empty index with <column_count> columns of the types defined
by the given key type.

The key type needs to be an ordered list of attribute types, as Attributes
are not named and therefore are only identified by there ordering.

@param[in] name
  the name to identify this index - needs to be unique in a session

@param[in] attribute_count
  number of attributes that form the key type of this index

@param[in] type
  pointer to an array of <attribute_count> attribute types used to create the key
  of the new index.

@return ErrorCode
  - \ref kOk
         if the index was successfully created
  - \ref kErrorIndexExists
         if an index with the given name already exits
  - \ref kErrorOutOfMemory
         if the operation was not completed because of a lack of free memory 
  - \ref kErrorGenericFailure
         if the index could not be created for some other reason

*/
ErrorCode CreateIndex(const char* name, uint8_t attribute_count, KeyType type);

/**
Opens an index structure specified by its name to be used by the current thread.

An index may be used by multiple transactions at a time, therefore concurrency control
is necessary.

@param[in] name
  the unique name identifying the index to open

@param[out] idx
  return the opened index

@return ErrorCode
  - \ref kOk
         if the index was successfully opened
  - \ref kErrorUnknownIndex
         if an index with the given name has not been created
  - \ref kErrorGenericFailure
         if the index could not be created for some other reason
*/
ErrorCode OpenIndex(const char* name, Index **idx);

/**
Closes an specified index on this thread.

After calling this, the given index pointer will be invalidated and therefore
cannot be used by any thread.

If the given index has been already closed, the function will return
\ref kErrorUnknownIndex.

@param[in,out] idx
  the index being closed

@return ErrorCode
  - \ref kOk
         if the index was successfully closed
  - \ref kErrorUnknownIndex
         if the index has been closed already or never existed
  - \ref kErrorGenericFailure
         if the index could not be created for some other reason
*/
ErrorCode CloseIndex(Index **idx);

/**
Deletes the index given by its name. This should free all resources associated
with this index.

If the index is currently opened, it needs to be closed before deletion.

@param[in] name
  name of the index to be deleted

@return ErrorCode
  - \ref kOk
         if the index was successfully deleted
  - \ref kErrorUnknownIndex
         if an index with the given name does not exist
  - \ref kErrorGenericFailure
         if the index could not be deleted for some other reason
*/
ErrorCode DeleteIndex(const char* name);

/**
Inserts a record (representing a multidimensional key and an associated payload)
into the index.

It might be called from outside of a running transaction. In this case all
changes are committed immediately.

As changes to the source record should not affect the saved data, your
implementation is responsible for copying all necessary data.

@param[in,out] tx
  the transaction to be used (or NULL if called from outside of a running transaction)

@param[in,out] idx
  the index to insert to

@param[in] record
  the record to be inserted

@return ErrorCode
  - \ref kOk
         if the record was successfully inserted
  - \ref kErrorUnknownIndex
         if the index has been closed already or never existed
  - \ref kErrorIncompatibleRecord
         if the record that should be inserted is not compatible with the given index
  - \ref kErrorDeadlock
         if the call could not be completed because of a deadlock
  - \ref kErrorOutOfMemory
         if operation was not completed because of a lack of free memory
  - \ref kErrorGenericFailure
         if the record could not be inserted for some other reason
*/
ErrorCode InsertRecord(Transaction *tx, Index *idx, Record *record);

/**
Searches for a key/value combination given as a record and updates its value.
If the record was not found, an appropriate ErrorCode (\ref kErrorNotFound)
will be returned.

It might be called from outside of a running transaction. In this case all
changes are committed immediately.

@param[in,out] tx
  the transaction to be used (or NULL if called from outside of a running transaction)

@param[in,out] idx
  the index to modify

@param[in,out] old_record
  the record to be updated

@param[in] new_payload
  the new payload to be assigned to the given record
  
@return ErrorCode
  - \ref kOk
         if the record was successfully updated
  - \ref kErrorUnknownIndex
         if the given index has been closed already or never existed
  - \ref kErrorIncompatibleRecord
         if the record that should be updated is not compatible with the given index
  - \ref kErrorNotFound
         if the given record could not be found
  - \ref kErrorDeadlock
         if the call could not be completed because of a deadlock
  - \ref kErrorOutOfMemory
         if operation was not completed because of a lack of free memory
  - \ref kErrorGenericFailure
         if the record could not be updated for some other reason
*/
ErrorCode UpdateRecord(Transaction *tx, Index *idx, Record *old_record, const char *new_payload);

/**
Searches for a record and removes it from the index.

If the record was not found, an appropriate ErrorCode (\ref kErrorNotFound)
will be returned.

It might be called from outside of a running transaction. In this case all
changes are committed immediately.

@param[in,out] tx
  the transaction to be used (or NULL if called from outside of a running transaction)

@param[in] idx
  the index to modify

@param[in] record
  the record to be deleted
  
@return ErrorCode
  - \ref kOK
         if the record was successfully deleted
  - \ref kErrorUnknownIndex
         if the given index has been closed already or never existed
  - \ref kErrorIncompatibleRecord
         if the record that should be deleted is not compatible with the given index
  - \ref kErrorNotFound
         if the given record could not be found
  - \ref kErrorDeadlock
         if the call could not be completed because of a deadlock
  - \ref kErrorGenericFailure
         if the record could not be deleted for some other reason
*/
ErrorCode DeleteRecord(Transaction *tx, Index *idx, Record *record);

/**
Returns an \ref Iterator that starts at the first value of the given minimum
multidimensional key.

One or more parts of the multidimensional keys might not be defined (set to NULL),
meaning that they have to be treated as wildcards (in such cases ordering is required).

Records with the same key may be returned in any order, but in a manner that n records 
with the same key are retrieved by n successive GetNext() calls.

If no matching records have been found, \ref kErrorNotFound will be returned. In this case
the iterator will immediately return \ref kErrorNotFound, when called for the first time.

@param[in,out] tx
  the transaction to be used (or NULL if called from outside of a running transaction)

@param[in] idx
  the \ref Index to retrieve records from

@param[in] min_keys
  the minimum multidimensional Key 

@param[in] max_keys
  the maximum multidimensional Key

@param[out] it
  returns an \ref Iterator allowing to iterate over the resulting record set

@see GetNext()

@return ErrorCode
  - \ref kOk
         if the records were successfully retrieved
  - \ref kErrorUnknownIndex
         if the given index has been closed or never existed
  - \ref kErrorNotFound
         if no records in the specied key range were found
  - \ref kErrorDeadlock
         if the call could not be completed because of a deadlock
  - \ref kErrorGenericFailure
         if the records could not be retrieved for some other reason
*/
ErrorCode GetRecords(Transaction *tx, Index *idx, Key min_keys, Key max_keys, Iterator **it);

/**
Moves the iterator to the next record or reports an end of range (by returning
an \ref kErrorNotFound status), if the maximum multidimensional key is exceeded.

Records are ordered in ascending order by there key. Records with the same key
may be returned in any order (see GetRecords() for details).

@param[in,out] it
  the Iterator used to retrieve the next record

@param[out] record
  returns the next record of the iterator. If the current record was the last
  record of the given iterator, record will be set to NULL and \ref kErrorNotFound is returned. 

@see GetRecords()

@return ErrorCode
  - \ref kOk
         if the next record was successfully retrieved
  - \ref kErrorIteratorClosed
         if the given iterator has been closed already or never existed
  - \ref kErrorNotFound
         if a follow-up record could not be found
  - \ref kErrorDeadlock
         if the call could not be completed because of a deadlock
  - \ref kErrorGenericFailure
         if the operation did not complete did not complete for some other reason
*/
ErrorCode GetNext(Iterator *it, Record** record);

/**
Closes the given iterator and frees all of its resources.
An iterator may only be closed once, otherwise \ref kErrorIteratorClosed will be returned.

@param[in,out] it
  the iterator to close

@return ErrorCode
  - \ref kOk
         if the given iterator was successfully closed
  - \ref kErrorIteratorClosed
         if the given iterator has been closed already or never existed
  - \ref kErrorGenericFailure
         if the Iterator could not be closed for some other reason
*/
ErrorCode CloseIterator(Iterator **it);

#ifdef __cplusplus
}
#endif

#endif /* _CONTESTINTERFACE_H_ */

