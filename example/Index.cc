#define __STDC_LIMIT_MACROS
#include "Index.h"
#include "Iterator.h"
#include "Util.h"
#include <db_cxx.h>
#include <assert.h>

#include <stdint.h>
#include <cstdlib>
#include <string.h>

//TODO: Optimize GetBDBKey

Dbt* Index::GetBDBKey(Key key, bool max){
  return structure_->GetBDBKey(key);
};

Key Index::GetKey(const Dbt *bdb_key){
  return structure_->GetKey(bdb_key);
}

void Index::FreeKey(Key key)
{
	int i;
	for (i = 0; i < key.attribute_count; i++)
	{
		delete key.value[i];
	}
	delete[] key.value;
}


Dbt* IndexStructure::GetBDBKey(Key key, bool max){

  // Allocate the necessary memory
  char* data = new char[size_];

  int offset = 0, size = 0;
  
  // If the key value is NULL we have to set a wildcard
  // depending on if it is a maximum or minimum key
  int32_t short_wildcard = (max?INT32_MAX:INT32_MIN);
  int64_t int_wildcard = (max?INT64_MAX:INT64_MIN);

  for(int i = 0; i < attribute_count_; i++){
    // Determine the attribute size
    if(type_[i] == kShort){
      size = 4;
      if(key.value[i] == NULL)
        memcpy(data+offset,&short_wildcard,size);
    }else if(type_[i] == kInt){
      size = 8;
      if(key.value[i] == NULL)
        memcpy(data+offset,&int_wildcard,size);

    }else{
      size = MAX_VARCHAR_LENGTH+1;
      if(key.value[i] == NULL){
         memset(data+offset,127,MAX_VARCHAR_LENGTH);
         memset(data+offset+MAX_VARCHAR_LENGTH,'\0',1);
      }
    }
    if(key.value[i] != NULL){
      // Copy the data into the buffer
      memcpy(data+offset,&(key.value[i]->char_value),size);
    }
    // Set the offset to the first byte after the attribute
    offset += size;
  }

  // Return the newly created Dbt object
  Dbt* rt = new Dbt(data,size_);
  return rt;
};

Key IndexStructure::GetKey(const Dbt *bdb_key){
  // Create the new key object
  Key key;
  key.value = new Attribute*[attribute_count_];
  key.attribute_count = attribute_count_;
  
  int offset = 0, size = 0;
  for(int i = 0; i < attribute_count_; i++){
    // Determine the attribute size
    if(type_[i] == kShort)
      size = 4;
    else if(type_[i] == kInt)
      size = 8;
    else
      size = MAX_VARCHAR_LENGTH+1;
    
    // Create the attribute
    key.value[i] = new Attribute;
    key.value[i]->type = type_[i];
    memcpy(&(key.value[i]->char_value),((char*)bdb_key->get_data())+offset,size);

    // Set the offset to the first byte after the attribute
    offset+=size;
  }

  return key;
}

Index::Index(const char* name){
  name_ = name;
  closed_ = true;
  op_count_ = 0;
};

ErrorCode Index::Open(const char* name, Index** index){
  
  *index = new Index(name);

  // Creat a new db handle
  (*index)->db_ = new Db(ConnectionManager::getInstance().env(), 0);
  
  // Try to get the structure of the requested index
  (*index)->structure_ = IndexManager::getInstance().Find(name);
  if(!(*index)->structure_)
    return kErrorUnknownIndex;
  
  (*index)->structure_->register_handle(*index);


  // Allow duplicates for this db instance
	(*index)->db_->set_flags(DB_DUP);
	  
  // Set the compare function for the b-tree
  (*index)->db_->set_bt_compare(&keycmp);

  // And finally open the index
  (*index)->db_->open(NULL, 	    // Transaction pointer
    NULL, 					              // File name (NULL, because we want to load from main memory)
    (*index)->name_,				 	    // Logical db name (i.e. the index name)
    DB_BTREE,				              // Database type (we use b-tree)
    DB_THREAD | DB_AUTO_COMMIT,   // Open flags
    0                             // File mode (defaults)
    );

  // The index was successfully opened
  (*index)->closed_ = false;

  return kOk;
};

