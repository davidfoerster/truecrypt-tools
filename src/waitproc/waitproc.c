#ifndef _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 200112L
#endif
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <assert.h>

#include "utils.h"
#include "flagged_int.h"
#include "argparse.h"


#ifndef DEBUG
# ifdef NDEBUG
#	define DEBUG 0
# else
#	define DEBUG 1
# endif
#endif


typedef unsigned long flag_t;

static
struct waitproc_options {
	struct a_flagged_int pids;
	long interval_sec;
	flag_t flags;

	struct timespec wait_start;
	int last_signal;
}
waitproc_options = { 0 };


enum waitproc_flags_position {
	WAITPROC_FLAG_DISJUNCTIVE,
	WAITPROC_FLAG_TERMINATE,
	WAITPROC_FLAG_KILL,
	WAITPROC_FLAG_QUIET,
	WAITPROC_FLAG_ALARMSET,
	WAITPROC_FLAG_ERROROCCURED,
	_WAITPROC_FLAG_COUNT
};

static inline
flag_t waitproc_flag_frompos(enum waitproc_flags_position flagpos)
{
	assert(flagpos >= 0 && flagpos < _WAITPROC_FLAG_COUNT);
	return 1UL << flagpos;
}

static inline
bool waitproc_flags_test(enum waitproc_flags_position flagpos)
{
	return waitproc_options.flags & waitproc_flag_frompos(flagpos);
}

static inline
void waitproc_flags_set(enum waitproc_flags_position flagpos)
{
	waitproc_options.flags |= waitproc_flag_frompos(flagpos);
}

static inline
void waitproc_flags_unset(enum waitproc_flags_position flagpos)
{
	waitproc_options.flags &= ~waitproc_flag_frompos(flagpos);
}

static inline
void waitproc_flags_setc(enum waitproc_flags_position flagpos, bool value)
{
	const flag_t mask = waitproc_flag_frompos(flagpos);
	assert((unsigned) value <= 1);
	waitproc_options.flags = (waitproc_options.flags & ~mask) | ((flag_t) value << flagpos);
}


struct trace_data {
	int count;
	enum flagged_state target_state;
	union {
		const int *signal;
		enum __ptrace_request trace_type;
	} d;
	bool error_occured;
};


struct trace_data *trace_data_init(struct trace_data *data)
{
	data->count = 0;
	data->error_occured = false;
	return data;
}


void print_terminated_process(pid_t pid)
{
#if DEBUG
	struct timespec now;
	int r;
#endif

	if (!waitproc_flags_test(WAITPROC_FLAG_QUIET)) {

#if DEBUG
		if (!timespec_iszero(&waitproc_options.wait_start)) {
			r = clock_gettime(CLOCK_MONOTONIC, &now);
			assert(r == 0);
		} else {
			r = -1;
		}
#endif

		printf(
#if DEBUG
			"[%10.6g s] "
#endif
			"%i\n",
#if DEBUG
			!r ? timespec_subtract(&now, &waitproc_options.wait_start) : 0.0,
#endif
			pid
		);

	}
}


#define SIGINVALID -1


bool send_signal(struct flagged_int *p, void *data_)
{
	struct trace_data *data = (struct trace_data*) data_;
	const int *signal;
	int count = 0;

	if (inrange(p->state, STATE_ATTACHED, STATE_DETACHED)) {
		for (signal = data->d.signal; *signal != SIGINVALID; signal++) {
			if (kill((pid_t) p->i, *signal) == 0) {
				p->state = data->target_state;
				if (data->target_state == STATE_TERMINATED) p->valid = false;
				count++;
			} else {
				p->valid = false;
				switch (errno) {
					case ESRCH:
						p->state = STATE_TERMINATED;
						break;

					default:
						assert(errno != EINVAL);
						UNEXPECTED_STATE();
						break;
				}
				break;
			}
		}
	}

	if (count)
		data->count++;

	return true;
}


bool attach_process(struct flagged_int *p, void *data_)
{
	struct trace_data *data = (struct trace_data*) data_;
	if (p->state < STATE_ATTACHED) {
		if (ptrace(PTRACE_ATTACH, (pid_t) p->i, 0, 0) == 0) {
			p->state = STATE_ATTACHED;
			data->count++;
		} else {
			p->valid = false;
			switch (errno) {
				case ESRCH:
					p->state = STATE_TERMINATED;
					print_terminated_process((pid_t) p->i);
					break;

				case EPERM:
					warnx("No permission to attach to PID %li.", p->i);
					data->error_occured = true;
					break;

				default:
					data->error_occured = true;
					perror("ptrace attach");
					break;
			}
		}
	}
	return true;
}


