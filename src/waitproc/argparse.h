/*
 * argparse.h
 *
 *  Created on: 08.03.2012
 *      Author: malte
 */

#pragma once
#ifndef ARGPARSE_H_
#define ARGPARSE_H_

#include <argp.h>
#include <stdint.h>


enum argument_type {
	ARGUMENT_CHAR,
	ARGUMENT_BYTE,
	ARGUMENT_SHORT,
	ARGUMENT_INT,
	ARGUMENT_LONG,
	ARGUMENT_LONGLONG,
	ARGUMENT_INTMAX_T,
	ARGUMENT_SIZE_T,
	ARGUMENT_FLOAT,
	ARGUMENT_DOUBLE,
	ARGUMENT_PERIOD,

	ARGUMENT_UNSIGNED = 		1 <<  8,

	ARGUMENT_BASE_ANY = 		0 <<  9,
	ARGUMENT_BASE_DECIMAL = 	1 <<  9,
	ARGUMENT_BASE_OCTAL = 		2 <<  9,
	ARGUMENT_BASE_HEXADECIMAL =	3 <<  9
};

static inline
enum argument_type ARGUMENT_TYPE(enum argument_type t) {
	return t & 0xff;
}

static inline
enum argument_type ARGUMENT_BASE(enum argument_type t) {
	return t & ARGUMENT_BASE_HEXADECIMAL;
}


typedef int (*argp_action_callback)(int, const char*, struct argp_state*, void*);


struct argp_action {
	int key;

	enum argp_action_type {
		ARGP_ACTION_END = 0,
		ARGP_ACTION_SET_VALUE,
		ARGP_ACTION_SET_VALUEPTR,
		ARGP_ACTION_SET_ARG,
		ARGP_ACTION_SET_FLAG,
		ARGP_ACTION_UNSET_FLAG,
		ARGP_ACTION_CALLBACK,
		ARGP_ACTION_PARSE
	} action_type;

	union {
		void *p;
		//char *c;
		//unsigned char *uc;
		//short *s;
		//unsigned short *us;
		//int *i;
		//unsigned int *ui;
		//long *l;
		unsigned long *ul;
		//long long *ll;
		//unsigned long long *ull;
		//intmax_t *imax;
		//uintmax_t *uimax;
		//size_t *size;
		//ptrdiff_t *ptrdiff;
		float *f;
		double *d;
		char const **str;
		argp_action_callback callback;
	} target;

	union {
		unsigned long ul;
		unsigned long *ulp;
		void *p;
		enum argument_type argtype;
	} data;
};


error_t argp_handle_argument(
	int key, const char *arg,
	struct argp_state *state,
	struct argp_action *action
);


error_t argp_action_wrapper(int key, char *arg, struct argp_state *state);


#endif /* ARGPARSE_H_ */