Dbc* Index::Cursor(Transaction* tx){
  Dbc* cursor;

  // Create a new cursor with isolation level read committed
  db_->cursor((DbTxn*) tx, &cursor, DB_READ_COMMITTED);

  return cursor;
};

Index::~Index(){
  Close();
};

void Index::Close(){
  lock(mutex_){
    if(!closed_)
      closed_ = true;
    else
      return;
  }
  // If there are currently operations that work on this index wait until they are done
  while(op_count_ > 0){
    sleep(1);
  };
      //std::cerr<<__LINE__<<"\n";

      if(db_ != NULL){
               // std::cerr<<__LINE__<<"\n";

        db_->close(0);
        //std::cerr<<__LINE__<<"\n";

        db_ = NULL;
      }
          //std::cerr<<__LINE__<<"\n";

      if(structure_ != NULL){
        structure_->unregister_handle(this);
      }
          //std::cerr<<__LINE__<<"\n";

      closed_ = true;
        //std::cerr<<__LINE__<<"\n";

      // Close all iterators that use this index handle
      /*std::set<Iterator*>::iterator it;
      while((it=iterators_.begin()) != iterators_.end()){
        if(*it == NULL)
          std::cout<<"NULL"<<std::flush;
        else
          (*it)->Close();
      }*/
    
  
}

ErrorCode Index::Insert(Transaction *tx, Record *record){
  // Convert the payload
  Dbt value;
  value.set_data(record->payload.data);
  value.set_size(record->payload.size);
  value.set_flags(0);
	
  
  // If the insert occured inside a larger transaction, then add
  // the parent transaction to the set of open transactions
  if(tx != NULL){
    if(!structure_->start_transaction((DbTxn*) tx))
      return kErrorUnknownIndex;
  }

  // Perform the database put
  Dbt bdbkey = *GetBDBKey(record->key);
  bdbkey.set_flags(0);//TODO: Check if needed
  ErrorCode res = kOk;
  if (db_->put((DbTxn*) tx, &bdbkey, &value, 0) != 0){
	  res = kErrorGenericFailure;
  }else{
    release(&bdbkey);
    release(&value);
  }
  return res;
  
}

ErrorCode Index::Update(Transaction *tx, Record *record, Block *payload, uint8_t flags){
  ErrorCode result = kOk;
  DbTxn* tid;
  Dbc* cursor;
  
  bool ignore_payload = (flags & kIgnorePayload);
  lock(mutex_){
  // Convert the record 
  Dbt key = *GetBDBKey(record->key);
  void* pkey = key.get_data();
  
  Dbt value;
  // If necessary set the value to match
  if(!ignore_payload){
    value.set_data(record->payload.data);
    value.set_size(record->payload.size);
  }

  Dbt original_value = value;

  // Convert the new payload
  Dbt new_value;
  new_value.set_data(payload->data);
  new_value.set_size(payload->size);
  
  // Create a serializable nested transaction (to prevent the transaction
  // from seeing data that has been inserted after the transaction begun)
  ConnectionManager::getInstance().env()->txn_begin((DbTxn*) tx, &tid,  0 );
  
  // Start writing on the index
  if(structure_->start_transaction(tid)){

    // Create a cursor for this index
    db_->cursor(tid, &cursor, 0);

    // Retrieve the first record
    int err = 0, i;
    if(ignore_payload){
      err = cursor->get(&key, &value, DB_SET_RANGE);

      // Check that the found record has a valid key
      Key otkey = GetKey(&key);
      if(keycmp(otkey,record->key))
        err = DB_NOTFOUND;
      FreeKey(otkey);
    } else {
      // Try an exact match
      err = cursor->get(&key, &value, DB_GET_BOTH);
    }
    if(err == 0){
      // Update the record using the new payload
      if((err = cursor->put(&key, &new_value, DB_CURRENT)) == 0){
        // If the update occured inside a larger transaction, then add
        // the parent transaction to the set of open transactions
        if(tx != NULL){
          structure_->start_transaction((DbTxn*) tx);
        }
        
        // If necessary update duplicates
        if(flags & kMatchDuplicates){
          if(!ignore_payload){
            // Find next exact duplicate
            while((err = cursor->get(&key, &value, DB_NEXT_DUP)) == 0){
              // And update it
              if ((err = cursor->put(&key, &new_value, DB_CURRENT)) != 0)
                break;
            }
          } else {
            // Find next record with the same key
        	Key cmpkey = GetKey(&key);
            while(((err = cursor->get(&key, &value, DB_NEXT)) == 0)
                    && (keycmp(cmpkey,record->key) == 0)){
              // And update it
              if ((err = cursor->put(&key, &new_value, DB_CURRENT)) != 0)
                break;
            }
            FreeKey(cmpkey);

          }
          
          if(err != DB_NOTFOUND)
            result = kErrorNotFound;
        }
      } else {
        if(err == DB_NOTFOUND)
          result = kErrorNotFound;
        else
          result = kErrorGenericFailure;
      }
    } else {
      if(err == DB_NOTFOUND)
        result = kErrorNotFound;
      else
        result = kErrorGenericFailure;
    }
  } else {
    // Abort the new transaction
	delete [] (char*) pkey;
    tid->abort();
    return kErrorUnknownIndex;
  }

  // Commit the nested transaction
  tid->commit(0);
  delete [] (char*) pkey;
  // We finished writing on the index
  structure_->end_transaction(tid);
  }
  return result;
}

