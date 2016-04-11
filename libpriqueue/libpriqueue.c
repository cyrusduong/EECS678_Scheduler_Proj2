/** @file libpriqueue.c
 */

#include <stdlib.h>
#include <stdio.h>

#include "libpriqueue.h"

node* newNode() {
	node* ptr = (node*) malloc (sizeof(node));
	ptr->data = NULL;
	ptr->nextNode = NULL;
	return ptr;
}

/**
  Initializes the priqueue_t data structure.

  Assumtions
    - You may assume this function will only be called once per instance of priqueue_t
    - You may assume this function will be the first function called using an instance of priqueue_t.
  @param q a pointer to an instance of the priqueue_t data structure
  @param comparer a function pointer that compares two elements.
  See also @ref comparer-page
 */
void priqueue_init(priqueue_t *q, int(*comparer)(const void *, const void *))
{
	q->head = NULL;
	q->size = 0;
	q->compare = comparer;
}

/**
  Inserts the specified element into this priority queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr a pointer to the data to be inserted into the priority queue
  @return The zero-based index where ptr is stored in the priority queue, where 0 indicates that ptr was stored at the front of the priority queue.
 */
int priqueue_offer(priqueue_t *q, void *ptr)
{
	node* nNode = newNode();
	nNode->data = ptr;
	//If queue is empty
	if (q->size == 0) {
		q->head = nNode;
		q->size++;
		return 0;
	} else { //Else something is in the queue
		//Element belongs at head
		if (q->compare(nNode->data, q->head->data) < 0) {
			nNode->nextNode = q->head;
			q->head = nNode;
			q->size++;
			return 0;
		}

		//Traverse until we find correct spot
		node* traverse = q->head;
		for (int i = 0; i < q->size; i++) {
			if (traverse->nextNode == NULL || q->compare(nNode->data, traverse->nextNode->data) < 0) {
				nNode->nextNode = traverse->nextNode;
				traverse->nextNode = nNode;
				q->size++;
				return i+1;
			} else {
				traverse = traverse->nextNode;
			}
		}
	}
	return -1; //Returns if error
}

/**
  Retrieves, but does not remove, the head of this queue, returning NULL if
  this queue is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return pointer to element at the head of the queue
  @return NULL if the queue is empty
 */
void *priqueue_peek(priqueue_t *q)
{
	if (q->head != NULL) {
		return (q->head)->data;
	}
	return NULL;
}


/**
  Retrieves and removes the head of this queue, or NULL if this queue
  is empty.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the head of this queue
  @return NULL if this queue is empty
 */
void *priqueue_poll(priqueue_t *q)
{
	if (q->head != NULL) {
		node* removeMe = q->head;
		void* dataToReturn = removeMe->data;
		q->head = (q->head)->nextNode;
		q->size -= 1;
		free(removeMe);
		return dataToReturn;
	}
	return NULL;
}


/**
  Returns the element at the specified position in this list, or NULL if
  the queue does not contain an index'th element.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of retrieved element
  @return the index'th element in the queue
  @return NULL if the queue does not contain the index'th element
 */
void *priqueue_at(priqueue_t *q, int index)
{
	if (index < q->size) {
		node* traverse = q->head;
		for (int i = 0; i < index; i++) {
			traverse = traverse->nextNode;
		}
		void *data = traverse->data;
		return data;
	}
	return NULL;
}


/**
  Removes all instances of ptr from the queue.

  This function should not use the comparer function, but check if the data contained in each element of the queue is equal (==) to ptr.

  @param q a pointer to an instance of the priqueue_t data structure
  @param ptr address of element to be removed
  @return the number of entries removed
 */
int priqueue_remove(priqueue_t *q, void *ptr)
{
	int numRemoved = 0;
	while (q->size > 0 && q->head->data == ptr) {
		node* removeMe = q->head;
		q->head = q->head->nextNode;
		free(removeMe);
		q->size--;
		numRemoved++;
	}

	node* prev = NULL;
	node* traverse = NULL;
	if (q->head != NULL) {
		prev = q->head;
		traverse = q->head->nextNode;
	}

	while (traverse != NULL) {
		if (traverse->data == ptr) {
			prev->nextNode = traverse->nextNode;
			free(traverse);
			q->size--;
			numRemoved++;
			traverse = prev->nextNode;
		} else {
			// printf("numRemoved=%d\n", numRemoved);
			prev = traverse;
			traverse = traverse->nextNode;
		}
	}

	return numRemoved;
}


/**
  Removes the specified index from the queue, moving later elements up
  a spot in the queue to fill the gap.

  @param q a pointer to an instance of the priqueue_t data structure
  @param index position of element to be removed
  @return the element removed from the queue
  @return NULL if the specified index does not exist
 */
void *priqueue_remove_at(priqueue_t *q, int index)
{
	if (index < q->size) {
		node* prev = NULL;
		node* traverse = q->head;
		for (int i = 0; i < index; i++) {
			prev = traverse;
			traverse = traverse->nextNode;
		}
		void *data = traverse->data;
		prev->nextNode = traverse->nextNode;
		free(traverse);
		q->size--;
		return data;
	}
	return NULL;
}


/**
  Returns the number of elements in the queue.

  @param q a pointer to an instance of the priqueue_t data structure
  @return the number of elements in the queue
 */
int priqueue_size(priqueue_t *q)
{
	return q->size;
}


/**
  Destroys and frees all the memory associated with q.

  @param q a pointer to an instance of the priqueue_t data structure
 */
void priqueue_destroy(priqueue_t *q)
{
	while (priqueue_poll(q) != NULL) {}
}
