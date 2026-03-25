/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php_eventloop.h"
#include "zend_exceptions.h"
#include "zend_closures.h"
#include "zend_enum.h"
#include "ext/standard/info.h"

#ifdef HAVE_CLOCK_GETTIME
# include <time.h>
#else
# include <sys/time.h>
#endif

#ifndef PHP_WIN32
# include <signal.h>
#endif

#include "eventloop_arginfo.h"

ZEND_DECLARE_MODULE_GLOBALS(eventloop)

zend_class_entry *eventloop_ce;
zend_class_entry *eventloop_callback_type_ce;
zend_class_entry *eventloop_invalid_callback_error_ce;
#ifdef HAVE_FIBERS
zend_class_entry *eventloop_suspension_ce;
#endif

static zval callback_type_cases[6];

/* {{{ eventloop_now */
double eventloop_now(void)
{
#ifdef HAVE_CLOCK_GETTIME
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#else
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
#endif
}
/* }}} */

/* {{{ eventloop_select_best_driver */
eventloop_driver *eventloop_select_best_driver(void)
{
	eventloop_driver *d;

#ifdef HAVE_EPOLL
	d = eventloop_driver_epoll_get();
	if (d && d->init() == SUCCESS) {
		return d;
	}
#endif
#ifdef HAVE_KQUEUE
	d = eventloop_driver_kqueue_get();
	if (d && d->init() == SUCCESS) {
		return d;
	}
#endif
#ifdef HAVE_POLL
	d = eventloop_driver_poll_get();
	if (d && d->init() == SUCCESS) {
		return d;
	}
#endif
	d = eventloop_driver_select_get();
	if (d && d->init() == SUCCESS) {
		return d;
	}

	return NULL;
}
/* }}} */

/* {{{ eventloop_dispatch_callback */
void eventloop_dispatch_callback(eventloop_callback *cb)
{
	zval retval;
	zval params[1];
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;
	char *errstr = NULL;

	if (UNEXPECTED(!(cb->flags & EVENTLOOP_CB_FLAG_ENABLED) ||
	    (cb->flags & EVENTLOOP_CB_FLAG_CANCELLED))) {
		return;
	}

	/* For defer/delay: auto-cancel after execution */
	if (cb->type == EVENTLOOP_CB_DEFER || cb->type == EVENTLOOP_CB_DELAY) {
		cb->flags &= ~EVENTLOOP_CB_FLAG_ENABLED;
		cb->flags |= EVENTLOOP_CB_FLAG_CANCELLED;
	}

	ZVAL_STR_COPY(params, cb->id);

	if (UNEXPECTED(zend_fcall_info_init(&cb->closure, 0, &fci, &fcc, NULL, &errstr) != SUCCESS)) {
		if (errstr) {
			php_error_docref(NULL, E_WARNING, "Invalid callback: %s", errstr);
			efree(errstr);
		}
		zval_ptr_dtor(params);
		return;
	}

	ZVAL_UNDEF(&retval);
	fci.retval = &retval;
	fci.param_count = 1;
	fci.params = params;

	if (zend_call_function(&fci, &fcc) == SUCCESS) {
		zval_ptr_dtor(&retval);
	}

	if (UNEXPECTED(EG(exception))) {
		if (Z_TYPE(EVENTLOOP_G(error_handler)) != IS_NULL) {
			zval error_params[1];
			zval error_retval;
			zval exception_zv;
			zend_fcall_info error_fci;
			zend_fcall_info_cache error_fcc;

			ZVAL_OBJ_COPY(&exception_zv, EG(exception));
			zend_clear_exception();
			ZVAL_COPY_VALUE(&error_params[0], &exception_zv);

			if (zend_fcall_info_init(&EVENTLOOP_G(error_handler), 0,
			    &error_fci, &error_fcc, NULL, NULL) == SUCCESS) {
				ZVAL_UNDEF(&error_retval);
				error_fci.retval = &error_retval;
				error_fci.param_count = 1;
				error_fci.params = error_params;

				if (zend_call_function(&error_fci, &error_fcc) == SUCCESS) {
					zval_ptr_dtor(&error_retval);
				}

				if (UNEXPECTED(EG(exception))) {
					EVENTLOOP_G(stopped) = true;
				}
			}
			zval_ptr_dtor(&exception_zv);
		} else {
			EVENTLOOP_G(stopped) = true;
		}
	}

	zval_ptr_dtor(params);

	if (cb->type == EVENTLOOP_CB_DEFER || cb->type == EVENTLOOP_CB_DELAY) {
		zend_hash_del(&EVENTLOOP_G(callbacks), cb->id);
	}
}
/* }}} */