ErrorCode Index::Delete(Transaction *tx, Record *record, uint8_t flags){
  ErrorCode result = kOk;
  DbTxn* tid;
  Dbc* cursor;
  
  bool ignore_payload = (flags & kIgnorePayload);

  // Convert the record 
  Dbt key = *GetBDBKey(record->key);
  void* pkey = key.get_data();
  
  Dbt value;
  // If necessary set the value to match
  if(!ignore_payload){
    value.set_data(record->payload.data);
    value.set_size(record->payload.size);
  }
    
  // Create a serializable nested transaction (to prevent the transaction
  // from seeing data that has been inserted after the transaction begun)
  ConnectionManager::getInstance().env()->txn_begin((DbTxn*) tx, &tid,  0 );
  
  // Start writing on the index
  if(structure_->start_transaction(tid)){
  
    // Create a cursor for this index
    db_->cursor(tid, &cursor, DB_READ_COMMITTED);

    // Retrieve the first record
    int err = 0, i;
    if(ignore_payload){
      err = cursor->get(&key, &value, DB_SET_RANGE);
    
      // Check that the found record has a valid key
      Key cmpkey = GetKey(&key);
      if(keycmp( cmpkey,record->key))
        err = DB_NOTFOUND;
      FreeKey(cmpkey);

    } else {
      // Try an exact match
      err = cursor->get(&key, &value, DB_GET_BOTH);
    }

    if(err == 0){
      // Delete the record
      if((err = cursor->del(0)) == 0){
        // If the deletion occured inside a larger transaction, then add
        // the parent transaction to the set of open transactions
        if(tx != NULL){
          structure_->start_transaction((DbTxn*) tx);
        }

        // If necessary delete duplicates
        if(flags & kMatchDuplicates){
          if(!ignore_payload){
            // Find next exact duplicate
            while((err = cursor->get(&key, &value, DB_NEXT_DUP)) == 0){
              // And delete it
              if ((err = cursor->del(0)) != 0)
                break;
            }
          } else {
            // Find next record with the same key
        	 Key cmpkey = GetKey(&key);
            while(((err = cursor->get(&key, &value, DB_NEXT)) == 0)
                    && (keycmp(cmpkey,record->key) == 0)){
              // And delete it
              if ((err = cursor->del(0)) != 0)
                break;
            }
            FreeKey(cmpkey);

          }
          
          if(err != DB_NOTFOUND)
            result = kErrorGenericFailure;
        }
      } else {
      if(err == DB_NOTFOUND)
        result = kErrorNotFound;
      else
        result = kErrorGenericFailure;
    }


    } else {
      if(err == DB_NOTFOUND)
        result = kErrorNotFound;
      else
        result = kErrorGenericFailure;
    }
  } else {
    // Abort the new transaction
	delete [] (char*) pkey;
    tid->abort();

    return kErrorUnknownIndex;
  }

  // Commit the nested transaction //TODO: Errorhandling
  delete [] (char*) pkey;
  tid->commit(0);

  // We finished writing on the index
  structure_->end_transaction(tid);
  
  return result;
}
bool Index::Compatible(Record *record){
  if((record == NULL) || closed_)
    return false;

  return (record->key.attribute_count == structure_->attribute_count());
}

