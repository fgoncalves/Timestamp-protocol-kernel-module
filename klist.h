#ifndef __KLIST_H__
#define __KLIST_H__

typedef struct sklist_node{
	void* data;
	struct sklist_node* front;
	struct sklist_node* back;
}klist_node;

typedef struct{
	klist_node* first;
	klist_node* last;
}klist;

extern klist_node* make_klist_node(void* data);
extern klist* make_klist(void);
extern void add_klist_node_to_klist(klist *kl, klist_node* kn);
extern void remove_klist_node_from_klist(klist* kl, klist_node* kn);
extern void free_klist(klist* kl, void (*free_klist_node_data_cb)(void* data));

typedef struct{
	klist* list;
	struct sklist_node* cur;
}klist_iterator;

extern klist_iterator* make_klist_iterator(klist* kl);
extern uint8_t klist_iterator_has_next(klist_iterator* ki);
extern klist_node* klist_iterator_next(klist_iterator* ki);
extern void free_klist_iterator(klist_iterator* ki);
extern klist_node* klist_iterator_peek(klist_iterator* ki);
extern void* klist_iterator_remove_current(klist_iterator* ki);
#endif
