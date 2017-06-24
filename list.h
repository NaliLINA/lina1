#include <stdio.h>
struct node
{
	int c;
	struct node *next;
};

void list_init(struct node **phead);
void list_add(struct node *phead,int c);
int list_get(struct node *phead);
void list_destory(struct node *phead);
