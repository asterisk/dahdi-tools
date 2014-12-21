#include <assert.h>
#include <stdio.h>
#include <xtalk/xlist.h>
#include <autoconfig.h>

void dump_list(const struct xlist_node *list)
{
	struct xlist_node *curr;
	const char *p;
	int len;

	len = xlist_length(list);
	p = list->data;
	printf("dumping list: %s[%d]\n", p, len);
	for (curr = list->next; curr != list; curr = curr->next) {
		p = curr->data;
		printf("> %s\n", p);
	}
}

void string_destructor(void *data)
{
	const char	*p = data;

	printf("destroy: '%s'\n", p);
}

int main()
{
	struct xlist_node	*list1;
	struct xlist_node	*list2;
	struct xlist_node	*list3;
	struct xlist_node	*item1;
	struct xlist_node	*item2;
	struct xlist_node	*item3;

	list1 = xlist_new("list1");
	list2 = xlist_new("list2");
	list3 = xlist_new("list3");
	item1 = xlist_new("item1");
	item2 = xlist_new("item2");
	item3 = xlist_new("item3");
	assert(xlist_empty(list1));
	assert(xlist_empty(list2));
	assert(xlist_empty(list3));
	assert(xlist_empty(item1));
	assert(xlist_empty(item2));
	assert(xlist_empty(item3));
	dump_list(list1);
	dump_list(list2);
	xlist_append_item(list1, item1);
	assert(!xlist_empty(list1));
	xlist_append_item(list1, item2);
	xlist_append_item(list1, item3);
	dump_list(list1);
	xlist_remove_item(item2);
	assert(!xlist_empty(list1));
	xlist_append_item(list2, item2);
	assert(!xlist_empty(list2));
	dump_list(list1);
	dump_list(list2);
	xlist_shift(list1);
	dump_list(list1);
	xlist_append_list(list1, list2);
	dump_list(list1);
	xlist_append_item(list3, item1);
	xlist_append_list(list1, list3);
	dump_list(list1);
	xlist_destroy(list1, string_destructor);
	xlist_destroy(list2, string_destructor);
	xlist_destroy(list3, string_destructor);
	return 0;
}
