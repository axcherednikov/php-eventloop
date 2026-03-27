/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifndef PHP_EVENTLOOP_H
# define PHP_EVENTLOOP_H

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# include "php.h"
# include "php_network.h"

# ifdef HAVE_FIBERS
#  include "zend_fibers.h"
# endif

# define PHP_EVENTLOOP_VERSION "1.0.0"

extern zend_module_entry eventloop_module_entry;
# define phpext_eventloop_ptr &eventloop_module_entry

/* Forward declarations */
typedef struct _eventloop_callback eventloop_callback;
typedef struct _eventloop_timer_heap eventloop_timer_heap;

/* {{{ I/O driver interface */
typedef struct _eventloop_driver {
	const char *name;
	int (*init)(void);
	void (*shutdown)(void);
	int (*add)(eventloop_callback *cb);
	void (*remove)(eventloop_callback *cb);
	int (*poll)(double timeout);
} eventloop_driver;
/* }}} */

/* {{{ Callback types */
typedef enum {
	EVENTLOOP_CB_DEFER    = 0,
	EVENTLOOP_CB_DELAY    = 1,
	EVENTLOOP_CB_REPEAT   = 2,
	EVENTLOOP_CB_READABLE = 3,
	EVENTLOOP_CB_WRITABLE = 4,
	EVENTLOOP_CB_SIGNAL   = 5,
} eventloop_cb_type;

# define EVENTLOOP_CB_FLAG_ENABLED    (1 << 0)
# define EVENTLOOP_CB_FLAG_REFERENCED (1 << 1)
# define EVENTLOOP_CB_FLAG_CANCELLED  (1 << 2)
/* }}} */

/* {{{ Callback structure */
struct _eventloop_callback {
	zend_string *id;
	eventloop_cb_type type;
	uint8_t flags;
	zval closure;

	union {
		struct {
			/* no extra data */
		} defer;
		struct {
			double delay;
			double expiry;
		} delay;
		struct {
			double interval;
			double expiry;
		} repeat;
		struct {
			zval stream;
			php_socket_t fd;
		} io;
		struct {
			int signo;
		} signal;
	};

	uint32_t heap_index; /* Position in timer heap (for delay/repeat) */
};
/* }}} */

/* {{{ Timer min-heap */
struct _eventloop_timer_heap {
	eventloop_callback **data;
	uint32_t size;
	uint32_t capacity;
};
/* }}} */

/* {{{ Microtask entry */
typedef struct _eventloop_microtask {
	zval closure;
	zval args;
} eventloop_microtask;
/* }}} */

/* {{{ Module globals */
ZEND_BEGIN_MODULE_GLOBALS(eventloop)
	HashTable callbacks;

	zend_string **deferred_queue;
	uint32_t deferred_count;
	uint32_t deferred_capacity;

	eventloop_microtask *microtask_queue;
	uint32_t microtask_count;
	uint32_t microtask_capacity;

	eventloop_timer_heap timer_heap;

	HashTable signal_callbacks;

	eventloop_driver *driver;

	uint64_t next_id;
	uint32_t io_watcher_count;
	bool running;
	bool stopped;

	zval error_handler;
ZEND_END_MODULE_GLOBALS(eventloop)

ZEND_EXTERN_MODULE_GLOBALS(eventloop)
# define EVENTLOOP_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(eventloop, v)

# if defined(ZTS) && defined(COMPILE_DL_EVENTLOOP)
ZEND_TSRMLS_CACHE_EXTERN()
# endif
/* }}} */

extern zend_class_entry *eventloop_ce;
extern zend_class_entry *eventloop_callback_type_ce;
extern zend_class_entry *eventloop_invalid_callback_error_ce;
# ifdef HAVE_FIBERS
extern zend_class_entry *eventloop_suspension_ce;
# endif

/* Time utility */
double eventloop_now(void);

/* Callback management */
eventloop_callback *eventloop_cb_create(eventloop_cb_type type, zval *closure);
void eventloop_cb_free(eventloop_callback *cb);
eventloop_callback *eventloop_cb_find(const zend_string *id);
void eventloop_cb_enable(eventloop_callback *cb);
void eventloop_cb_disable(eventloop_callback *cb);
void eventloop_cb_cancel(eventloop_callback *cb);
bool eventloop_has_referenced_callbacks(void);

/* Timer heap */
void eventloop_timer_heap_init(eventloop_timer_heap *heap);
void eventloop_timer_heap_destroy(eventloop_timer_heap *heap);
void eventloop_timer_heap_push(eventloop_timer_heap *heap, eventloop_callback *cb);
eventloop_callback *eventloop_timer_heap_peek(const eventloop_timer_heap *heap);
eventloop_callback *eventloop_timer_heap_pop(eventloop_timer_heap *heap);
void eventloop_timer_heap_remove(eventloop_timer_heap *heap, eventloop_callback *cb);
void eventloop_timer_heap_update(eventloop_timer_heap *heap, eventloop_callback *cb);

# ifdef HAVE_FIBERS
/* Suspension */
void eventloop_suspension_init(void);
# endif

/* Drivers */
eventloop_driver *eventloop_driver_select_get(void);
# ifdef HAVE_POLL
eventloop_driver *eventloop_driver_poll_get(void);
# endif
# ifdef HAVE_EPOLL
eventloop_driver *eventloop_driver_epoll_get(void);
# endif
# ifdef HAVE_KQUEUE
eventloop_driver *eventloop_driver_kqueue_get(void);
# endif

eventloop_driver *eventloop_select_best_driver(void);

/* Internal */
void eventloop_dispatch_callback(eventloop_callback *cb);
void eventloop_process_microtasks(void);
void eventloop_process_deferred(void);
void eventloop_process_timers(void);

#endif	/* PHP_EVENTLOOP_H */
