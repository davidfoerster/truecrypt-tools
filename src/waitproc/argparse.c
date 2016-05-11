/*
 * argparse.c
 *
 *  Created on: 08.03.2012
 *      Author: malte
 */

#include "argparse.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "utils.h"


// forward declarations ===================================

error_t argp_handle_argument_internal(const char *arg, struct argp_state *state, struct argp_action *action);
error_t argp_handle_argument_parse(const char *arg, struct argp_state *state, struct argp_action *action);
char *argtype_to_conversion_specifier(enum argument_type type, char *fmt);
bool argument_check_validity(const struct argp_action *action);
int parse_period(const char *s, long *period);


// implementation =========================================

error_t argp_action_wrapper(int key, char *arg, struct argp_state *state)
{
	return argp_handle_argument(key, arg, state, state->input);
}


error_t argp_handle_argument(
	int key, const char *arg,
	struct argp_state *state,
	struct argp_action *action
) {
	for (; action->action_type != ARGP_ACTION_END; action++) {
		if (action->key == key) {
			return argp_handle_argument_internal(arg, state, action);
		}
	}
	return ARGP_ERR_UNKNOWN;
}


error_t argp_handle_argument_internal(
	const char *arg,
	struct argp_state *state,
	struct argp_action *action
) {
	switch (action->action_type) {
	case ARGP_ACTION_SET_VALUE:
		*action->target.ul = action->data.ul;
		break;

	case ARGP_ACTION_SET_VALUEPTR:
		*action->target.ul = *action->data.ulp;
		break;

	case ARGP_ACTION_SET_ARG:
		*action->target.str = arg;
		return 0;

	case ARGP_ACTION_SET_FLAG:
		*action->target.ul |= action->data.ul;
		break;

	case ARGP_ACTION_UNSET_FLAG:
		*action->target.ul &= ~action->data.ul;
		break;

	case ARGP_ACTION_CALLBACK:
		return action->target.callback(action->key, arg, state, action->data.p);

	case ARGP_ACTION_PARSE:
		return argp_handle_argument_parse(arg, state, action);

	default:
		assert(false);
		return EINVAL;
	}

	if (arg) {
		argp_error(state, "Option '%s' does not expect an argument; you supplied '%s'.", state->argv[state->next-1], arg);
		return EINVAL;
	}

	return 0;
}


int argp_handle_argument_parse(
	const char *arg,
	struct argp_state *state,
	struct argp_action *action
) {
	if (!arg) {
		argp_error(state, "Option '%s' expects an argument.", state->argv[state->next-1]);
		return EINVAL;
	}

	{
		char fmt[7], *fmt_end;
		if (!!(fmt_end = argtype_to_conversion_specifier(action->data.argtype, fmt + 1))) {
			int r, char_count;
			assert(array_end(fmt) - fmt_end >= 3);

			*fmt = '%';
			strncpy(fmt_end, "%n", (size_t)(array_end(fmt) - fmt_end));

			r = sscanf(arg, fmt, action->target.p, &char_count);
			if (r > 0 && !arg[char_count]) {
				if (argument_check_validity(action))
					return 0;
			} else if (r >= 0) {
				errno = EINVAL;
			} else {
				assert(r != EINVAL);
			}
		}
	}

	switch (ARGUMENT_TYPE(action->data.argtype)) {
	case ARGUMENT_PERIOD:
		if (parse_period(arg, (long*) action->target.p) == 0)
			return 0;
		break;

	default:
		assert(false);
		return EINVAL;
	}

	argp_error(state, "'%s' is an invalid argument to option '%s' because \"%s\".",
		arg, state->argv[state->next-1],
		(errno == EINVAL) ? "Parse error" : strerror(errno));
	return EINVAL;
}


char *argtype_to_conversion_specifier(enum argument_type type, char *fmt)
{
	switch (ARGUMENT_TYPE(type)) {
	case ARGUMENT_CHAR:
		*fmt = 'c';
		return fmt + 1;

	case ARGUMENT_BYTE:
		*fmt++ = 'h';
		// fall through

	case ARGUMENT_SHORT:
		*fmt = 'h';
		break;

	case ARGUMENT_INT:
		break;

	case ARGUMENT_LONG:
		*fmt = 'l';
		break;

	case ARGUMENT_LONGLONG:
		*fmt = 'L';
		break;

	case ARGUMENT_INTMAX_T:
		*fmt = 'j';
		break;

	case ARGUMENT_SIZE_T:
		*fmt = (type & ARGUMENT_UNSIGNED) ? 'z' : 't';
		break;

	case ARGUMENT_DOUBLE:
		*fmt++ = 'l';
		// fall through
	case ARGUMENT_FLOAT:
		*fmt = 'f';
		return fmt + 1;

	default:
		return NULL;
	}

	fmt++;

	switch (ARGUMENT_BASE(type)) {
	case ARGUMENT_BASE_ANY:
		*fmt = 'i';
		break;

	case ARGUMENT_BASE_DECIMAL:
		*fmt = (type & ARGUMENT_UNSIGNED) ? 'u' : 'd';
		break;

	case ARGUMENT_BASE_OCTAL:
		*fmt = 'o';
		break;

	case ARGUMENT_BASE_HEXADECIMAL:
		*fmt = 'x';
		break;

	default:
		assert(false);
		break;
	}

	return fmt + 1;
}


bool argument_check_validity(const struct argp_action *action)
{
	if (!(action->data.argtype & ARGUMENT_UNSIGNED) ||
			!((action->data.argtype & ARGUMENT_FLOAT) && *action->target.f < 0) ||
			!((action->data.argtype & ARGUMENT_DOUBLE) && *action->target.d < 0)
	) {
		return true;
	} else {
		errno = ERANGE;
		return false;
	}
}


inline int return_error(error_t e)
{
	errno = e;
	return -1;
}

int parse_period(const char *s, long *period_)
{
	static const char units[] = { 'w', 'd', 'h', 'm', 's', '\0' };
	static const unsigned long factors[] = {
		60 * 60 * 24 * 7,
		60 * 60 * 24,
		60 * 60,
		60,
		1, 1
	};

	unsigned long period = 0, n;
	const char *unit, *last_unit = NULL;
	int c;
	bool negative;

	assert(elementsof(units) == elementsof(factors));

	if ((negative = (*s == '-')) || *s == '+')
		s++;

	for (; sscanf(s, "%lu%n", &n, &c) > 0; s += c + 1) {
		unit = memchr(units, s[c], elementsof(units));
		if (unit <= last_unit)
			return return_error(EINVAL);
		last_unit = unit;

		if (!mul_ul_checked(n, factors[unit - units], &n))
			return return_error(ERANGE);

		if (!add_ul_checked(period, n, &period))
			return return_error(ERANGE);

		if (!s[c] || s[c] == 's') {
			if (negative) {
				if (period > (unsigned long) LONG_MAX + 1)
					return return_error(ERANGE);
				*period_ = -(long) period;
			} else {
				if (period > LONG_MAX)
					return return_error(ERANGE);
				*period_ = (long) period;
			}
			return 0;
		}
	}

	assert(errno != EINVAL);
	return -1;
}
