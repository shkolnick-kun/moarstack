//
// Created by svalov on 9/13/16.
//

#include <funcResults.h>
#include "priorityQueue.h"
#include <stddef.h>
#include <stdlib.h>
#include <memory.h>
#include <priorityQueue.h>

/* Priority queue implementations:
 *   Unsorted list: O(1) insertion time, O(n) deqeue time (takes O(n) to search)
 *                  we can do this with a linked list
 *   Heap: tree data structure such as parent node always greater than child node.
 *         Unlike BST, there is no left subtree greater than right subtree
 *         Most implementation of heap use an array, where root is at index 0, left
 *         child at index 1, right child inde 2, left child of left child at index 3...
 *         Array can be dynamically resized. Linked list can also be used.
 *         Descending heap or max heap has greatest priority element on top (root). It
 *         supports getMax
 *         Ascending heap or min heap has lowest priority element on top. It supports getMin,
 *         which is called pq_pop here
 */

int pqInit(PriorityQueue_T* queue, int size, pqCompareFunc_T func, size_t keySize, size_t dataSize)
{
	if(NULL == queue)
		return FUNC_RESULT_FAILED_ARGUMENT;
	if(NULL == func)
		return FUNC_RESULT_FAILED_ARGUMENT;
	if(0 == keySize)
		return FUNC_RESULT_FAILED_ARGUMENT;
	if(0 == dataSize)
		return FUNC_RESULT_FAILED_ARGUMENT;

	if (size < 4)  // allocate at least for 4 elements
		size = 4;
	queue->Storage = (pqEntry_T *)malloc(sizeof(pqEntry_T) * size);
	if(NULL == queue->Storage)
		return FUNC_RESULT_FAILED_MEM_ALLOCATION;
	queue->Allocated = size;
	queue->Count = 0;

	queue->Compare = func;
	queue->DataSize = dataSize;
	queue->PrioritySize = keySize;

	return FUNC_RESULT_SUCCESS;
}
int pqClear(PriorityQueue_T* queue){
	if(NULL == queue)
		return FUNC_RESULT_FAILED_ARGUMENT;

	while(queue->Count!=0)
		pqDequeue(queue, NULL);
	return FUNC_RESULT_SUCCESS;
}
int pqDeinit(PriorityQueue_T* queue){
	if(NULL == queue)
		return FUNC_RESULT_FAILED_ARGUMENT;

	int clearRes = pqClear(queue);
	free(queue->Storage);
	queue->Storage = NULL;
	queue->Allocated = 0;
	return clearRes;
}

static inline int parent(int index){
	return (index-1)/2;
}
static inline int child(int index){
	return index*2 + 1;
}

int pqLift(PriorityQueue_T* queue, int index){
	pqEntry_T item = queue->Storage[index];
	while(	index>0 &&
			queue->Compare(item.Priority, queue->Storage[parent(index)].Priority, queue->PrioritySize)>0
			){
		int prnt = parent(index);
		queue->Storage[index] = queue->Storage[prnt];
		index = prnt;
	}
	queue->Storage[index] = item;
	return index;
}

int pqSift(PriorityQueue_T* queue, int index){
	pqEntry_T item = queue->Storage[index];
	while(index<=parent(queue->Count-1)){
		int j = child(index);
		if(		j<queue->Count &&
				(queue->Compare(queue->Storage[j].Priority, queue->Storage[j+1].Priority, queue->PrioritySize)<0)
				)
			++j;
		if(queue->Compare(item.Priority, queue->Storage[j].Priority, queue->PrioritySize)>=0) break;
		queue->Storage[index] = queue->Storage[j];
		index = j;
	}
	queue->Storage[index] = item;
	return index;
}

int pqEnqueue(PriorityQueue_T* queue, void* key, void* data){
	if(NULL == queue)
		return FUNC_RESULT_FAILED_ARGUMENT;
	if(NULL == key)
		return FUNC_RESULT_FAILED_ARGUMENT;
	if(NULL == data)
		return FUNC_RESULT_FAILED_ARGUMENT;

	if (queue->Count >= queue->Allocated)
	{
		queue->Allocated *= 2;
		queue->Storage = (pqEntry_T*)realloc(queue->Storage , sizeof(pqEntry_T) * queue->Allocated);
		// todo check returned type
	}

	int index = queue->Count;
	// allocate and copy data
	queue->Storage[index].Data = malloc(queue->DataSize);
	if(NULL == queue->Storage[index].Data)
		return FUNC_RESULT_FAILED_MEM_ALLOCATION;
	memcpy(queue->Storage[index].Data, data, queue->DataSize);

	// allocate and copy key
	queue->Storage[index].Priority = malloc(queue->PrioritySize);
	if(NULL == queue->Storage[index].Priority)
		return FUNC_RESULT_FAILED_MEM_ALLOCATION;
	memcpy(queue->Storage[index].Priority, key, queue->PrioritySize);

	queue->Count++;
	// lift
	int liftRes = pqLift(queue, index);
	return FUNC_RESULT_SUCCESS;
}

int pqDequeue(PriorityQueue_T* queue, void* data){
	if(NULL == queue)
		return FUNC_RESULT_FAILED_ARGUMENT;
	if(0 == queue->Count)
		return FUNC_RESULT_FAILED_ARGUMENT;

	if(NULL != data)
		memcpy(data, queue->Storage[0].Data, queue->DataSize);
	// free
	free(queue->Storage[0].Priority);
	queue->Storage[0].Priority = NULL;
	free(queue->Storage[0].Data);
	queue->Storage[0].Data = NULL;
	// move
	queue->Storage[0] = queue->Storage[--(queue->Count)];
	// sift down
	int siftRes = pqSift(queue,0);
	return FUNC_RESULT_SUCCESS;
}

