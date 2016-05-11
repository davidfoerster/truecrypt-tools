/*
 * flagged_int.h
 *
 *  Created on: 08.03.2012
 *      Author: malte
 */

#pragma once
#ifndef FLAGGED_INT_H_
#define FLAGGED_INT_H_

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <assert.h>


#ifndef INLINE
# if __GNUC__ && !__GNUC_STDC_INLINE__
#  define INLINE extern inline
# else
#  define INLINE inline
# endif
#endif



struct flagged_int {
	long i;
	enum flagged_state {
		STATE_UNMODIFIED = -1,
		STATE_INITIAL = 0,
		STATE_ATTACHED,
		STATE_CONTINUED,
		STATE_DETACHED,
		STATE_TERMINATED
	} state;
	bool valid;
};


struct a_flagged_int {
	struct flagged_int *values;
	size_t length, size;
};


typedef bool (*a_flagged_callback)(struct flagged_int *p, void *data);


bool a_flagged_int_init(struct a_flagged_int *a, size_t size);

void a_flagged_int_free(struct a_flagged_int *a);

struct flagged_int *get_flagged_int(struct a_flagged_int *a, long val);

void a_flagged_each(struct a_flagged_int *a, a_flagged_callback callback, void *data);

bool push_flagged_int(struct a_flagged_int *a, long value, bool valid);

void a_flagged_int_slice(struct a_flagged_int *a, ssize_t start_, size_t count);

ssize_t a_flagged_int_remove(struct a_flagged_int *a, struct flagged_int *p);

//inline struct flagged_int *a_flagged_int_end(struct a_flagged_int *a);


// implementations ========================================

#ifdef NDEBUG
static inline
#else
INLINE
#endif
struct flagged_int *a_flagged_int_end(struct a_flagged_int *a)
{
	assert(!!a->values);
	assert(a->length <= a->size);

	return a->values + a->length;
}

#endif /* FLAGGED_INT_H_ */
