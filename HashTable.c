/*
 * Copyright ©2025 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Winter Quarter 2025 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "CSE333.h"
#include "HashTable.h"
#include "LinkedList.h"
#include "HashTable_priv.h"

///////////////////////////////////////////////////////////////////////////////
// Internal helper functions.
//
#define INVALID_IDX -1

// Grows the hashtable (ie, increase the number of buckets) if its load
// factor has become too high.
static void MaybeResize(HashTable *ht);

int HashKeyToBucketNum(HashTable *ht, HTKey_t key) {
  return key % ht->num_buckets;
}

// Deallocation functions that do nothing.  Useful if we want to deallocate
// the structure (eg, the linked list) without deallocating its elements or
// if we know that the structure is empty.
static void LLNoOpFree(LLPayload_t freeme) { }
static void HTNoOpFree(HTValue_t freeme) { }


///////////////////////////////////////////////////////////////////////////////
// HashTable implementation.

HTKey_t FNVHash64(unsigned char *buffer, int len) {
  // This code is adapted from code by Landon Curt Noll
  // and Bonelli Nicola:
  //     http://code.google.com/p/nicola-bonelli-repo/
  static const uint64_t FNV1_64_INIT = 0xcbf29ce484222325ULL;
  static const uint64_t FNV_64_PRIME = 0x100000001b3ULL;
  unsigned char *bp = (unsigned char *) buffer;
  unsigned char *be = bp + len;
  uint64_t hval = FNV1_64_INIT;

  // FNV-1a hash each octet of the buffer.
  while (bp < be) {
    // XOR the bottom with the current octet.
    hval ^= (uint64_t) * bp++;
    // Multiply by the 64 bit FNV magic prime mod 2^64.
    hval *= FNV_64_PRIME;
  }
  return hval;
}

HashTable* HashTable_Allocate(int num_buckets) {
  HashTable *ht;
  int i;

  Verify333(num_buckets > 0);

  // Allocate the hash table record.
  ht = (HashTable *) malloc(sizeof(HashTable));
  Verify333(ht != NULL);

  // Initialize the record.
  ht->num_buckets = num_buckets;
  ht->num_elements = 0;
  ht->buckets = (LinkedList **) malloc(num_buckets * sizeof(LinkedList *));
  Verify333(ht->buckets != NULL);
  for (i = 0; i < num_buckets; i++) {
    ht->buckets[i] = LinkedList_Allocate();
  }

  return ht;
}

void HashTable_Free(HashTable *table,
                    ValueFreeFnPtr value_free_function) {
  int i;

  Verify333(table != NULL);

  // Free each bucket's chain.
  for (i = 0; i < table->num_buckets; i++) {
    LinkedList *bucket = table->buckets[i];
    HTKeyValue_t *kv;

    // Pop elements off the chain list one at a time.  We can't do a single
    // call to LinkedList_Free since we need to use the passed-in
    // value_free_function -- which takes a HTValue_t, not an LLPayload_t -- to
    // free the caller's memory.
    while (LinkedList_NumElements(bucket) > 0) {
      Verify333(LinkedList_Pop(bucket, (LLPayload_t *)&kv));
      value_free_function(kv->value);
      free(kv);
    }
    // The chain is empty, so we can pass in the
    // null free function to LinkedList_Free.
    LinkedList_Free(bucket, LLNoOpFree);
  }

  // Free the bucket array within the table, then free the table record itself.
  free(table->buckets);
  free(table);
}

int HashTable_NumElements(HashTable *table) {
  Verify333(table != NULL);
  return table->num_elements;
}

// Search a given linked list for the given value.
// If replace == true and value is in
// ll then replace with new value and return old value in val.
// Return true when successful
// When mode = 0 just return the key's value, when mode = 1 delete the key, and
// and when mode = 2 replace the key's value with newPayload->value
bool Search_LinkedList(LinkedList *ll, HTKeyValue_t newPayload,
                       HTValue_t *oldVal, int mode) {
  // Initialize iterator for given list
  LLIterator *iter = LLIterator_Allocate(ll);
  // If iterator starts invalid immediately then free it and return false because ll is empty
  if (!LLIterator_IsValid(iter)) {
    LLIterator_Free(iter);
    return false;
  }
  // Iterator through linked list looking for given key
  do {
    // Initialize current node's payload
    HTKeyValue_t *oldPayload;
    // Get current node from iterator
    LLIterator_Get(iter, (LLPayload_t*) &oldPayload);
    // If keys are different then continue to next node
    if (oldPayload->key != newPayload.key) {
      continue;
    }
    // Else if keys are the same then store old value
    *oldVal = oldPayload->value;
    // If in delete mode then remove node with LLIterator_Remove()
    if (mode == 1) {
      LLIterator_Remove(iter, free);
    } else if (mode == 2) {
      // If in replace mode then put new value in payload
      oldPayload->value = newPayload.value;
    }
    // Since we found the key we can free the iterator and return true
    LLIterator_Free(iter);
    return true;
  // If iterator reaches end of list then exit
  } while (LLIterator_Next(iter));
  // Free iterator and return false since we didn't find the key
  LLIterator_Free(iter);
  return false;
}

bool HashTable_Insert(HashTable *table,
                      HTKeyValue_t newkeyvalue,
                      HTKeyValue_t *oldkeyvalue) {
  int bucket;
  LinkedList *chain;

  Verify333(table != NULL);
  MaybeResize(table);

  // Calculate which bucket and chain we're inserting into.
  bucket = HashKeyToBucketNum(table, newkeyvalue.key);
  chain = table->buckets[bucket];

  // STEP 1: finish the implementation of InsertHashTable.
  // This is a fairly complex task, so you might decide you want
  // to define/implement a helper function that helps you find
  // and optionally remove a key within a chain, rather than putting
  // all that logic inside here.  You might also find that your helper
  // can be reused in steps 2 and 3.

  // Search the keys chain for the key and replace if found
  if (Search_LinkedList(chain, newkeyvalue, &oldkeyvalue->value, 2)) {
    // If key was found then we already updated it
    // and can return true after copying new key
    oldkeyvalue->key = newkeyvalue.key;
    return true;
  }
  // If key isn't already in chain we have to add it
  // Malloc new block for newkeyvalue and store it there
  HTKeyValue_t *payload = malloc(sizeof(HTKeyValue_t));
  *payload = newkeyvalue;
  // Push payload to the list
  LinkedList_Push(chain, payload);
  // Increment num_elements
  table->num_elements++;
  // Return false since we had to add key
  return false;
}

bool HashTable_Find(HashTable *table,
                    HTKey_t key,
                    HTKeyValue_t *keyvalue) {
  Verify333(table != NULL);

  // STEP 2: implement HashTable_Find.
  int bucket;
  LinkedList *chain;

  // Calculate which bucket and chain the key would be in.
  bucket = HashKeyToBucketNum(table, key);
  chain = table->buckets[bucket];

  // Initialize HTKeyValue_t struct for Search_LinkedList()
  HTKeyValue_t target;
  target.key = key;
  target.value = NULL;
  // Search chain for the key but don't replace
  if (!Search_LinkedList(chain, target, &keyvalue->value, 0)) {
    // Key wasn't found so return false
    return false;
  }
  // Else if key was found copy key to keyvalue and return true
  keyvalue->key = key;
  return true;
}

bool HashTable_Remove(HashTable *table,
                      HTKey_t key,
                      HTKeyValue_t *keyvalue) {
  Verify333(table != NULL);

  // STEP 3: implement HashTable_Remove.
  int bucket;
  LinkedList *chain;

  // Calculate which bucket and chain the key would be in.
  bucket = HashKeyToBucketNum(table, key);
  chain = table->buckets[bucket];

  // Initialize HTKeyValue_t struct for Search_LinkedList()
  HTKeyValue_t target;
  target.key = key;
  target.value = NULL;
  // Search chain for key and delete
  if (!Search_LinkedList(chain, target, &keyvalue->value, 1)) {
    // Key wasn't found so return false
    return false;
  }
  // Else key was found so copy key to keyvalue,
  // decrement num_elements, and return true
  keyvalue->key = key;
  table->num_elements--;
  return true;
}


///////////////////////////////////////////////////////////////////////////////
// HTIterator implementation.

HTIterator* HTIterator_Allocate(HashTable *table) {
  HTIterator *iter;
  int         i;

  Verify333(table != NULL);

  iter = (HTIterator *) malloc(sizeof(HTIterator));
  Verify333(iter != NULL);

  // If the hash table is empty, the iterator is immediately invalid,
  // since it can't point to anything.
  if (table->num_elements == 0) {
    iter->ht = table;
    iter->bucket_it = NULL;
    iter->bucket_idx = INVALID_IDX;
    return iter;
  }

  // Initialize the iterator.  There is at least one element in the
  // table, so find the first element and point the iterator at it.
  iter->ht = table;
  for (i = 0; i < table->num_buckets; i++) {
    if (LinkedList_NumElements(table->buckets[i]) > 0) {
      iter->bucket_idx = i;
      break;
    }
  }
  Verify333(i < table->num_buckets);  // make sure we found it.
  iter->bucket_it = LLIterator_Allocate(table->buckets[iter->bucket_idx]);
  return iter;
}

void HTIterator_Free(HTIterator *iter) {
  Verify333(iter != NULL);
  if (iter->bucket_it != NULL) {
    LLIterator_Free(iter->bucket_it);
    iter->bucket_it = NULL;
  }
  free(iter);
}

bool HTIterator_IsValid(HTIterator *iter) {
  Verify333(iter != NULL);

  // STEP 4: Accidentally deleted so didn't grab correct comment
  // If iter->bucket_it is null return false
  if (iter->bucket_it == NULL) {
    return false;
  }

  // If the current LLIterator isn't valid then iter isn't since
  // HTIterator_Next() would have updated the LLIterator if it could
  return LLIterator_IsValid(iter->bucket_it);
}

bool HTIterator_Next(HTIterator *iter) {
  Verify333(iter != NULL);

  // STEP 5: implement HTIterator_Next.
  // Return false if hash table is empty
  if (iter->ht->num_elements == 0) {
    return false;
  }
  // General case of multiple elements
  // Iterate bucket_it, if that worked return true
  if (LLIterator_Next(iter->bucket_it)) {
    return true;
  }
  // If bucket_it was invalidated we need to find the next nonempty bucket and
  // create an iterator of that
  // Search the hash tables buckets array
  // for the first one after bucket_idx with elements
  for (int i = iter->bucket_idx + 1; i < iter->ht->num_buckets; i++) {
    // If current bucket isn't empty move HTIterator to there
    if (LinkedList_NumElements(iter->ht->buckets[i]) > 0) {
      // Change bucket_idx to i
      iter->bucket_idx = i;
      // Free the invalidated LLIterator
      LLIterator_Free(iter->bucket_it);
      // Create an LLIterator of bucket i
      iter->bucket_it = LLIterator_Allocate(iter->ht->buckets[i]);
      // Iterator is successfully iterated so return true
      return true;
    }
  }
  // If we made it through the loop then all remaining buckets are empty,
  // and we need to invalidate iter
  // bucket_it is already invalidated from the LLIterator_Next() call,
  // so we only need to invalidate bucket_idx
  iter->bucket_idx = INVALID_IDX;
  // Return false since we failed to iterate
  return false;

  return true;  // you may need to change this return value
}

bool HTIterator_Get(HTIterator *iter, HTKeyValue_t *keyvalue) {
  Verify333(iter != NULL);

  // STEP 6: implement HTIterator_Get.
  // If iter is invalid return false
  if (!HTIterator_IsValid(iter)) {
    return false;
  }
  // Else get payload from LLIterator_Get() and store in keyvalue
  HTKeyValue_t *outputKeyValue;
  LLIterator_Get(iter->bucket_it, (LLPayload_t*) &outputKeyValue);
  *keyvalue = *outputKeyValue;

  return true;  // you may need to change this return value
}

bool HTIterator_Remove(HTIterator *iter, HTKeyValue_t *keyvalue) {
  HTKeyValue_t kv;

  Verify333(iter != NULL);

  // Try to get what the iterator is pointing to.
  if (!HTIterator_Get(iter, &kv)) {
    return false;
  }

  // Advance the iterator.  Thanks to the above call to
  // HTIterator_Get, we know that this iterator is valid (though it
  // may not be valid after this call to HTIterator_Next).
  HTIterator_Next(iter);

  // Lastly, remove the element.  Again, we know this call will succeed
  // due to the successful HTIterator_Get above.
  Verify333(HashTable_Remove(iter->ht, kv.key, keyvalue));
  Verify333(kv.key == keyvalue->key);
  Verify333(kv.value == keyvalue->value);

  return true;
}

static void MaybeResize(HashTable *ht) {
  HashTable *newht;
  HashTable tmp;
  HTIterator *it;

  // Resize if the load factor is > 3.
  if (ht->num_elements < 3 * ht->num_buckets)
    return;

  // This is the resize case.  Allocate a new hashtable,
  // iterate over the old hashtable, do the surgery on
  // the old hashtable record and free up the new hashtable
  // record.
  newht = HashTable_Allocate(ht->num_buckets * 9);

  // Loop through the old ht copying its elements over into the new one.
  for (it = HTIterator_Allocate(ht);
       HTIterator_IsValid(it);
       HTIterator_Next(it)) {
    HTKeyValue_t item, unused;

    Verify333(HTIterator_Get(it, &item));
    HashTable_Insert(newht, item, &unused);
  }

  // Swap the new table onto the old, then free the old table (tricky!).  We
  // use the "no-op free" because we don't actually want to free the elements;
  // they're owned by the new table.
  tmp = *ht;
  *ht = *newht;
  *newht = tmp;

  // Done!  Clean up our iterator and temporary table.
  HTIterator_Free(it);
  HashTable_Free(newht, &HTNoOpFree);
}
