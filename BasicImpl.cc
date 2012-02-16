/*
Copyright (c) 2011 TU Dresden - Database Technology Group

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author: Lukas M. Maas <Lukas_Michael.Maas@mailbox.tu-dresden.de>

Current version: 1.0 (released February 11, 2012)

Version history:
  - 1.0 Initial release (Februrary 11, 2012) 
*/

/** @file
Basic implementation that just prints out the requested operation and returns an appropriate error code.
*/

#include <contest_interface.h>

ErrorCode BeginTransaction(Transaction **tx){
  printf("BeginTransaction\n");
  return kOk;
};

ErrorCode AbortTransaction(Transaction **tx){
  printf("AbortTransaction\n");
  return kOk;
};

ErrorCode CommitTransaction(Transaction **tx){
  printf("CommitTransaction\n");
  return kOk;
};

ErrorCode CreateIndex(const char* name, uint8_t column_count, KeyType types){
  printf("CreateIndex\n");
  return kOk;
}

ErrorCode OpenIndex(const char* name, Index **idx){
  printf("OpenIndex\n");
  return kOk;
}

ErrorCode CloseIndex(Index **idx){
  printf("CloseIndex\n");
  return kOk;
}

ErrorCode DeleteIndex(const char* name){
  printf("DeleteIndex\n");
  return kOk;
}

ErrorCode InsertRecord(Transaction *tx,Index *idx, Record *record){
  printf("InserRecord\n");
  return kOk;
}

ErrorCode UpdateRecord(Transaction *tx,Index *idx, Record *old_record,Block *new_payload, uint8_t flags){
  printf("UpdateRecord\n");
  return kOk;
}

ErrorCode DeleteRecord(Transaction *tx,Index *idx, Record *record, uint8_t flags){
  printf("DeleteRecord\n");
  return kOk;
}

ErrorCode GetRecords(Transaction *tx,Index *idx, Key min_keys, Key max_keys, Iterator **it){
  printf("GetRecords\n");
  return kErrorNotFound;
}

ErrorCode GetNext(Iterator *it, Record** record){
  printf("GetNext\n");
  return kErrorNotFound;
}

ErrorCode CloseIterator(Iterator **it){
  printf("CloseIterator\n");
  return kOk;
}
