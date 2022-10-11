#ifndef __DRM_TEST_UTIL_H
#define __DRM_TEST_UTIL_H
#include <stddef.h>

#define link_container_of(ptr, sample, member) \
	(__typeof__(sample))((char *)(ptr) - \
			     offsetof(__typeof__(*sample), member))

struct list_link;

typedef struct list_link {
  struct list_link * next;
} list_link;

typedef struct linked_list {
	list_link link;
	void * ptr;
} linked_list;

#endif