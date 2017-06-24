#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include <assert.h>

void list_init(struct node **phead)
{
	assert(phead != NULL);
	*phead = (struct node *)malloc(sizeof(struct node));

	(*phead)->next = NULL;
}

void list_add(struct node *phead,int c)
{
	assert(phead != NULL);
	
	struct node *tmp = ( struct  node *) malloc(sizeof(struct node ));
	tmp->c = c;
	tmp->next = NULL;

	struct node *p = phead;
	while(p->next != NULL)
	{
		p = p->next;
	}
	p->next = tmp;
}

int list_get(struct node *phead)//尾插从头拿
{

	assert(phead != NULL);
	
	int fd = -1;
	struct node *p = phead->next;
	if(p == NULL)
		return ;
	fd = p->c;
	phead->next = p->next;
	free(p);

	return fd;
}
/*
void list_destory(struct node *phead)
{
	assert(phead != NULL);
	
	int fd = -1;
	struct node *p = phead->next;
	if(p == NULL)
		return ;
	fd = p->c;
	phead->next = p->next;
	free(p);
	
	return fd;
}
*/
