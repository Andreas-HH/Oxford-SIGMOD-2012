#include "Util.h"
#include "Index.h"

#include <assert.h>
#include <string.h>
#include <cstdlib>

IndexStructure* test = NULL;
/**
Compares two attributes
 */
int attcmp(const Attribute &a, const Attribute &b){
  switch(a.type){
    case kShort:
      if(a.short_value < b.short_value )
        return -1;
      else if (a.short_value > b.short_value )
        return 1;
      break;
    case kInt:
      if(a.int_value < b.int_value )
        return -1;
      else if (a.int_value > b.int_value )
        return 1;
      break;
    case kVarchar:
      return strcmp(a.char_value, b.char_value);
    }
  return 0;
}

//
// Compares two Berkeley DB keys (used for the b-tree)
//
int keycmp(Db *db, const Dbt *a,  const Dbt *b){
  const char* fileName;
  const char* name;
  IndexStructure *is;// = test;
  // Get database name
  //if(test == NULL){
    assert(db->get_dbname((const char **)&fileName,(const char**)&name) == 0);
  
  
  // Try to get the structure of the index
  /*IndexStructure *is*//*test*/is = IndexManager::getInstance().Find(name);
  //is = test;
  //}

  assert(is != NULL);
  
  // Compare the keys
  Key ak = is->GetKey(a);
  Key bk = is->GetKey(b);

  int rt = keycmp(ak, bk);
  int i;

  for (i = 0; i < ak.attribute_count; i++){
	  delete ak.value[i];
  }
  delete[] ak.value;

  for (i = 0; i < bk.attribute_count; i++){
	  delete bk.value[i];
	}
	delete[] bk.value;

  return rt;
}

int keycmp(const Key &a, const Key &b){
  assert(a.attribute_count == b.attribute_count);

  // Compare the keys
  for(int i = 0; i < a.attribute_count; i++){
    assert(a.value[i] != NULL);
    assert(b.value[i] != NULL);
    int res = attcmp(*(a.value[i]),*(b.value[i]));
    if (res != 0)
      return res;
  }
  return 0;
}

// Checks whether every attribute of key a is smaller than or equal to key b
bool CheckBounds(const Key &a, const Key &b, int *index){
  assert(a.attribute_count == b.attribute_count);

  for(int i = 0; i < a.attribute_count; i++ ){
    if((a.value[i] != NULL) && (b.value[i] != NULL) && (attcmp(*(a.value[i]), *(b.value[i])) > 0)){
      if(index)
        *index = i;
      return false;
    }
    // Check whether 
    //if((a.value[i] != NULL) && (b.value[i] != NULL) && (attcmp(*(a.value[i]), *(b.value[i])) > 0)){
    //  return false;
    //}
  }
  return true;
};

void release (const Dbt *dbt){
    if ((dbt != NULL) && (dbt->get_data() != NULL))
    {
        //delete dbt->get_data();
        //dbt->set_data(NULL);
        //delete dbt;
    }
}

void CopyKey(Key &a, const Key &b){
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

void CopyRecord(Record *dst, Record *src)
{
	dst->payload.size = src->payload.size;
	dst->payload.data = malloc(src->payload.size);
	memcpy(dst->payload.data, src->payload.data, src->payload.size);
	CopyKey(dst->key, src->key);
}

// Deletes a key
void DeleteKey(Key* key){
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
void DeleteRecord(Record **record){
  if(*record == NULL)
    return;

  DeleteKey(&((*record)->key));

  free((*record)->payload.data);
  delete (*record);

  (*record) = NULL;
}

