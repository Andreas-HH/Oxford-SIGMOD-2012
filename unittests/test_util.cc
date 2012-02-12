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
Implements classes to build and run a set of unit tests.

@see unittests/test_util.h
*/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include <contest_interface.h>
#include "test_util.h"



void CopyKeyX(Key &a, const Key &b){
  a.value = new Attribute*[b.attribute_count];
  a.attribute_count = b.attribute_count;
  for(int i = 0; i < a.attribute_count; i++){
    if(b.value[i] != NULL){
      a.value[i] = new Attribute();
      memcpy(a.value[i],b.value[i],sizeof(Attribute));
    } else {
      a.value[i] = NULL;
    }
  }
}

void CopyRecordX(Record *dst, Record *src)
{
	dst->payload.size = src->payload.size;
	dst->payload.data = malloc(src->payload.size);
	memcpy(dst->payload.data, src->payload.data, src->payload.size);
	CopyKeyX(dst->key, src->key);
}

// Deletes a key
void DeleteKeyX(Key* key){
	//if (key.value == 0)
		//return;
  for (int i = 0; i < key->attribute_count; i++){
    if(key->value[i] != NULL){
		  delete key->value[i];
    }
	}

	delete[] key->value;
};

// Deletes a record
void DeleteRecordX(Record **record){
  if(*record == NULL)
    return;

  DeleteKeyX(&((*record)->key));

  free((*record)->payload.data);
  delete (*record);

  (*record) = NULL;
}


/**
Constructor for Tester.

Creates a new Tester instance and performs all necessary initializations.
*/
Tester::Tester(){
};

/**
Destructor for Tester.

Performs all necessary clean up work.
*/
Tester::~Tester(){
};


/**
Return the singleton instance of Tester

@return the singleton instance of Tester
*/
Tester& Tester::getInstance(){
  static Tester instance;
  return instance;
};

/**
Assert the given condition or return a message.

@param condition
  the condition to assert

@param file
  the file in which assertion has been called (normally this should be __FILE__)

@param line
  the line where the assertion has been called (normally this should be __LINE__)

@param message
  a message that will be printed if the given condition was not asserted
*/
void Tester::Assert(const bool condition, const char* file, const int line, const char* message){
  if(!condition){
    num_errors_++;
    std::stringstream result;
    result << "  " << file << ":" << line << ">";
    int length = result.str().length();
    if(length > 45)
      length = 45;
    std::string padding(50-length, ' ');

    result << padding << message << std::endl;

    std::cerr << result.str() << std::flush;
  }
};

/**
Register a test function under the given name.

\note
  If a test with the given name already exists, it will be overwritten

@param name
  the (unique) name of the given test function

@param func
  the test function to be registered

@return
  always true (needed to invoke test registration from a global context)
*/
bool Tester::RegisterTest(const char* name, void (*func)()){
  this->tests_[name] = func;
  return true;
};

/**
Run all registered tests.
*/
void Tester::RunTests(){
  std::map<const char*, void (*)()>::iterator it;
  for(it = tests_.begin(); it != tests_.end(); it++){
    std::cerr << "Executing '"<< it->first << "'..."<<std::endl<<std::flush;
    num_errors_ = 0;
    (*(it->second))();
    if(num_errors_ > 0)
      std::cerr << "  Test failed with " << num_errors_
                << ((num_errors_==1)?" error":" errors") << std::endl;
  }
};
