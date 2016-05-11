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


#define elementsof(x) (sizeof(x) / sizeof(*x))
#define array_end(a) ((a) + elementsof(a))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))


#ifdef NDEBUG
#	define verify(expr) (expr)
#else
#	define verify(expr) assert(expr)
#endif


// help CDT indexer
#if !defined __timespec_defined || !__timespec_defined
#	define __timespec_defined	1
#	include <bits/types.h>	/* This defines __time_t for us.  */
struct timespec {
	__time_t tv_sec;		/* Seconds.  */
	long int tv_nsec;		/* Nanoseconds.  */
};
#endif /* timespec not defined and <time.h> or need timespec.  */

#ifndef CLOCK_MONOTONIC
#	define CLOCK_MONOTONIC 1
#endif
// end: help CDT indexer


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
#if __CHAR_BIT__ == 8
# if __BYTE_ORDER == __LITTLE_ENDIAN
	return (*(unsigned int*)a & ((1U << (2*sizeof(*a)*CHAR_BIT)) - 1)) == b1;
# elif __BYTE_ORDER == __BIG_ENDIAN
	return ((*(unsigned int*)a >> ((sizeof(unsigned int) - 2*sizeof(char))*CHAR_BIT)) & ((1U << (2*sizeof(*a)*CHAR_BIT)) - 1)) == b1;
# endif
#endif
#if __CHAR_BIT__ != 8 || (__BYTE_ORDER != __LITTLE_ENDIAN && __BYTE_ORDER != __BIG_ENDIAN)
	return a[0] == b1 && a[1] == '\0';
#endif
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

#endif /* UTILS_H_ */
