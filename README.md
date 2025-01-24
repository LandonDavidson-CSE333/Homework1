# Homework1

The files I actually wrote are HashTable.c and LinkedList.c, of which I wrote the functions with the comment `// STEP X ...`. Everything else was provided by the class as either testing programs or were easy enough to not bother making students implemented (Like getSize() functions). Specific interface details are in the respective .h header files

- LinkedList.c:
  - LinkedList_Allocate(): Return a properly initialized LinkedList struct with head and tail as null
  
  - LinkedList_Free(): Free each node in the passed LinkedList and its payload one at a time to avoid memory leaks
  
  - LinkedList_Push(): Add a node to the head of the passed LinkedList
  
  - LinkedList_Pop(): Remove a node from the head of the passed LinkedList and return its payload through the output parameter
  
  - LinkedList_Append(): Add a node to the tail of the passed LinkedList
  
  - LLIterator_Next(): Move the passed LLIterator's current node to the next node in the list if possible. If we are at the end of the list iterate into the null value and return false to mark the iterator invalid
  
  - LLIterator_Remove(): Remove the current node of the passed LLIterator and return false if the list is now empty. When removing a node we have to splice it out by changing its next and previous node's next and previous pointers.
  
  - LLSlice(): Remove a node from the tail of the passed LinkedList and return its payload through the output parameter. Private helper function used by LLIterator_Remove()

- HashTable.c:
  
  - Search_LinkedList(): Search a given LinkedList for the given key and when found either replace its value, remove the node, or just return the value depending on what mode
  
  - HashTable_Insert(): Find the given keys bucket in the given HashTable, Search_LinkedList() on the bucket in insert mode, and return the old value through the output parameter
  
  - HashTable_Find(): Find the given keys bucket in the given HashTable, Search_LinkedList() on the bucket in find mode, and return the value through the output parameter
  
  - HashTable_Remove(): Find the given keys bucket in the given HashTable, Search_LinkedList() on the bucket in delete mode, and return the old value through the output parameter
  
  - HTIterator_IsValid(): If HTIterator's LLIterator is null that means the HashTable is empty so return false, then check if HTIterator's LLIterator is valid since HTIterator_Next() will always create a ne valid LLIterator if possible
  
  - HTIterator_Next(): If the HashTable is empty immediately return false. Otherwise if the HTIterator's LLIterator is successfully iterated we haven't reached the end of the bucket and can just return true. Finally, if LLIterator_Next() failed then we need to search the remaining buckets in the HashTable to find the next nonempty LinkedList and create an iterator of that list. If there aren't anymore nonempty LinkedLists then we are at the end of the HashTable and should invalidate the HTIterator and return false
  
  - HTIterator_Get(): If HTIterator isn't valid immediately return false since there is no current node. Otherwise return the current node's value through the output parameter