int pqTop(PriorityQueue_T* queue, void* data){
	if(NULL == queue)
		return FUNC_RESULT_FAILED_ARGUMENT;
	if(NULL == data)
		return FUNC_RESULT_FAILED_ARGUMENT;

	void* dataPtr = pqTopData(queue);
	if(NULL != dataPtr)
		return FUNC_RESULT_FAILED;
	memcpy(data, dataPtr, queue->DataSize);

	return FUNC_RESULT_SUCCESS;
}
void* pqTopData(PriorityQueue_T* queue){
	if(NULL == queue)
		return NULL;
	if(0 == queue->Count)
		return NULL;
	return queue->Storage[0].Data;
}
void* pqTopPriority(PriorityQueue_T* queue){
	if(NULL == queue)
		return NULL;
	if(0 == queue->Count)
		return NULL;
	return queue->Storage[0].Priority;
}

//void pq_push(pq_ptr q, TYPEDATA data, int priority)
//{
//	pq_element * pq_element_ptr;
//	int s, f; // indices to traverse tree (s son, f father)
//
//	// check if we need to reallocate memory
//	if (q->n >= q->alloc)
//	{
//		q->alloc *= 2;
//		q->pq_array = (pq_element *)realloc(q->pq_array, sizeof(pq_element) * q->alloc);
//	}
//	q->n++; // one more element in PQ
//	s = q->n - 1; // new element is placed at bottom of tree/array
//	f = (s-1)/2; //pq_array[f] is father of pq_array[s]
//	while ((s > 0) && (priority < q->pq_array[f].priority))
//	{
//		q->pq_array[s] = q->pq_array[f]; // shift father down
//		s = f; // son takes place of father
//		f = (s-1)/2; // father at this new position of son
//	}
//	//place element here
//	//printf("Inserting data %d and priority %d in heap at index %d\n", data, priority, s);
//	q->pq_array[s].data = data;
//	q->pq_array[s].priority = priority;
//
//}
//
///* adjust tree that lost its root */
//void pq_adjusttree(pq_ptr q)
//{
//	int p;  // index to parent
//	int s1; // index for son 1
//	int s2; // index for son 2
//	int i;
//
//	p = 0;
//	s1 = 1;
//	s2 = 2;
//
//	// readjusting tree since root (pq_array[0]) will be deleted
//	do
//	{
//		// if only one son, or son 1 lower priority
//		if ((s2 > q->n-1) || (q->pq_array[s1].priority < q->pq_array[s2].priority))
//		{
//			q->pq_array[p]=q->pq_array[s1]; // son 1 takes place of parent
//			p = s1;
//		}
//		else
//		{
//			q->pq_array[p]=q->pq_array[s2]; // son 2 takes place of parent
//			p = s2;
//		}
//		s1 = 2*p + 1;
//		s2 = 2*p + 2;
//	} while (s1 <= q->n-1); // parent doesn't have any children. we are done
//	//printf("priority last parent: %d\n", p);
//
//	// rellocate nodes in array after last parent that was moved up
//	int size = q->n;
//	int current = p+1;
//	while (current != size)
//	{
//		q->n = current - 1;
//		pq_push(q, q->pq_array[current].data, q->pq_array[current].priority);
//		current++;
//	}
//
//	q->n = size-1; // one fewer element in heap
//
//	if (q->n < q->alloc/2 && q->n >= 4)
//	{
//		q->pq_array = realloc(q->pq_array, sizeof(pq_element) * (q->alloc/2));
//		q->alloc = q->alloc/2;
//	}
//
//}
//
//void pq_display(pq_ptr pq)
//{
//	int i;
//	printf("Size of priority queue: %d\n", pq->n);
//	printf("Space allocated for queue: %d\n", pq->alloc);
//	printf("Data in priority queue:\n");
//	for (i = 0; i < pq->n; i++)
//		printf("%d ", pq->pq_array[i].data);
//	printf("\n");
//	printf("Priority of the data:\n");
//	for (i = 0; i < pq->n; i++)
//		printf("%d ", pq->pq_array[i].priority);
//	printf("\n");
//}
//
//
//TYPEDATA pq_pop(pq_ptr q)
//{
//	if (q->n == 0)
//	{
//		printf("Underflow error: trying to pop empty queue\n");
//		return 0;
//	}
//
//	TYPEDATA out;
//	out = q->pq_array[0].data;
//	if (q->n == 1)
//		q->n = 0;
//	else
//		pq_adjusttree(q);
//	return out;
//}
//
//// the following functions are to be used by MST Prim's algorithm
//
//void pq_changepriority(pq_ptr pq, int data, int priority)
//{
//	int i;
//	for (i = 0; i < pq->n; i++)
//	{
//		if (pq->pq_array[i].data == data)
//		{
//			pq->pq_array[i].priority = priority;
//			return;
//		}
//	}
//	printf("Data %d could not be find in PQ\n");
//	exit(-1);
//}
//
//int pq_haskey(pq_ptr pq, int data)
//{
//	int i;
//	for (i = 0; i < pq->n; i++)
//	{
//		if (pq->pq_array[i].data == data)
//			return TRUE;
//	}
//	return FALSE;
//}