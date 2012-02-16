#define __STDC_LIMIT_MACROS
#include "Iterator.h"
#include "Util.h"

#include <db_cxx.h>
#include <sstream>

#include <stdint.h>
#include <cstdlib>
#include <string.h>
#include <assert.h>

/**
 * Initialize the iterator to iterate over a given index.
 */
Iterator::Iterator(Transaction* tx, Index* idx, Key min_keys, Key max_keys){
  
  index_ = idx;
  closed_ = false;
  end_ = false;
  initialized_ = false;
  cvalue = NULL;

    
  // Initialize the min_key_ (copy the whole key)
  CopyKey(min_key_, min_keys);
  
  // Initialize the max_key_ (copy the whole key)
  CopyKey(max_key_, max_keys);

  // Initialize the cursor
  cursor_ = index_->Cursor(tx);
  
  // Start with the min_key and an empty value
  key_ = index_->GetBDBKey(min_keys);
  value_ = new Dbt();
  value_->set_size(0);


  // Register the new iterator
  //index_->register_iterator(this);
}

//
// Retrieves the next value from the iterator
//
// It essentially applies post-filtering to the result set, in order to find 
// the next valid key/value pair
// (i.e., it reads the next key/value pairs until a valid one has been found 
//  or the given range is exceeded).
//
// This approach can be very inefficient (especially for partial match queries
// that do not restrict the dimension of the first key)
//
bool Iterator::Next(){
  int err, index;
  bool found = false;


  if(!initialized_){
    // Get the first key/value pair in the range of this iterator
	char* pkey = (char*) key_->get_data();
    err = cursor_->get(key_, value_, DB_SET_RANGE);
    if (pkey != key_->get_data())
    {
    	delete [] pkey;
    }
    initialized_ = true;
  } else {
    // Move the cursor to the next key
    err = cursor_->get(key_, value_, DB_NEXT);
  }
  
  while(true){
    
    if( err == 0){
    	Key key = index_->GetKey(key_);
      
      // Check if the key is inside the range of this iterator
      if(!CheckBounds(key,max_key_,&index)){
    	  index_->FreeKey(key);

        // As the records are ordered starting with the first key attribute
        // we have exceeded our key range when the first key attribute of the retrieved
        // key is greater than the first attribute of the maximum key
        if(index == 0){

          // Mark the iterator as ended
          SetEnded();
          
          // And clean up
          //delete [] (char*)key_->get_data();
	        //delete key_;
          //delete [] (char*)value_->get_data();
          //delete value_;

          return true;
        }
        // Move the cursor to the next key
        err = cursor_->get(key_, value_, DB_NEXT);
      } else {
        // We've found a record
    	  index_->FreeKey(key);
        return true;
      }
    } else {
      // Mark the iterator as ended because no new record could be fetched
      SetEnded();

      // If no record was found, than no error occured
      if(err != DB_NOTFOUND){
        // Otherwise, some error occured while fetching the record

        // Close the iterator
        Close();

        return false;
      }
      return true;
    }
  }
  return true;
}

// Return the record to which the iterator refers
Record* Iterator::value(){
  
  if (cvalue != NULL)
	  DeleteRecord(&cvalue);
  cvalue = 0;
  
  
  // If the iterator has already ended, don't return a record
  if(!end_){
    cvalue = new Record;
    // Set the key
    cvalue->key = index_->GetKey(key_);
  
    // Set the payload
    cvalue->payload.data = malloc(value_->get_size());
    memcpy(cvalue->payload.data,value_->get_data(),value_->get_size());
    cvalue->payload.size = value_->get_size();
  }
  return cvalue;
};

// Close the iterator
void Iterator::Close(){

	if (cvalue != NULL)
		 DeleteRecord(&cvalue);

	closed_ = true;
    CloseCursor();
    DeleteKey(&min_key_);
    DeleteKey(&max_key_);

    delete key_;
    delete value_;

}

// Closes the Berkeley DB Cursor
void Iterator::CloseCursor(){
  // Needed to prevent closing of cursor that are already closed
  if(cursor_!=NULL){
    cursor_->close();
    cursor_ = NULL;
  }
}

// Mark the iterator as ended
void Iterator::SetEnded(){
    end_=true;
    
    // Close the current cursor, so that read locks are released
    CloseCursor();
};
