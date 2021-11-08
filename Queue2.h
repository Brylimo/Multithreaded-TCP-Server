#ifndef Queue2_h
#define Queue2_h

#define TRUE 1
#define FALSE 0

typedef struct _tdata
{
  	int thread_num;
  	int value;
} Tdata; 

typedef Tdata Data2;

typedef struct _node2
{
	Data2* data;
	struct _node2* next;
}Node2;

typedef struct _queue2
{
	Node2* front;
	Node2* rear;
}Queue2;

void queue2Init(Queue2* que);
int IsQueue2Empty(Queue2* que);
int IsQueue2Full(Queue2* que);
void enqueue2(Queue2* que, Data2* data);
Data2* dequeue2(Queue2* que);
Data2* queue2Peek(Queue2* que);

#endif /* Queue2_h */
