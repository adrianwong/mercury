/*
**	Definitions for the list module.
*/

#ifndef	LIST_H

#define	LIST_H

typedef	struct	s_list
{
	Cast		l_data;
	struct	s_list	*l_prev;
	struct	s_list	*l_next;
} List;

#define	next(ptr)		(ptr)->l_next
#define	prev(ptr)		(ptr)->l_prev
#define	ldata(ptr)		(ptr)->l_data
#define	first_ptr(list)		((list)->l_next)
#define	last_ptr(list)		((list)->l_prev)
#define	first(list)		((list)->l_next->l_data)
#define	last(list)		((list)->l_prev->l_data)

#define	makelist(d)		list_makelist((Cast) d)
#define	addhead(l, d)		list_addhead(l, (Cast) d)
#define	addtail(l, d)		list_addtail(l, (Cast) d)
#define	insert_before(l, w, d)	list_insert_before(l, w, (Cast) d)
#define	insert_after(l, w, d)	list_insert_after(l, w, (Cast) d)

#define	for_list(p, l)			for (p = (l? next(l): NULL); p != l && p != NULL; p = next(p))
#define	for_2list(p1, p2, l1, l2)	for (p1 = (l1? next(l1): NULL), p2 = (l2? next(l2): NULL); p1 != l1 && p1 != NULL && p2 != l2 && p2 != NULL; p1 = next(p1), p2 = next(p2))
#define for_unlist(p, np, l)    	for (p = (l? next(l): NULL), np = (p? next(p): NULL); p != l && p != NULL; p = np, np = (p? next(p): NULL))
#define	end_list(p, l)			(p == l || p == NULL)

extern	List	*makelist0(void);
extern	List	*list_makelist(Cast);
extern	List	*list_addhead(List *, Cast);
extern	List	*list_addtail(List *, Cast);
extern	List	*addlist(List *, List *);
extern	List	*addndlist(List *, List *);
extern	void	list_insert_before(List *, List *, Cast);
extern	void	list_insert_after(List *, List *, Cast);
extern	int	length(List *);
extern	void	delete(List *, List *, void (*)(Cast));
extern	void	oldlist(List *, void (*)(Cast));

#endif
