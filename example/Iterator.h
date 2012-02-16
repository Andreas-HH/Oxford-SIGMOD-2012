#ifndef _ITERATOR_H_
#define _ITERATOR_H_

#include "Index.h"

class Dbc;
class Dbt;

// Represents an iterator
class Iterator {
 public:
  // Constructor
  Iterator(Transaction* tx, Index* idx, Key min_keys, Key max_keys);

  // Close the iterator
  void Close();

  // Move the iterator to the next record
  bool Next();

  // Return whether the iterator has been closed
  bool closed() const { return closed_; };

  // Return whether the iterator has exceeded its range
  bool end() const { return end_; };

  // Return the record to which the iterator refers
  Record* value();
    
 private:
  // Mark the iterator as ended
  void SetEnded();
  
  Record* cvalue;

  // The current key to which the iterator refers
  Dbt *key_;

  // The current value to which the iterator refers
  Dbt *value_;

  // The maximum key that limits the range of this iterator
  Key max_key_;

  // The minimum key for this iterator
  Key min_key_;
  
  // The index which is iterated over
  Index *index_;

  // The used Berkeley DB cursor
  Dbc *cursor_;

  // Whether the iterator has been closed
  bool closed_;

  // Whether the iterator has exceeded its range
  bool end_;

  // Whether the iterator is initialized
  bool initialized_;
  
  // Closes the Berkeley DB Cursor
  void CloseCursor();

  DISALLOW_COPY_AND_ASSIGN(Iterator);
};


#endif
