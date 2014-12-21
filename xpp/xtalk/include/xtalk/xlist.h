#ifndef	XLIST_H
#define	XLIST_H

#include <xtalk/api_defs.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

struct xlist_node {
	void			*data;
	struct xlist_node	*next;
	struct xlist_node	*prev;
};

typedef void (*xlist_destructor_t)(void *data);

XTALK_API struct xlist_node *xlist_new(void *data);
XTALK_API void xlist_destroy(struct xlist_node *list, xlist_destructor_t destructor);
XTALK_API void xlist_append_list(struct xlist_node *list1, struct xlist_node *list2);
XTALK_API void xlist_append_item(struct xlist_node *list, struct xlist_node *item);
XTALK_API void xlist_remove_item(struct xlist_node *item);
XTALK_API struct xlist_node *xlist_shift(struct xlist_node *list);
XTALK_API int xlist_empty(const struct xlist_node *list);
XTALK_API size_t xlist_length(const struct xlist_node *list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* XLIST_H */
