#ifndef _INDEX_H_
#define _INDEX_H_

#include <map>
#include <set>
#include <string>

#include <contest_interface.h>
#include <common/macros.h>

#include "ConnectionManager.h"
#include "Mutex.h"

class Db;
class Dbt;
class Dbc;
class IndexStructure;
class DbTxn; // TODO: Replace with Transaction

// Class representing an index handle
class Index{
 public:    
  // Destructor
  ~Index();
  
  // Opens an index
  static ErrorCode Open(const char* name, Index** index);
  
  // Close this index
  void Close();
  
  // Create a cursor to access the data inside this index
  Dbc* Cursor(Transaction* tx);
  
  // Return the name of this index
  const char* name() const { return name_; };
  
  // Converts the given Dbt to a key of this index
  Key GetKey(const Dbt *bdb_key);
  void FreeKey(Key key);
  
  // Converts the given Key of this index into a Dbt object
  Dbt *GetBDBKey(Key key, bool max = false);
  
  // Insert the given record into the index
  ErrorCode Insert(Transaction *tx, Record *record);
  
  // Update the given record with the given payload
  ErrorCode Update(Transaction *tx, Record *record, Block *payload, uint8_t flags);
  
  // Delete the given record
  ErrorCode Delete(Transaction *tx, Record *record, uint8_t flags);
  
  // Checks whether the given record is compatible with this index
  bool Compatible(Record *record);

  // Checks whether the given key is compatible with this index
  bool Compatible(Key key);
  
  // Register a new iterator handle
  bool register_iterator(Iterator* iterator);
  
  // Unregister an iterator
  void unregister_iterator(Iterator* iterator);

  // Return whether the index has been closed
  bool closed () const { return closed_; };

 private:
  // Constructor
  Index(const char* name);
  
  // The Berkeley DB database handle
  Db	*db_;

  // The name of this index
  const char* name_;

  // The structure of this index
  IndexStructure* structure_;

  // Whether the index has been closed
  bool closed_;

  // The number of operations, that are currently in progress
  uint8_t op_count_;

  // A set of all open iterators that use this index handle
  std::set<Iterator*> iterators_;
  
  // A mutex for protecting the insert and read operations on the iterator set
  Mutex mutex_;
  
  DISALLOW_COPY_AND_ASSIGN(Index);
};

// Class representing the structure of an index
class IndexStructure{
  public:
  // Constructor
  IndexStructure(uint8_t attribute_count, KeyType type);
  
  // Destructor
  ~IndexStructure();
  
  // Converts the given Dbt to a key of this index
  Key GetKey(const Dbt *bdb_key);
  
  // Converts the given Key of this index into a Dbt object
  Dbt *GetBDBKey(Key key, bool max = false);

  // Register a new index handle
  void register_handle(Index* handle);
  
  // Unregister an index handle
  void unregister_handle(Index* handle);
  
  // Close all registered handles
  void CloseHandles();
  
  // Start a new modifying transaction on this index (returns false if index is readonly)
  bool start_transaction(DbTxn *tx);

  // End a modifying transaction on this index
  void end_transaction(DbTxn *tx);

  // Try to make this index read-only (will return false if open transactions have written to this index)
  bool MakeReadOnly();

  uint8_t attribute_count() const { return attribute_count_; };
  AttributeType* type(){ return type_; };
  size_t size(){return size_;};
 
 private:
  // The number of attributes that form a key of this index
  uint8_t attribute_count_;
  
  // An array of attribute types
   AttributeType* type_;
  
  // The size of a key of this index in byte
  size_t size_;

  // Whether the index is readonly
  bool read_only_;

  // A set of all open handles of this index structure
  std::set<Index*> handles_;

  // A set of open transactions that have modified this index
  std::set<DbTxn*> transactions_;
  
  // A mutex for protecting the insert and read operations on the handle set
  Mutex mutex_;

  // A mutex for protecting the insert and read operations on the transaction set
  Mutex transaction_mutex_;

  DISALLOW_COPY_AND_ASSIGN(IndexStructure);
};



/**
 * Defines a simple index manager.
 * 
 * It is used to manage the structures of the created indices.
 * 
 * IndexManager implements the Singleton Pattern. 
 */
class IndexManager{
 	public:
    // Return the singleton instance of IndexManager
		static IndexManager& getInstance();
		
    // Search for a index with the given name
    IndexStructure *Find(std::string name);    
    
    // Insert a index structure (may overwrite existing entrie)
    void Insert(std::string name, IndexStructure* structure);
    
    // Search and delete the index structure with the given name
    ErrorCode Remove(std::string name);

    // Removes the given transaction from all indices
    void CloseTransaction(Transaction* tx);

    //IndexStructure* structure("");

	private:
		// Private constructor (don't allow instanciation from outside)
		IndexManager(){};

		// Destructor
		~IndexManager(){};
		
		// A map holding the structures of all indices
		std::map<std::string,IndexStructure*> indices_;
    

    //std::map<Db*, IndexStructure*> structure_;
    // A mutex for protecting the insert and read operations
    // Note: It would be better to use some type of S/X locks
    // as multiple threads may read the map concurrently without
    // affecting each other (this map however is not performance-critical)
    Mutex mutex_;
    
    Mutex db_mutex_;

    DISALLOW_COPY_AND_ASSIGN(IndexManager);
};



#endif
