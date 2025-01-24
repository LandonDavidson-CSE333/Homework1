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

#include "CSE333.h"
#include "LinkedList.h"
#include "LinkedList_priv.h"


///////////////////////////////////////////////////////////////////////////////
// LinkedList implementation.

LinkedList* LinkedList_Allocate(void) {
  // Allocate the linked list record.
  LinkedList *ll = (LinkedList *) malloc(sizeof(LinkedList));
  Verify333(ll != NULL);

  // STEP 1: initialize the newly allocated record structure.
  // List is empty so initialize num_elements as 0 and head/tail as null
  ll->num_elements = 0;
  ll->head = NULL;
  ll->tail = NULL;

  // Return our newly minted linked list.
  return ll;
}

void LinkedList_Free(LinkedList *list,
                     LLPayloadFreeFnPtr payload_free_function) {
  Verify333(list != NULL);
  Verify333(payload_free_function != NULL);

  // STEP 2: sweep through the list and free all of the nodes' payloads
  // (using the payload_free_function supplied as an argument) and
  // the nodes themselves.
  // Create cur_node variable as list head
  LinkedListNode *cur_node = list->head;
  // Run until cur_node is null (when we leave the list)
  while (cur_node != NULL) {
    // Free the payload of the cur_node
    payload_free_function(cur_node->payload);
    // Save the next node for after we free cur_node
    LinkedListNode *next_node = cur_node->next;
    // Free cur_node and set cur_node to the next node we saved previously
    free(cur_node);
    // Set current node to next
    cur_node = next_node;
  }

  // free the LinkedList
  free(list);
}

int LinkedList_NumElements(LinkedList *list) {
  Verify333(list != NULL);
  return list->num_elements;
}

void LinkedList_Push(LinkedList *list, LLPayload_t payload) {
  Verify333(list != NULL);

  // Allocate space for the new node.
  LinkedListNode *ln = (LinkedListNode *) malloc(sizeof(LinkedListNode));
  Verify333(ln != NULL);

  // Set the payload
  ln->payload = payload;

  // Null out ln->prev
  ln->prev = NULL;

  if (list->num_elements == 0) {
    // Degenerate case; list is currently empty
    Verify333(list->head == NULL);
    Verify333(list->tail == NULL);
    ln->next = ln->prev = NULL;
    list->head = list->tail = ln;
    list->num_elements = 1;
  } else {
    // STEP 3: typical case; list has >=1 elements
    // Set head's prev to the new node
    list->head->prev = ln;
    // Set the new nodes next to head
    ln->next = list->head;
    // Change list->head to new node
    list->head = ln;
    // Increment num_elements
    list->num_elements++;
  }
}

bool LinkedList_Pop(LinkedList *list, LLPayload_t *payload_ptr) {
  Verify333(payload_ptr != NULL);
  Verify333(list != NULL);

  // STEP 4: implement LinkedList_Pop.  Make sure you test for
  // and empty list and fail.  If the list is non-empty, there
  // are two cases to consider: (a) a list with a single element in it
  // and (b) the general case of a list with >=2 elements in it.
  // Be sure to call free() to deallocate the memory that was
  // previously allocated by LinkedList_Push().
  // If list is empty immediately return false
  if (list->num_elements == 0) {
    return false;
  }
  // Save the head's payload to payload_ptr
  *payload_ptr = list->head->payload;
  // If list has one element handle the edge case
  if (list->num_elements == 1) {
    // Free the head node
    free(list->head);
    // Set head and tail to null since list is now empty
    list->head = NULL;
    list->tail = NULL;
  } else {
    // Else do the standard case
    // Set head to its next value
    list->head = list->head->next;
    // Free unused node through new head's prev
    free(list->head->prev);
    // Set new head's prev to null
    list->head->prev = NULL;
  }
  // Decrement num_elements by 1
  list->num_elements--;

  // Since we already handled the only fail case (num_elements == 0),
  // everything else returns true for success
  return true;  // you may need to change this return value
}

void LinkedList_Append(LinkedList *list, LLPayload_t payload) {
  Verify333(list != NULL);

  // STEP 5: implement LinkedList_Append.  It's kind of like
  // LinkedList_Push, but obviously you need to add to the end
  // instead of the beginning.
  // Allocate space for the new node.
  LinkedListNode *ln = (LinkedListNode *) malloc(sizeof(LinkedListNode));
  Verify333(ln != NULL);

  // Set the payload
  ln->payload = payload;

  if (list->num_elements == 0) {
    // Degenerate case; list is currently empty
    Verify333(list->head == NULL);
    Verify333(list->tail == NULL);
    ln->next = ln->prev = NULL;
    list->head = list->tail = ln;
    list->num_elements = 1;
  } else {
    // Set tail's next to the new node
    list->tail->next = ln;
    // Set the new nodes prev to tail
    ln->prev = list->tail;
    // Change list->tail to new node
    list->tail = ln;
    // Increment num_elements
    list->num_elements++;
  }
}

void LinkedList_Sort(LinkedList *list, bool ascending,
                     LLPayloadComparatorFnPtr comparator_function) {
  Verify333(list != NULL);
  if (list->num_elements < 2) {
    // No sorting needed.
    return;
  }

  // We'll implement bubblesort! Nice and easy, and nice and slow :)
  int swapped;
  do {
    LinkedListNode *curnode;

    swapped = 0;
    curnode = list->head;
    while (curnode->next != NULL) {
      int compare_result = comparator_function(curnode->payload,
                                               curnode->next->payload);
      if (ascending) {
        compare_result *= -1;
      }
      if (compare_result < 0) {
        // Bubble-swap the payloads.
        LLPayload_t tmp;
        tmp = curnode->payload;
        curnode->payload = curnode->next->payload;
        curnode->next->payload = tmp;
        swapped = 1;
      }
      curnode = curnode->next;
    }
  } while (swapped);
}


