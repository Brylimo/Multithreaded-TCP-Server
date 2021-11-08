#include <stdio.h>
#include <stdlib.h>
#include "Queue2.h"

void queue2Init(Queue2* que)
{
	que->front = NULL;
	que->rear = NULL;
}

int IsQueue2Empty(Queue2* que)
{
	if (que->front == NULL)
		return TRUE;
	else
		return FALSE;
}

void enqueue2(Queue2* que, Data2* data)
{
	Node2* newNode = (Node2*)malloc(sizeof(Node2));
	newNode->data = data;
	newNode->next = NULL;

	if (IsQueue2Empty(que))
	{
		que->front = newNode;
		que->rear = newNode;
	} else { 
		que->rear->next = newNode;
		que->rear = newNode;
	} 
}

Data2* dequeue2(Queue2* que)
{
	if (IsQueue2Empty(que))
	{
		exit(-1);
	}

	Node2* temp = que->front;
	Data2* tempData = temp->data;
	que->front = que->front->next;

	free(temp);
	return tempData;
}

Data2* queue2Peek(Queue2* que)
{
	if (IsQueue2Empty(que))
	{
		exit(-1);
	}
	return que->front->data;	
}