bool detach_process(struct flagged_int *p, void *data_)
{
	struct trace_data *data = (struct trace_data*) data_;
	if (p->state < data->target_state) {
		long r;
		if (p->state < STATE_ATTACHED) {
			r = 0;
		} else {
			if (p->state >= STATE_CONTINUED) {
				r = kill((pid_t) p->i, SIGSTOP);
				if (r != 0 && errno == ESRCH) {
					p->state = STATE_TERMINATED;
					p->valid = false;
					print_terminated_process((pid_t) p->i);
					return true;
				}
				assert(r == 0);
			}
			r = ptrace(data->d.trace_type, (pid_t) p->i, 0, 0);
		}

		if (r == 0) {
			p->state = data->target_state;
			if (data->target_state == STATE_TERMINATED)
				print_terminated_process((pid_t) p->i);
			data->count++;
		} else {
			p->valid = false;
			data->error_occured = true;
			perror("ptrace attach");
		}
	}
	return true;
}


void signal_handler_store_signum(int signal)
{
	waitproc_options.last_signal = signal;
}


void set_alarm()
{
	assert(!waitproc_flags_test(WAITPROC_FLAG_ALARMSET) && timespec_iszero(&waitproc_options.wait_start));

	verify(clock_gettime(CLOCK_MONOTONIC, &waitproc_options.wait_start) == 0);

	if (waitproc_options.interval_sec) {
		struct sigaction action;
		memset(&action, 0, sizeof(action));
		action.sa_handler = &signal_handler_store_signum;

		waitproc_options.last_signal = SIGINVALID;
		sigaction(SIGALRM, &action, NULL);
		assert(inrange(waitproc_options.interval_sec, 1, UINT_MAX+1));
		alarm((unsigned int) waitproc_options.interval_sec);
		waitproc_flags_set(WAITPROC_FLAG_ALARMSET);
	}
}


bool handle_wait(pid_t pid, int status, int count, int *continued, int *terminated)
{
	unsigned int alarm_remaining_sec = waitproc_flags_test(WAITPROC_FLAG_ALARMSET) ? alarm(0) : 0;

	if (pid != -1) {
		struct flagged_int *p = get_flagged_int(&waitproc_options.pids, pid);
		assert(p);

		if (WIFSTOPPED(status)) {
			long r = WSTOPSIG(status);
			assert(p->valid);
			switch (r) {
				case SIGSTOP:
					if (p->state == STATE_ATTACHED) {
						p->state = STATE_CONTINUED;
						r = 0;
						assert(*continued < count);
						if (++(*continued) == count)
							set_alarm();
					} else {
						r = SIGCONT;
					}
					break;

				case SIGTSTP:
					r = SIGCONT;
					break;

				default:
					// forward the signal
					break;
			}
			if (r >= 0) {
				assert(p->state >= STATE_ATTACHED);
				r = ptrace(PTRACE_CONT, pid, 0, r);
				assert(r == 0);
			}
		} else if (WIFEXITED(status) || WIFSIGNALED(status)) {
			assert(p->valid && p->state < STATE_TERMINATED);
			p->state = STATE_TERMINATED;
			p->valid = false;
			print_terminated_process(pid);
			if (++(*terminated) == count)
				return false;
		}
	} else switch (errno) {
		case EINTR:
			if (waitproc_options.last_signal == SIGALRM) {
				waitproc_options.last_signal = SIGINVALID;
				if (DEBUG || !alarm_remaining_sec)
					return false;
			} else {
				fputs("Interrupted by unexpected signal\n", stderr);
				UNEXPECTED_STATE();
			}
			break;

		case ECHILD:
			*terminated = count;
			return false;

		default:
			UNEXPECTED_STATE();
			break;
	}

	assert(*terminated < count);

	if (alarm_remaining_sec)
		alarm(alarm_remaining_sec);

	return true;
}


static const int termination_signals[] = { SIGHUP, SIGTERM, SIGINVALID };

int wait_pids()
{
	struct trace_data data;
	int count, continued, terminated;
	pid_t pid; int wait_status;

	a_flagged_each(&waitproc_options.pids, &attach_process, trace_data_init(&data));
	waitproc_flags_setc(WAITPROC_FLAG_ERROROCCURED, data.error_occured);
	if (data.count <= 0)
		return data.count;
	count = data.count;

	if (waitproc_flags_test(WAITPROC_FLAG_TERMINATE)) {
		data.target_state = STATE_ATTACHED;
		data.d.signal = termination_signals;
		a_flagged_each(&waitproc_options.pids, &send_signal, trace_data_init(&data));
		waitproc_flags_setc(WAITPROC_FLAG_ERROROCCURED, data.error_occured);
		if (data.count <= 0)
			return data.count;
	}

	continued = 0;
	terminated = 0;
	do {
		pid = waitpid(-1, &wait_status, WUNTRACED);
	} while (handle_wait(pid, wait_status, count, &continued, &terminated));

	if (waitproc_flags_test(WAITPROC_FLAG_KILL)) {
		data.target_state = STATE_TERMINATED;
		data.d.trace_type = PTRACE_KILL;
		terminated = count;
	} else {
		data.target_state = STATE_DETACHED;
		data.d.trace_type = PTRACE_DETACH;
	}
	a_flagged_each(&waitproc_options.pids, &detach_process, trace_data_init(&data));
	waitproc_flags_setc(WAITPROC_FLAG_ERROROCCURED, data.error_occured);

	return terminated;
}


