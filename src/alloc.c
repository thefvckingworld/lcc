#include "c.h"
struct block { // 每个内存块块头的数据结构声明
	struct block *next; // 用链表将其串联起来
	char *limit; // 可分配去的末地址
	char *avail; // 可分配区的首地址
};
union align { // 这边的align可以表示系统内最小对齐地址。
	long l;
	char *p;
	double d;
	int (*f)(void);
};
union header {
	struct block b;
	union align a;
};
#ifdef PURIFY
union header *arena[3];

void *allocate(unsigned long n, unsigned a) {
	union header *new = malloc(sizeof *new + n);

	assert(a < NELEMS(arena));
	if (new == NULL) {
		error("insufficient memory\n");
		exit(1);
	}
	new->b.next = (void *)arena[a];
	arena[a] = new;
	return new + 1;
}

void deallocate(unsigned a) {
	union header *p, *q;

	assert(a < NELEMS(arena));
	for (p = arena[a]; p; p = q) {
		q = (void *)p->b.next;
		free(p);
	}
	arena[a] = NULL;
}

void *newarray(unsigned long m, unsigned long n, unsigned a) {
	return allocate(m*n, a);
}
#else
static struct block
	 first[] = {  { NULL },  { NULL },  { NULL } },
	*arena[] = { &first[0], &first[1], &first[2] };
static struct block *freeblocks;

void *allocate(unsigned long n, unsigned a) {
	struct block *ap;

	assert(a < NELEMS(arena));
	assert(n > 0);
	ap = arena[a];
	n = roundup(n, sizeof (union align));
	while (n > ap->limit - ap->avail) {
		if ((ap->next = freeblocks) != NULL) {
			freeblocks = freeblocks->next;
			ap = ap->next;
		} else
			{
				unsigned m = sizeof (union header) + n + roundup(10*1024, sizeof (union align));
				ap->next = malloc(m);
				ap = ap->next;
				if (ap == NULL) {
					error("insufficient memory\n");
					exit(1);
				}
				ap->limit = (char *)ap + m;
			}
		ap->avail = (char *)((union header *)ap + 1);
		ap->next = NULL;
		arena[a] = ap;             // 分配区指向的都是最后一块block

	}
	ap->avail += n;
	return ap->avail - n;
}

void *newarray(unsigned long m, unsigned long n, unsigned a) {
	return allocate(m*n, a);
}
void deallocate(unsigned a) {
	assert(a < NELEMS(arena));
	arena[a]->next = freeblocks;
	freeblocks = first[a].next;
	first[a].next = NULL;
	arena[a] = &first[a];
}
#endif