/* {{{ eventloop_process_microtasks */
void eventloop_process_microtasks(void)
{
	eventloop_microtask mt;
	zval retval;
	zend_fcall_info fci;
	zend_fcall_info_cache fcc;

	while (EVENTLOOP_G(microtask_count) > 0) {
		mt = EVENTLOOP_G(microtask_queue)[0];

		EVENTLOOP_G(microtask_count)--;
		if (EVENTLOOP_G(microtask_count) > 0) {
			memmove(&EVENTLOOP_G(microtask_queue)[0],
				&EVENTLOOP_G(microtask_queue)[1],
				sizeof(eventloop_microtask) * EVENTLOOP_G(microtask_count));
		}

		if (zend_fcall_info_init(&mt.closure, 0, &fci, &fcc, NULL, NULL) == SUCCESS) {
			ZVAL_UNDEF(&retval);
			fci.retval = &retval;

			if (Z_TYPE(mt.args) == IS_ARRAY) {
				zend_fcall_info_args(&fci, &mt.args);
			} else {
				fci.param_count = 0;
				fci.params = NULL;
			}

			if (zend_call_function(&fci, &fcc) == SUCCESS) {
				zval_ptr_dtor(&retval);
			}

			if (Z_TYPE(mt.args) == IS_ARRAY) {
				zend_fcall_info_args_clear(&fci, 1);
			}

			if (UNEXPECTED(EG(exception)) && Z_TYPE(EVENTLOOP_G(error_handler)) == IS_NULL) {
				EVENTLOOP_G(stopped) = true;
			}
		}

		zval_ptr_dtor(&mt.closure);
		zval_ptr_dtor(&mt.args);
	}
}
/* }}} */

/* {{{ eventloop_process_deferred */
void eventloop_process_deferred(void)
{
	uint32_t count;
	uint32_t i;
	zend_string **snapshot;
	eventloop_callback *cb;

	if (EVENTLOOP_G(deferred_count) == 0) {
		return;
	}

	/* Take a snapshot of current deferred IDs to avoid infinite loops
	 * if callbacks register new deferred callbacks. */
	count = EVENTLOOP_G(deferred_count);
	snapshot = emalloc(sizeof(zend_string *) * count);
	memcpy(snapshot, EVENTLOOP_G(deferred_queue), sizeof(zend_string *) * count);

	EVENTLOOP_G(deferred_count) = 0;

	for (i = 0; i < count; i++) {
		if (EVENTLOOP_G(stopped)) {
			break;
		}

		cb = eventloop_cb_find(snapshot[i]);
		if (cb && (cb->flags & EVENTLOOP_CB_FLAG_ENABLED) &&
		    !(cb->flags & EVENTLOOP_CB_FLAG_CANCELLED)) {
			eventloop_dispatch_callback(cb);
		}
		zend_string_release(snapshot[i]);
	}

	/* Release remaining entries skipped due to loop stop */
	for (; i < count; i++) {
		zend_string_release(snapshot[i]);
	}

	efree(snapshot);
}
/* }}} */

/* {{{ eventloop_process_timers */
void eventloop_process_timers(void)
{
	double now;
	double expiry;
	eventloop_callback *cb;

	now = eventloop_now();

	while ((cb = eventloop_timer_heap_peek(&EVENTLOOP_G(timer_heap))) != NULL) {
		expiry = (cb->type == EVENTLOOP_CB_DELAY) ?
			cb->delay.expiry : cb->repeat.expiry;

		if (expiry > now) {
			break;
		}

		eventloop_timer_heap_pop(&EVENTLOOP_G(timer_heap));

		if (!(cb->flags & EVENTLOOP_CB_FLAG_ENABLED) ||
		    (cb->flags & EVENTLOOP_CB_FLAG_CANCELLED)) {
			continue;
		}

		if (cb->type == EVENTLOOP_CB_REPEAT) {
			cb->repeat.expiry = now + cb->repeat.interval;
			eventloop_timer_heap_push(&EVENTLOOP_G(timer_heap), cb);
		}

		eventloop_dispatch_callback(cb);

		if (EVENTLOOP_G(stopped)) {
			break;
		}
	}
}
/* }}} */

#ifndef PHP_WIN32
static volatile sig_atomic_t pending_signals[32];

static void eventloop_signal_handler(int signo)
{
	if (signo >= 0 && signo < 32) {
		pending_signals[signo] = 1;
	}
}

