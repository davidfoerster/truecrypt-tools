/*
 * utils.h
 *
 *  Created on: 08.03.2012
 *      Author: malte
 */

#pragma once
#ifndef UTILS_H_
#define UTILS_H_

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>


#define VOID(x) ((void)(x))
#define UNUSED(x) VOID(sizeof(x))

#define elementsof(x) (sizeof(x) / sizeof(*x))
#define array_end(a) ((a) + elementsof(a))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#ifdef NDEBUG
#	define verify(expr) unused_value((int)(expr))
#else
#	define verify(expr) assert(expr)
#endif

static inline
bool inrange(long a, long min, long max);

static inline
bool timespec_iszero(const struct timespec *t);

static inline
double timespec_subtract(const struct timespec *x, const struct timespec *y);

static inline
bool streq(const char *a, const char *b);

static inline
bool streq1(const char *a, int b1);

static inline
bool add_ul_checked(unsigned long a, unsigned long b, unsigned long *sum_);

static inline
bool mul_ul_checked(unsigned long a, unsigned long b, unsigned long *product);


#ifdef NDEBUG
#	undef assert
#	define assert(expr) ((void)sizeof(expr))

#	define UNEXPECTED_STATE() ((void)0)
#else
#	define UNEXPECTED_STATE() ({ \
		fprintf(stderr, "%s (%s:%d): Unexpected program state\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
		abort(); \
	})
#endif



// implementations ========================================

bool inrange(long a, long min, long max)
{
	return (unsigned long)(a - min) < (unsigned long)(max - min);
}


bool timespec_iszero(const struct timespec *t)
{
	return (t->tv_sec | t->tv_nsec) == 0;
}


double timespec_subtract(const struct timespec *x, const struct timespec *y)
{
	return (double)(x->tv_sec - y->tv_sec) + (double)(x->tv_nsec - y->tv_nsec) * 1e-9;
}


bool streq(const char *a, const char *b)
{
	return strcmp(a, b) == 0;
}


bool streq1(const char *a, int b1)
{
	return a[0] == b1 && a[1] == '\0';
}


bool add_ul_checked(unsigned long a, unsigned long b, unsigned long *sum_)
{
	unsigned long sum = a + b;
	if (sum < a || sum < b)
		return false;
	*sum_ = sum;
	return true;
}


bool mul_ul_checked(unsigned long a, unsigned long b, unsigned long *product)
{
	if (b > 1 && a > ULONG_MAX / b)
		return false;
	*product = a * b;
	return true;
}


static inline void unused_value( int unused )
{
	UNUSED(unused);
}

#endif /* UTILS_H_ */
