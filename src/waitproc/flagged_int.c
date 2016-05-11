/*
 * flagged_int.c
 *
 *  Created on: 08.03.2012
 *      Author: malte
 */

#ifndef _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 200112L
#endif
#define INLINE
#include "flagged_int.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#ifndef FLAGGED_BUFFER_INITIALLENGTH
#	define FLAGGED_BUFFER_INITIALLENGTH 4U
#endif


bool a_flagged_int_init(struct a_flagged_int *a, size_t size)
{
	assert(!a->values);

	if (!size)
		size = FLAGGED_BUFFER_INITIALLENGTH;

	a->values = malloc(size * sizeof(*a->values));
#ifndef NDEBUG
	if (a->values) memset(a->values, 0x55, size * sizeof(*a->values));
#endif
	a->size = a->values ? size : 0;
	a->length = 0;

	return !!a->values;
}


void a_flagged_int_free(struct a_flagged_int *a)
{
	if (a->values) {
		assert(a->length <= a->size);
		free(a->values);
#ifndef NDEBUG
		memset(a, 0x55, sizeof(*a));
#endif
	}
}


struct flagged_int *get_flagged_int(struct a_flagged_int *a, long val)
{
	struct flagged_int *p = a->values;
	const struct flagged_int *const p_end = a_flagged_int_end(a);
	for (; p != p_end; p++) {
		if (p->i == val)
			return p;
	}
	return NULL;
}


void a_flagged_each(struct a_flagged_int *a, a_flagged_callback callback, void *data)
{
	struct flagged_int *p = a->values;
	const struct flagged_int *const p_end = p + a->length;
	while (p != p_end && p->valid && callback(p, data))
		p++;
}


bool push_flagged_int(struct a_flagged_int *a, long value, bool valid)
{
	struct flagged_int *elem;

	if (a->length >= a->size) {
		assert(a->length == a->size);
		if (!a->values) {
			assert(a->size == 0);
			a->size = FLAGGED_BUFFER_INITIALLENGTH;
		} if (a->size > SIZE_MAX / 2) {
			if (a->size == SIZE_MAX) {
				assert(a->size != SIZE_MAX);
				errno = ERANGE;
				return false;
			}
			a->size = SIZE_MAX;
		} else {
			do {
				a->size *= 2;
			} while (a->size <= a->length);
		}

		if (!(elem = realloc(a->values, a->size * sizeof(*a->values))))
			return false;
		a->values = elem;
	}

	elem = &a->values[a->length++];
	elem->i = value;
	elem->state = STATE_INITIAL;
	elem->valid = valid;

	return true;
}


void a_flagged_int_slice(struct a_flagged_int *a, ssize_t start_, size_t count)
{
	size_t start = (size_t) ((start_ >= 0) ? 
		start_ : 
		(start_ + (ssize_t) a->length));
	if (start < a->length) {
		if (start + count > a->length)
			count = a->length - start;
		memmove(&a->values[start], &a->values[start + count], (a->length - start - count) * sizeof(a->values));
		a->length -= count;
	}
}


ssize_t a_flagged_int_remove(struct a_flagged_int *a, struct flagged_int *p)
{
	ssize_t index = p - a->values;
	if ((size_t) index < a->length) {
		a_flagged_int_slice(a, index, 1);
		return index;
	}
	return -1;
}


#ifndef NDBEUG
INLINE struct flagged_int *a_flagged_int_end(struct a_flagged_int *a);
#endif