static void eventloop_process_signals(void)
{
	int i;
	zval *id_zv;
	zend_string *id;
	eventloop_callback *cb;

	for (i = 0; i < 32; i++) {
		if (pending_signals[i]) {
			pending_signals[i] = 0;

			id_zv = zend_hash_index_find(&EVENTLOOP_G(signal_callbacks), (zend_ulong)i);
			if (id_zv) {
				id = Z_STR_P(id_zv);
				cb = eventloop_cb_find(id);
				if (cb && (cb->flags & EVENTLOOP_CB_FLAG_ENABLED) &&
				    !(cb->flags & EVENTLOOP_CB_FLAG_CANCELLED)) {
					eventloop_dispatch_callback(cb);
				}
			}
		}
	}
}
#endif

/* {{{ proto InvalidCallbackError::__construct(string $callbackId, string $message) */
ZEND_METHOD(EventLoop_InvalidCallbackError, __construct)
{
	zend_string *callback_id;
	zend_string *message;
	zval zv;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_STR(callback_id)
		Z_PARAM_STR(message)
	ZEND_PARSE_PARAMETERS_END();

	/* Set the readonly $callbackId property */
	ZVAL_STR_COPY(&zv, callback_id);
	zend_update_property_str(eventloop_invalid_callback_error_ce,
		Z_OBJ_P(ZEND_THIS), "callbackId", sizeof("callbackId") - 1, callback_id);
	zval_ptr_dtor(&zv);

	/* Set the $message property via parent Error */
	zend_update_property_str(zend_ce_error,
		Z_OBJ_P(ZEND_THIS), "message", sizeof("message") - 1, message);
}
/* }}} */

/* {{{ proto string InvalidCallbackError::getCallbackId() */
ZEND_METHOD(EventLoop_InvalidCallbackError, getCallbackId)
{
	zval rv;

	ZEND_PARSE_PARAMETERS_NONE();

	RETURN_COPY(zend_read_property(eventloop_invalid_callback_error_ce,
		Z_OBJ_P(ZEND_THIS), "callbackId", sizeof("callbackId") - 1, 0, &rv));
}
/* }}} */

static void eventloop_throw_invalid_callback(zend_string *id)
{
	zval obj;
	zval args[2];
	zend_string *msg;

	msg = zend_strpprintf(0, "Invalid callback identifier \"%s\"", ZSTR_VAL(id));

	object_init_ex(&obj, eventloop_invalid_callback_error_ce);

	ZVAL_STR_COPY(&args[0], id);
	ZVAL_STR(&args[1], msg);

	zend_call_known_instance_method_with_2_params(
		eventloop_invalid_callback_error_ce->constructor,
		Z_OBJ(obj), NULL, &args[0], &args[1]);

	zval_ptr_dtor(&args[0]);
	zval_ptr_dtor(&args[1]);

	zend_throw_exception_object(&obj);
}

static eventloop_callback *eventloop_find_or_throw(zend_string *id)
{
	eventloop_callback *cb;

	cb = eventloop_cb_find(id);
	if (UNEXPECTED(!cb)) {
		eventloop_throw_invalid_callback(id);
		return NULL;
	}
	return cb;
}

static php_socket_t eventloop_stream_to_fd(zval *stream_zv)
{
	php_stream *stream;
	php_socket_t fd;

	stream = (php_stream *)zend_fetch_resource2_ex(
		stream_zv, "stream", php_file_le_stream(), php_file_le_pstream());

	if (UNEXPECTED(!stream)) {
		zend_throw_error(NULL, "Expected a valid stream resource");
		return -1;
	}

	if (UNEXPECTED(php_stream_cast(stream,
	    PHP_STREAM_AS_FD_FOR_SELECT | PHP_STREAM_CAST_INTERNAL,
	    (void **)&fd, 1) != SUCCESS || fd < 0)) {
		zend_throw_error(NULL, "Cannot obtain file descriptor from stream");
		return -1;
	}

	return fd;
}