int parse_pids(unsigned int argc, char *argv[])
{
	unsigned int argp;
	int pid, charcount;

	for (argp = 0; argp < argc; argp++) {
		if (sscanf(argv[argp], "%i%n", &pid, &charcount) > 0 && !argv[argp][charcount]) {
			push_flagged_int(&waitproc_options.pids, pid, true);
		}
	}

	return (int) min(waitproc_options.pids.length, (size_t)INT_MAX);
}


int parse_options(int key, char *arg, struct argp_state *state)
{
	int r;

	if ((r = argp_action_wrapper(key, arg, state)) != ARGP_ERR_UNKNOWN) {
		switch (key) {
		case 'i':
			if (r == EINVAL || !inrange(waitproc_options.interval_sec, 1, UINT_MAX+1)) {
				argp_error(state, "'%s' is not a time period from 1 to %u seconds.", arg, UINT_MAX);
				return EINVAL;
			}
			break;
		}
		return r;
	}

	switch (key) {
	case ARGP_KEY_ARG: {
		unsigned int remainig_argc = (unsigned int)(state->argc - state->next + 1);
		a_flagged_int_init(&waitproc_options.pids, remainig_argc);
		if (parse_pids(remainig_argc, &state->argv[state->next-1]) > 0) {
			state->next = state->argc;
			return 0;
		}
		// else: fall through
	}

	case ARGP_KEY_NO_ARGS:
		argp_usage(state);
		// don't return or fall through

	}

	return ARGP_ERR_UNKNOWN;
}


static struct argp_option const argp_options[] = {
	{ "interval",		'i', "INTERVAL", 0,
		"Wait no longer than INTERVAL for the PIDs to finish. "
		"The letters w, d, h, m, and s may be used to express an interval "
		"in weeks, days, hours, minutes, or seconds respectively. Multiple "
		"such expressions may be used in that order to add their intervals. "
		"The default is to use seconds.",
		0 },

	{ "disjunctive",	'd', NULL, 0,
		"Exit with status 0, if at least one PID terminated while waiting for it. "
		"The default is to fail, if at least one did not terminate.",
		0 },

	{ "terminate",		't', NULL, 0,
		"Send SIGTERM to each PID before waiting for them to terminate.",
		0 },

	{ "kill", 			'k', NULL, 0,
		"Send SIGKILL to each PID still running after INTERVAL.",
		0 },

	{ "quiet", 			'q', NULL, 0,
		"Don't write anything to stdout. Normally, when a PID terminates, "
		"we immediately print a line with that PID.",
		0 },

	{ 0 }
};

static struct argp const argp = {
	argp_options, &parse_options,

	"PID...",
	"waitproc waits for a set of processes, each specified by its PID, to terminate. "
	"It can limit the waiting period, ask these processes to terminate, and even "
	"kill them if necessary."
	"\v"
	"waitproc does not need to poll to determine, if a process is still alive. "
	"Instead it hooks into it with ptrace() and waits for it to terminate. "
	"As a consequence the parents of these processes cannot wait() for them anymore "
	"as long as we wait for them. If --terminate or --kill are in effect, "
	"SIGSTOP and SIGTSTP are intercepted and dropped. All other signals are "
	"forwarded.\n"
	"Depending on your system configuration, only root might be able to ptrace "
	"for security reasons. An attacker might use ptrace (and therefore waitproc) "
	"to prevent a process parent from waiting for it itself.",

	NULL, NULL, NULL
};


static struct argp_action argp_actions[] = {
	{ 'q', ARGP_ACTION_SET_FLAG, { &waitproc_options.flags }, { 1UL << WAITPROC_FLAG_QUIET } },
	{ 'd', ARGP_ACTION_SET_FLAG, { &waitproc_options.flags }, { 1UL << WAITPROC_FLAG_DISJUNCTIVE } },
	{ 't', ARGP_ACTION_SET_FLAG, { &waitproc_options.flags }, { 1UL << WAITPROC_FLAG_TERMINATE } },
	{ 'k', ARGP_ACTION_SET_FLAG, { &waitproc_options.flags }, { 1UL << WAITPROC_FLAG_KILL } },
	{ 'i', ARGP_ACTION_PARSE, { &waitproc_options.interval_sec }, { ARGUMENT_PERIOD } },
	{ 0 }
};


int main(int argc, char *argv[])
{
	int result;

	argp_parse(&argp, argc, argv, 0, NULL, argp_actions);

	result = wait_pids();
	result = (
		result > 0 && (
			!waitproc_flags_test(WAITPROC_FLAG_ERROROCCURED) ||
			 waitproc_flags_test(WAITPROC_FLAG_DISJUNCTIVE)
		))
		?	EXIT_SUCCESS
		:	EXIT_FAILURE;

	// clean up
	a_flagged_int_free(&waitproc_options.pids);

	return result;
}
