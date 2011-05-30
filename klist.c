#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "klist.h"

#define kalloc(type, how_many)				\
	(type*) kmalloc(sizeof(type) * how_many, GFP_ATOMIC)

#define kalloc_error(str)									\
	printk("%s in %s:%d: kmalloc failed. %s\n", __FILE__, __FUNCTION__, __LINE__, (str))

static void free_klist_node(klist_node* kn) {
	kfree(kn);
}

klist_node* make_klist_node(void* data) {
	klist_node* kn = kalloc(klist_node, 1);
	if (!kn) {
		kalloc_error("Unnable to create klist node.\n");
		return NULL;
	}
	memset(kn, 0, sizeof(klist_node));
	kn->data = data;
	return kn;
}
EXPORT_SYMBOL(make_klist_node);

klist* make_klist(void) {
	klist* kl = kalloc(klist, 1);
	if (!kl) {
		kalloc_error("Unnable to create klist.\n");
		return NULL;
	}
	memset(kl, 0, sizeof(klist));
	return kl;
}
EXPORT_SYMBOL(make_klist);

void add_klist_node_to_klist(klist *kl, klist_node* kn) {
	if (kl->first == kl->last) {
		kl->first = kl->last = kn;
		return;
	}

	kn->back = kl->last;
	kl->last->front = kn;
	kl->last = kn;
}
EXPORT_SYMBOL(add_klist_node_to_klist);

void remove_klist_node_from_klist(klist* kl, klist_node* kn) {
	if (!kn || !kl)
		return;

	if (kl->first == kn && kn == kl->last) {
		kl->first = kl->last = NULL;
		free_klist_node(kn);
		return;
	}

	if (kl->first == kn) {
		kl->first = kn->front;
		free_klist_node(kn);
		return;
	}

	if (kl->last == kn) {
		kl->last = kn->back;
		free_klist_node(kn);
		return;
	}

	kn->back->front = kn->front;
	kn->front->back = kn->back;
	free_klist_node(kn);
}
EXPORT_SYMBOL(remove_klist_node_from_klist);

void free_klist(klist* kl, void(*free_klist_node_data_cb)(void* data)) {
	klist_node* it;
	if (!kl)
		return;

	it = kl->first;
	while (it != NULL) {
		kl->first = it->front;
		if (free_klist_node_data_cb)
			free_klist_node_data_cb(it->data);
		kfree(it);
		it = kl->first;
	}

	kfree(kl);
}
EXPORT_SYMBOL(free_klist);

klist_iterator* make_klist_iterator(klist* kl) {
	klist_iterator* kit;

	if (!kl)
		return NULL;

	kit = kalloc(klist_iterator, 1);
	if (!kit) {
		kalloc_error("Unnable to create klist iterator.\n");
		return NULL;
	}
	kit->cur = kl->first;
	kit->list = kl;
	return kit;
}
EXPORT_SYMBOL(make_klist_iterator);

uint8_t klist_iterator_has_next(klist_iterator* ki) {
	return (ki && ki->cur);
}
EXPORT_SYMBOL(klist_iterator_has_next);

klist_node* klist_iterator_next(klist_iterator* ki) {
	klist_node* result;
	if (!ki || !ki->cur)
		return NULL;

	result = ki->cur;
	ki->cur = ki->cur->front;
	return result;
}
EXPORT_SYMBOL(klist_iterator_next);

void free_klist_iterator(klist_iterator* ki) {
	kfree(ki);
}
EXPORT_SYMBOL(free_klist_iterator);

klist_node* klist_iterator_peek(klist_iterator* ki){
	return ki->cur;
}
EXPORT_SYMBOL(klist_iterator_peek);

void* klist_iterator_remove_current(klist_iterator* ki){
	void* dt;
	klist_node* kn;
	if(ki->cur){
		dt = ki->cur->data;
		kn = ki->cur;
		ki->cur = ki->cur->front;
		remove_klist_node_from_klist(ki->list, kn);
		return dt;
	}
	return NULL;
}
EXPORT_SYMBOL(klist_iterator_remove_current);