/* {{{ proto void EventLoop\EventLoop::queue(\Closure $closure, mixed ...$args) */
ZEND_METHOD(EventLoop_EventLoop, queue)
{
	zval *closure;
	zval *args;
	uint32_t argc;
	eventloop_microtask *mt;
	uint32_t i;
	zval tmp;

	ZEND_PARSE_PARAMETERS_START(1, -1)
		Z_PARAM_OBJECT_OF_CLASS(closure, zend_ce_closure)
		Z_PARAM_VARIADIC('+', args, argc)
	ZEND_PARSE_PARAMETERS_END();

	if (EVENTLOOP_G(microtask_count) >= EVENTLOOP_G(microtask_capacity)) {
		EVENTLOOP_G(microtask_capacity) = EVENTLOOP_G(microtask_capacity) ?
			EVENTLOOP_G(microtask_capacity) * 2 : 8;
		EVENTLOOP_G(microtask_queue) = erealloc(EVENTLOOP_G(microtask_queue),
			sizeof(eventloop_microtask) * EVENTLOOP_G(microtask_capacity));
	}

	mt = &EVENTLOOP_G(microtask_queue)[EVENTLOOP_G(microtask_count)++];
	ZVAL_COPY(&mt->closure, closure);

	if (argc > 0) {
		array_init_size(&mt->args, argc);
		for (i = 0; i < argc; i++) {
			ZVAL_COPY(&tmp, &args[i]);
			zend_hash_next_index_insert(Z_ARRVAL(mt->args), &tmp);
		}
	} else {
		ZVAL_NULL(&mt->args);
	}
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::defer(\Closure $closure) */
ZEND_METHOD(EventLoop_EventLoop, defer)
{
	zval *closure;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(closure, zend_ce_closure)
	ZEND_PARSE_PARAMETERS_END();

	cb = eventloop_cb_create(EVENTLOOP_CB_DEFER, closure);

	if (EVENTLOOP_G(deferred_count) >= EVENTLOOP_G(deferred_capacity)) {
		EVENTLOOP_G(deferred_capacity) = EVENTLOOP_G(deferred_capacity) ?
			EVENTLOOP_G(deferred_capacity) * 2 : 8;
		EVENTLOOP_G(deferred_queue) = erealloc(EVENTLOOP_G(deferred_queue),
			sizeof(zend_string *) * EVENTLOOP_G(deferred_capacity));
	}

	EVENTLOOP_G(deferred_queue)[EVENTLOOP_G(deferred_count)++] =
		zend_string_copy(cb->id);

	RETURN_STR_COPY(cb->id);
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::delay(float $delay, \Closure $closure) */
ZEND_METHOD(EventLoop_EventLoop, delay)
{
	double delay;
	zval *closure;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_DOUBLE(delay)
		Z_PARAM_OBJECT_OF_CLASS(closure, zend_ce_closure)
	ZEND_PARSE_PARAMETERS_END();

	if (UNEXPECTED(delay < 0)) {
		zend_argument_value_error(1, "must be non-negative, %f given", delay);
		RETURN_THROWS();
	}

	cb = eventloop_cb_create(EVENTLOOP_CB_DELAY, closure);
	cb->delay.delay = delay;
	cb->delay.expiry = eventloop_now() + delay;

	eventloop_timer_heap_push(&EVENTLOOP_G(timer_heap), cb);

	RETURN_STR_COPY(cb->id);
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::repeat(float $interval, \Closure $closure) */
ZEND_METHOD(EventLoop_EventLoop, repeat)
{
	double interval;
	zval *closure;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_DOUBLE(interval)
		Z_PARAM_OBJECT_OF_CLASS(closure, zend_ce_closure)
	ZEND_PARSE_PARAMETERS_END();

	if (UNEXPECTED(interval < 0)) {
		zend_argument_value_error(1, "must be non-negative, %f given", interval);
		RETURN_THROWS();
	}

	cb = eventloop_cb_create(EVENTLOOP_CB_REPEAT, closure);
	cb->repeat.interval = interval;
	cb->repeat.expiry = eventloop_now() + interval;

	eventloop_timer_heap_push(&EVENTLOOP_G(timer_heap), cb);

	RETURN_STR_COPY(cb->id);
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::onReadable(mixed $stream, \Closure $closure) */
ZEND_METHOD(EventLoop_EventLoop, onReadable)
{
	zval *stream_zv;
	zval *closure;
	php_socket_t fd;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_ZVAL(stream_zv)
		Z_PARAM_OBJECT_OF_CLASS(closure, zend_ce_closure)
	ZEND_PARSE_PARAMETERS_END();

	fd = eventloop_stream_to_fd(stream_zv);
	if (UNEXPECTED(fd < 0)) {
		RETURN_THROWS();
	}

	cb = eventloop_cb_create(EVENTLOOP_CB_READABLE, closure);
	ZVAL_COPY(&cb->io.stream, stream_zv);
	cb->io.fd = fd;

	if (EVENTLOOP_G(driver)) {
		if (UNEXPECTED(EVENTLOOP_G(driver)->add(cb) != SUCCESS)) {
			eventloop_cb_cancel(cb);
			zend_throw_error(NULL, "Failed to register stream for reading");
			RETURN_THROWS();
		}
	}

	RETURN_STR_COPY(cb->id);
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::onWritable(mixed $stream, \Closure $closure) */
ZEND_METHOD(EventLoop_EventLoop, onWritable)
{
	zval *stream_zv;
	zval *closure;
	php_socket_t fd;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_ZVAL(stream_zv)
		Z_PARAM_OBJECT_OF_CLASS(closure, zend_ce_closure)
	ZEND_PARSE_PARAMETERS_END();

	fd = eventloop_stream_to_fd(stream_zv);
	if (UNEXPECTED(fd < 0)) {
		RETURN_THROWS();
	}

	cb = eventloop_cb_create(EVENTLOOP_CB_WRITABLE, closure);
	ZVAL_COPY(&cb->io.stream, stream_zv);
	cb->io.fd = fd;

	if (EVENTLOOP_G(driver)) {
		if (UNEXPECTED(EVENTLOOP_G(driver)->add(cb) != SUCCESS)) {
			eventloop_cb_cancel(cb);
			zend_throw_error(NULL, "Failed to register stream for writing");
			RETURN_THROWS();
		}
	}

	RETURN_STR_COPY(cb->id);
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::onSignal(int $signal, \Closure $closure) */
ZEND_METHOD(EventLoop_EventLoop, onSignal)
{
	zend_long signal;
	zval *closure;
	eventloop_callback *cb;
	zval id_zv;
#ifndef PHP_WIN32
	struct sigaction sa;
#endif

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_LONG(signal)
		Z_PARAM_OBJECT_OF_CLASS(closure, zend_ce_closure)
	ZEND_PARSE_PARAMETERS_END();

#ifdef PHP_WIN32
	zend_throw_error(NULL, "Signal handling is not supported on Windows");
	RETURN_THROWS();
#else
	if (UNEXPECTED(signal < 1 || signal >= 32)) {
		zend_argument_value_error(1, "must be a valid signal number between 1 and 31");
		RETURN_THROWS();
	}

	cb = eventloop_cb_create(EVENTLOOP_CB_SIGNAL, closure);
	cb->signal.signo = (int)signal;

	ZVAL_STR_COPY(&id_zv, cb->id);
	zend_hash_index_update(&EVENTLOOP_G(signal_callbacks), (zend_ulong)signal, &id_zv);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = eventloop_signal_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction((int)signal, &sa, NULL);

	RETURN_STR_COPY(cb->id);
#endif
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::enable(string $callbackId) */
ZEND_METHOD(EventLoop_EventLoop, enable)
{
	zend_string *id;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(id)
	ZEND_PARSE_PARAMETERS_END();

	cb = eventloop_find_or_throw(id);
	if (UNEXPECTED(!cb)) {
		RETURN_THROWS();
	}

	eventloop_cb_enable(cb);
	RETURN_STR_COPY(id);
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::disable(string $callbackId) */
ZEND_METHOD(EventLoop_EventLoop, disable)
{
	zend_string *id;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(id)
	ZEND_PARSE_PARAMETERS_END();

	cb = eventloop_find_or_throw(id);
	if (UNEXPECTED(!cb)) {
		RETURN_THROWS();
	}

	eventloop_cb_disable(cb);
	RETURN_STR_COPY(id);
}
/* }}} */

/* {{{ proto void EventLoop\EventLoop::cancel(string $callbackId) */
ZEND_METHOD(EventLoop_EventLoop, cancel)
{
	zend_string *id;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(id)
	ZEND_PARSE_PARAMETERS_END();

	cb = eventloop_cb_find(id);
	if (cb) {
		eventloop_cb_cancel(cb);
	}
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::reference(string $callbackId) */
ZEND_METHOD(EventLoop_EventLoop, reference)
{
	zend_string *id;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(id)
	ZEND_PARSE_PARAMETERS_END();

	cb = eventloop_find_or_throw(id);
	if (UNEXPECTED(!cb)) {
		RETURN_THROWS();
	}

	cb->flags |= EVENTLOOP_CB_FLAG_REFERENCED;
	RETURN_STR_COPY(id);
}
/* }}} */

/* {{{ proto string EventLoop\EventLoop::unreference(string $callbackId) */
ZEND_METHOD(EventLoop_EventLoop, unreference)
{
	zend_string *id;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(id)
	ZEND_PARSE_PARAMETERS_END();

	cb = eventloop_find_or_throw(id);
	if (UNEXPECTED(!cb)) {
		RETURN_THROWS();
	}

	cb->flags &= ~EVENTLOOP_CB_FLAG_REFERENCED;
	RETURN_STR_COPY(id);
}
/* }}} */

/* {{{ proto bool EventLoop\EventLoop::isEnabled(string $callbackId) */
ZEND_METHOD(EventLoop_EventLoop, isEnabled)
{
	zend_string *id;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(id)
	ZEND_PARSE_PARAMETERS_END();

	cb = eventloop_find_or_throw(id);
	if (UNEXPECTED(!cb)) {
		RETURN_THROWS();
	}

	RETURN_BOOL(cb->flags & EVENTLOOP_CB_FLAG_ENABLED);
}
/* }}} */

/* {{{ proto bool EventLoop\EventLoop::isReferenced(string $callbackId) */
ZEND_METHOD(EventLoop_EventLoop, isReferenced)
{
	zend_string *id;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(id)
	ZEND_PARSE_PARAMETERS_END();

	cb = eventloop_find_or_throw(id);
	if (UNEXPECTED(!cb)) {
		RETURN_THROWS();
	}

	RETURN_BOOL(cb->flags & EVENTLOOP_CB_FLAG_REFERENCED);
}
/* }}} */

static void eventloop_ensure_type_cache(void)
{
	const char *names[] = {"Defer", "Delay", "Repeat", "Readable", "Writable", "Signal"};
	int i;
	zend_string *name;
	zend_class_constant *c;
	zval *case_zv;

	if (Z_TYPE(callback_type_cases[0]) != IS_UNDEF) {
		return;
	}

	for (i = 0; i < 6; i++) {
		name = zend_string_init(names[i], strlen(names[i]), 0);
		c = zend_hash_find_ptr(
			&eventloop_callback_type_ce->constants_table, name);
		if (c) {
			case_zv = &c->value;
			if (Z_TYPE_P(case_zv) == IS_CONSTANT_AST) {
				zval_update_constant_ex(case_zv, eventloop_callback_type_ce);
			}
			ZVAL_COPY(&callback_type_cases[i], case_zv);
		} else {
			ZVAL_NULL(&callback_type_cases[i]);
		}
		zend_string_release(name);
	}
}

/* {{{ proto CallbackType EventLoop\EventLoop::getType(string $callbackId) */
ZEND_METHOD(EventLoop_EventLoop, getType)
{
	zend_string *id;
	eventloop_callback *cb;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_STR(id)
	ZEND_PARSE_PARAMETERS_END();

	cb = eventloop_find_or_throw(id);
	if (UNEXPECTED(!cb)) {
		RETURN_THROWS();
	}

	eventloop_ensure_type_cache();
	RETURN_COPY(&callback_type_cases[cb->type]);
}
/* }}} */

/* {{{ proto array EventLoop\EventLoop::getIdentifiers() */
ZEND_METHOD(EventLoop_EventLoop, getIdentifiers)
{
	eventloop_callback *cb;
	zval id;

	ZEND_PARSE_PARAMETERS_NONE();

	array_init(return_value);

	ZEND_HASH_FOREACH_PTR(&EVENTLOOP_G(callbacks), cb) {
		if (!(cb->flags & EVENTLOOP_CB_FLAG_CANCELLED)) {
			ZVAL_STR_COPY(&id, cb->id);
			zend_hash_next_index_insert(Z_ARRVAL_P(return_value), &id);
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */

/* {{{ proto void EventLoop\EventLoop::run() */
ZEND_METHOD(EventLoop_EventLoop, run)
{
	eventloop_callback *cb;
	eventloop_callback *next_timer;
	double timeout;
	double expiry;

	ZEND_PARSE_PARAMETERS_NONE();

	if (UNEXPECTED(EVENTLOOP_G(running))) {
		zend_throw_error(NULL, "The event loop is already running");
		RETURN_THROWS();
	}

	if (!EVENTLOOP_G(driver)) {
		EVENTLOOP_G(driver) = eventloop_select_best_driver();
		if (UNEXPECTED(!EVENTLOOP_G(driver))) {
			zend_throw_error(NULL, "No suitable I/O driver available");
			RETURN_THROWS();
		}
	}

	/* Register any I/O callbacks that were added before the driver was initialized */
	ZEND_HASH_FOREACH_PTR(&EVENTLOOP_G(callbacks), cb) {
		if ((cb->type == EVENTLOOP_CB_READABLE || cb->type == EVENTLOOP_CB_WRITABLE) &&
		    (cb->flags & EVENTLOOP_CB_FLAG_ENABLED) &&
		    !(cb->flags & EVENTLOOP_CB_FLAG_CANCELLED)) {
			EVENTLOOP_G(driver)->add(cb);
		}
	} ZEND_HASH_FOREACH_END();

	EVENTLOOP_G(running) = true;
	EVENTLOOP_G(stopped) = false;

	while (!EVENTLOOP_G(stopped)) {
		eventloop_process_microtasks();
		if (EVENTLOOP_G(stopped)) {
			break;
		}

		eventloop_process_deferred();
		if (EVENTLOOP_G(stopped)) {
			break;
		}

		if (!eventloop_has_referenced_callbacks() &&
		    EVENTLOOP_G(microtask_count) == 0 &&
		    EVENTLOOP_G(deferred_count) == 0) {
			break;
		}

		timeout = -1.0;
		next_timer = eventloop_timer_heap_peek(&EVENTLOOP_G(timer_heap));
		if (next_timer) {
			expiry = (next_timer->type == EVENTLOOP_CB_DELAY) ?
				next_timer->delay.expiry : next_timer->repeat.expiry;
			timeout = expiry - eventloop_now();
			if (timeout < 0) {
				timeout = 0;
			}
		}

		EVENTLOOP_G(driver)->poll(timeout);
		if (EVENTLOOP_G(stopped)) {
			break;
		}

		eventloop_process_timers();
		if (EVENTLOOP_G(stopped)) {
			break;
		}

#ifndef PHP_WIN32
		eventloop_process_signals();
#endif

		eventloop_process_microtasks();
	}

	EVENTLOOP_G(running) = false;
}
/* }}} */

/* {{{ proto void EventLoop\EventLoop::stop() */
ZEND_METHOD(EventLoop_EventLoop, stop)
{
	ZEND_PARSE_PARAMETERS_NONE();
	EVENTLOOP_G(stopped) = true;
}
/* }}} */

/* {{{ proto bool EventLoop\EventLoop::isRunning() */
ZEND_METHOD(EventLoop_EventLoop, isRunning)
{
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_BOOL(EVENTLOOP_G(running));
}
/* }}} */

/* {{{ proto void EventLoop\EventLoop::setErrorHandler(?\Closure $errorHandler) */
ZEND_METHOD(EventLoop_EventLoop, setErrorHandler)
{
	zval *handler;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS_OR_NULL(handler, zend_ce_closure)
	ZEND_PARSE_PARAMETERS_END();

	zval_ptr_dtor(&EVENTLOOP_G(error_handler));

	if (handler) {
		ZVAL_COPY(&EVENTLOOP_G(error_handler), handler);
	} else {
		ZVAL_NULL(&EVENTLOOP_G(error_handler));
	}
}
/* }}} */

/* {{{ proto ?\Closure EventLoop\EventLoop::getErrorHandler() */
ZEND_METHOD(EventLoop_EventLoop, getErrorHandler)
{
	ZEND_PARSE_PARAMETERS_NONE();

	if (Z_TYPE(EVENTLOOP_G(error_handler)) != IS_NULL) {
		RETURN_COPY(&EVENTLOOP_G(error_handler));
	}

	RETURN_NULL();
}
/* }}} */

#ifdef HAVE_FIBERS
/* {{{ proto Suspension EventLoop\EventLoop::getSuspension() */
ZEND_METHOD(EventLoop_EventLoop, getSuspension)
{
	ZEND_PARSE_PARAMETERS_NONE();
	object_init_ex(return_value, eventloop_suspension_ce);
}
/* }}} */
#endif

/* {{{ proto string EventLoop\EventLoop::getDriver() */
ZEND_METHOD(EventLoop_EventLoop, getDriver)
{
	ZEND_PARSE_PARAMETERS_NONE();

	if (EVENTLOOP_G(driver)) {
		RETURN_STRING(EVENTLOOP_G(driver)->name);
	}

#ifdef HAVE_EPOLL
	RETURN_STRING("epoll");
#elif defined(HAVE_KQUEUE)
	RETURN_STRING("kqueue");
#elif defined(HAVE_POLL)
	RETURN_STRING("poll");
#else
	RETURN_STRING("select");
#endif
}
/* }}} */

static void eventloop_callback_dtor(zval *zv)
{
	eventloop_cb_free(Z_PTR_P(zv));
}

/* {{{ PHP_GINIT_FUNCTION */
static PHP_GINIT_FUNCTION(eventloop)
{
#if defined(COMPILE_DL_EVENTLOOP) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	memset(eventloop_globals, 0, sizeof(*eventloop_globals));
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(eventloop)
{
	int i;

	eventloop_callback_type_ce = register_class_EventLoop_CallbackType();

	for (i = 0; i < 6; i++) {
		ZVAL_UNDEF(&callback_type_cases[i]);
	}

	eventloop_invalid_callback_error_ce = register_class_EventLoop_InvalidCallbackError(
		zend_ce_error);

	eventloop_ce = register_class_EventLoop_EventLoop();

#ifdef HAVE_FIBERS
	eventloop_suspension_init();
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(eventloop)
{
	int i;

	for (i = 0; i < 6; i++) {
		zval_ptr_dtor(&callback_type_cases[i]);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(eventloop)
{
	zend_hash_init(&EVENTLOOP_G(callbacks), 32, NULL, eventloop_callback_dtor, 0);
	zend_hash_init(&EVENTLOOP_G(signal_callbacks), 4, NULL, ZVAL_PTR_DTOR, 0);
	eventloop_timer_heap_init(&EVENTLOOP_G(timer_heap));

	EVENTLOOP_G(deferred_queue) = NULL;
	EVENTLOOP_G(deferred_count) = 0;
	EVENTLOOP_G(deferred_capacity) = 0;

	EVENTLOOP_G(microtask_queue) = NULL;
	EVENTLOOP_G(microtask_count) = 0;
	EVENTLOOP_G(microtask_capacity) = 0;

	EVENTLOOP_G(driver) = NULL;
	EVENTLOOP_G(next_id) = 1;
	EVENTLOOP_G(running) = false;
	EVENTLOOP_G(stopped) = false;
	ZVAL_NULL(&EVENTLOOP_G(error_handler));

#ifndef PHP_WIN32
	memset((void *)pending_signals, 0, sizeof(pending_signals));
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(eventloop)
{
	uint32_t i;

	if (EVENTLOOP_G(driver)) {
		EVENTLOOP_G(driver)->shutdown();
		EVENTLOOP_G(driver) = NULL;
	}

	for (i = 0; i < EVENTLOOP_G(microtask_count); i++) {
		zval_ptr_dtor(&EVENTLOOP_G(microtask_queue)[i].closure);
		zval_ptr_dtor(&EVENTLOOP_G(microtask_queue)[i].args);
	}
	if (EVENTLOOP_G(microtask_queue)) {
		efree(EVENTLOOP_G(microtask_queue));
	}

	for (i = 0; i < EVENTLOOP_G(deferred_count); i++) {
		zend_string_release(EVENTLOOP_G(deferred_queue)[i]);
	}
	if (EVENTLOOP_G(deferred_queue)) {
		efree(EVENTLOOP_G(deferred_queue));
	}

	eventloop_timer_heap_destroy(&EVENTLOOP_G(timer_heap));
	zval_ptr_dtor(&EVENTLOOP_G(error_handler));
	zend_hash_destroy(&EVENTLOOP_G(signal_callbacks));
	zend_hash_destroy(&EVENTLOOP_G(callbacks));

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(eventloop)
{
	const char *driver_name = "select";

#ifdef HAVE_EPOLL
	driver_name = "epoll";
#elif defined(HAVE_KQUEUE)
	driver_name = "kqueue";
#elif defined(HAVE_POLL)
	driver_name = "poll";
#endif

	php_info_print_table_start();
	php_info_print_table_header(2, "EventLoop support", "enabled");
	php_info_print_table_row(2, "Version", PHP_EVENTLOOP_VERSION);
	php_info_print_table_row(2, "Best available driver", driver_name);
#ifdef HAVE_EPOLL
	php_info_print_table_row(2, "epoll support", "yes");
#else
	php_info_print_table_row(2, "epoll support", "no");
#endif
#ifdef HAVE_KQUEUE
	php_info_print_table_row(2, "kqueue support", "yes");
#else
	php_info_print_table_row(2, "kqueue support", "no");
#endif
#ifdef HAVE_POLL
	php_info_print_table_row(2, "poll support", "yes");
#else
	php_info_print_table_row(2, "poll support", "no");
#endif
	php_info_print_table_row(2, "select support", "yes");
	php_info_print_table_end();
}
/* }}} */

zend_module_entry eventloop_module_entry = {
	STANDARD_MODULE_HEADER,
	"eventloop",						/* Extension name */
	NULL,								/* zend_function_entry */
	PHP_MINIT(eventloop),				/* PHP_MINIT */
	PHP_MSHUTDOWN(eventloop),			/* PHP_MSHUTDOWN */
	PHP_RINIT(eventloop),				/* PHP_RINIT */
	PHP_RSHUTDOWN(eventloop),			/* PHP_RSHUTDOWN */
	PHP_MINFO(eventloop),				/* PHP_MINFO */
	PHP_EVENTLOOP_VERSION,				/* Version */
	PHP_MODULE_GLOBALS(eventloop),		/* Module globals */
	PHP_GINIT(eventloop),				/* PHP_GINIT */
	NULL,								/* PHP_GSHUTDOWN */
	NULL,								/* Post deactivate */
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_EVENTLOOP
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(eventloop)
#endif