bool Index::Compatible(Key key){
  if(closed_)
    return false;

  return (key.attribute_count == structure_->attribute_count());
}

// Register a new iterator handle
bool Index::register_iterator(Iterator* iterator){
  lock(mutex_){
    if(closed_)
      return false;
    if(iterator != NULL)
      iterators_.insert(iterator);
  }
  return true;
};
  
// Unregister an iterator
void Index::unregister_iterator(Iterator* iterator){
  lock(mutex_){
    if(closed_)
      return;
    iterators_.erase(iterator);
  }
};


/**
Return the singleton instance of IndexManager;

@return the singleton instance of IndexManager
*/
IndexManager& IndexManager::getInstance(){
  static IndexManager instance;
  return instance;
};

void IndexManager::Insert(std::string name, IndexStructure* structure){
  lock(mutex_){
    indices_[name] = structure;
  }
};

IndexStructure* IndexManager::Find(std::string name){
  lock(mutex_){
    std::map<std::string,IndexStructure*>::iterator it = indices_.find(name);

    if( it != indices_.end())
    {
      return it->second;
    }
    
    return NULL;
  }
};

ErrorCode IndexManager::Remove(std::string name){
  lock(mutex_){
    std::map<std::string,IndexStructure*>::iterator it = indices_.find(name);

    if( it != indices_.end()){
      // Try to delete the index structure
      if(it->second->MakeReadOnly()){
        delete it->second;
        indices_.erase(it);
      } else {
        //std::cout<<"kErrorOpenTransaction"<<std::flush;
        return kErrorOpenTransactions;
      }
    } else {
      //std::cout<<"kErrorUnknownIndex"<<std::flush;
      return kErrorUnknownIndex;
    }
  }
  return kOk;
}

IndexStructure::IndexStructure(uint8_t attribute_count, KeyType type){
    attribute_count_ = attribute_count;
    type_ = new AttributeType[attribute_count];
    size_ = 0;
    read_only_=false;

    // Build the size and copy the type array
    for(int i = 0; i < attribute_count; i++){
      if(type[i] == kShort)
        size_ += 4;
      else if(type[i] == kInt)
        size_ += 8;
      else
        size_ += MAX_VARCHAR_LENGTH+1;
      
      type_[i] = type[i];
    }
  };

IndexStructure::~IndexStructure(){
    // Close all open Handles of this structure
    CloseHandles();
    delete[] type_;
  };

void IndexStructure::register_handle(Index* handle){
    lock(mutex_){
      handles_.insert(handle);
    }
  };

void IndexStructure::unregister_handle(Index* handle){
    lock(mutex_){
      handles_.erase(handle);
    }
  };

void IndexStructure::CloseHandles(){
      std::set<Index*>::iterator it;
      it=handles_.begin();
      while(it != handles_.end()){
        // Closs Index handle
        if(*it != NULL)
          (*it)->Close();
        it++;

      }
  };

  
// Start a new modifying transaction on this index (returns false if index is read-only)
bool IndexStructure::start_transaction(DbTxn *tx){
  lock(transaction_mutex_){
    if(read_only_)
      return false;
    
    transactions_.insert(tx);
  }
  return true;
}

// End a modifying transaction on this index
void IndexStructure::end_transaction(DbTxn *tx){
  lock(transaction_mutex_){
    transactions_.erase(tx);
  }
}

// Try to make this index read-only (will return false if open transactions have written to this index)
bool IndexStructure::MakeReadOnly(){
  lock(transaction_mutex_){
    if(transactions_.size() > 0){
      return false;
    }
    read_only_ = true;
  }
  return true;
}

void IndexManager::CloseTransaction(Transaction* tx){
  lock(mutex_){
    std::map<std::string,IndexStructure*>::iterator it;
    for(it=indices_.begin();it!=indices_.end();it++){
      it->second->end_transaction((DbTxn*) tx);
    }
  }
}