///////////////////////////////////////////////////////////////////////////////
// LLIterator implementation.

LLIterator* LLIterator_Allocate(LinkedList *list) {
  Verify333(list != NULL);

  // OK, let's manufacture an iterator.
  LLIterator *li = (LLIterator *) malloc(sizeof(LLIterator));
  Verify333(li != NULL);

  // Set up the iterator.
  li->list = list;
  li->node = list->head;

  return li;
}

void LLIterator_Free(LLIterator *iter) {
  Verify333(iter != NULL);
  free(iter);
}

bool LLIterator_IsValid(LLIterator *iter) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);

  return (iter->node != NULL);
}

bool LLIterator_Next(LLIterator *iter) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  // STEP 6: try to advance iterator to the next node and return true if
  // you succeed, false otherwise
  // Note that if the iterator is already at the last node,
  // you should move the iterator past the end of the list
  // Set iter->node to node's next value
  iter->node = iter->node->next;
  // If iter->node is now null then we moved past the end
  // and iter is invalidated, so return false
  if (iter->node == NULL) {
    return false;
  }

  // We covered the only false case so everything else should exit
  // with true since the function succeeded
  return true;  // you may need to change this return value
}

void LLIterator_Get(LLIterator *iter, LLPayload_t *payload) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  *payload = iter->node->payload;
}

bool LLIterator_Remove(LLIterator *iter,
                       LLPayloadFreeFnPtr payload_free_function) {
  Verify333(iter != NULL);
  Verify333(iter->list != NULL);
  Verify333(iter->node != NULL);

  // STEP 7: implement LLIterator_Remove.  This is the most
  // complex function you'll build.  There are several cases
  // to consider:
  // - degenerate case: the list becomes empty after deleting.
  // - degenerate case: iter points at head
  // - degenerate case: iter points at tail
  // - fully general case: iter points in the middle of a list,
  //                       and you have to "splice".
  //
  // Be sure to call the payload_free_function to free the payload
  // the iterator is pointing to, and also free any LinkedList
  // data structure element as appropriate.
  // Free payload since we don't need it
  payload_free_function(iter->node->payload);
  // Handle the edge case of one element in the list
  if (iter->list->num_elements == 1) {
    // Free current node
    free(iter->node);
    // Set iter->node to null to mark it invalid
    iter->node = NULL;
    // Set list head and tail to null and num_elements to 0
    // since it is now empty
    iter->list->head = NULL;
    iter->list->tail = NULL;
    iter->list->num_elements = 0;
    // List is empty and iter is invalid so return false
    return false;
  }
  // Now handle the edge case of iter->node being the tail
  // and move iter back instead of forward
  if (iter->node->next == NULL) {
    // Set iter->node to iter->node->prev
    iter->node = iter->node->prev;
    // Call LLSlice with dummy output parameter
    LLPayload_t payload;
    LLSlice(iter->list, &payload);
  } else if (iter->node->prev == NULL) {
    // Handle edge case of iter->node being the head
    // Set iter->node to iter->node->next
    iter->node = iter->node->next;
    // Call LinkedListPop with dummy output parameter
    LLPayload_t payload;
    LinkedList_Pop(iter->list, &payload);
  } else {
    // Finally do the standard case of removing a center node
    // and setting iter->node to iter->node->next
    // Set node behind iter->node to the node after it
    // (splice node out in forward direction)
    iter->node->prev->next = iter->node->next;
    // Splice node out in the backwards direction
    iter->node->next->prev = iter->node->prev;
    // Save next node before freeing current node
    LinkedListNode *next_node = iter->node->next;
    free(iter->node);
    // Reassign current node
    iter->node = next_node;
    // Decrement iter->list->num_elements
    iter->list->num_elements--;
  }

  // The degenerate one item case covers the only list empty case,
  // so everything else returns true
  return true;  // you may need to change this return value
}


///////////////////////////////////////////////////////////////////////////////
// Helper functions

bool LLSlice(LinkedList *list, LLPayload_t *payload_ptr) {
  Verify333(payload_ptr != NULL);
  Verify333(list != NULL);

  // STEP 8: implement LLSlice.
  // If list is empty return false
  if (list->num_elements == 0) {
    return false;
  }
  // Store payload in payload_ptr
  *payload_ptr = list->tail->payload;
  // If list has one element handle edge case
  if (list->num_elements == 1) {
    // Free tail node
    free(list->tail);
    // Set head and tail to null since list is now empty
    list->head = NULL;
    list->tail = NULL;
  } else {
    // Set tail to its prev value
    list->tail = list->tail->prev;
    // Free unused tail node through new lists next
    free(list->tail->next);
    // Set new tail's next to null
    list->tail->next = NULL;
  }
  // Decrement num_elements
  list->num_elements--;

  // The only fail case is covered by the elements == 0 case
  // so everything else succeeds and returns true
  return true;  // you may need to change this return value
}

void LLIteratorRewind(LLIterator *iter) {
  iter->node = iter->list->head;
}
