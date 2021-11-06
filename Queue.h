#ifndef Queue_h
#define Queue_h

#define TRUE 1
#define FALSE 0

typedef int Data;
typedef struct _node
{
	int data;
	struct _node* next;
}Node;
typedef struct _queue
{
	Node* front;
	Node* rear;
}Queue;

void queueInit(Queue* que);
int IsQueueEmpty(Queue* que);
int IsQueueFull(Queue* que);
void enqueue(Queue* que, Data data);
Data dequeue(Queue* que);
Data queuePeek(Queue* que);

#endif /* Queue_h */
