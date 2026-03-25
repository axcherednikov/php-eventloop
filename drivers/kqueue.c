/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php_eventloop.h"

#ifdef HAVE_KQUEUE

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>

#define KQUEUE_MAX_EVENTS   256
#define KQUEUE_CHANGES_INIT 64

static int kq_fd = -1;
static struct kevent *kq_events;

/* Pending changelist — flushed in a single kevent() call during poll */
static struct kevent *kq_changes;
static uint32_t kq_nchanges;
static uint32_t kq_changes_capacity;

/* Map fd+filter -> callback */
static HashTable kq_read_cbs;
static HashTable kq_write_cbs;

static void kq_changelist_push(uintptr_t ident, short filter, unsigned short flags, void *udata)
{
	if (kq_nchanges >= kq_changes_capacity) {
		kq_changes_capacity *= 2;
		kq_changes = erealloc(kq_changes, sizeof(struct kevent) * kq_changes_capacity);
	}

	EV_SET(&kq_changes[kq_nchanges], ident, filter, flags, 0, 0, udata);
	kq_nchanges++;
}

static int eventloop_kqueue_init(void)
{
	kq_fd = kqueue();
	if (UNEXPECTED(kq_fd < 0)) {
		php_error_docref(NULL, E_WARNING, "Failed to create kqueue: %s",
			strerror(errno));
		return FAILURE;
	}

	kq_events = ecalloc(KQUEUE_MAX_EVENTS, sizeof(struct kevent));
	kq_changes = ecalloc(KQUEUE_CHANGES_INIT, sizeof(struct kevent));
	kq_nchanges = 0;
	kq_changes_capacity = KQUEUE_CHANGES_INIT;
	zend_hash_init(&kq_read_cbs, 16, NULL, NULL, 0);
	zend_hash_init(&kq_write_cbs, 16, NULL, NULL, 0);

	return SUCCESS;
}

static void eventloop_kqueue_shutdown(void)
{
	if (kq_fd >= 0) {
		close(kq_fd);
		kq_fd = -1;
	}
	if (kq_events) {
		efree(kq_events);
		kq_events = NULL;
	}
	if (kq_changes) {
		efree(kq_changes);
		kq_changes = NULL;
	}
	kq_nchanges = 0;
	kq_changes_capacity = 0;
	zend_hash_destroy(&kq_read_cbs);
	zend_hash_destroy(&kq_write_cbs);
}

static int eventloop_kqueue_add(eventloop_callback *cb)
{
	short filter;

	if (cb->type == EVENTLOOP_CB_READABLE) {
		filter = EVFILT_READ;
		zend_hash_index_update_ptr(&kq_read_cbs, (zend_ulong)cb->io.fd, cb);
	} else {
		filter = EVFILT_WRITE;
		zend_hash_index_update_ptr(&kq_write_cbs, (zend_ulong)cb->io.fd, cb);
	}

	/* Defer the syscall — will be flushed in poll() */
	kq_changelist_push(cb->io.fd, filter, EV_ADD | EV_CLEAR, cb);

	return SUCCESS;
}

static void eventloop_kqueue_remove(eventloop_callback *cb)
{
	short filter;

	if (cb->type == EVENTLOOP_CB_READABLE) {
		filter = EVFILT_READ;
		zend_hash_index_del(&kq_read_cbs, (zend_ulong)cb->io.fd);
	} else {
		filter = EVFILT_WRITE;
		zend_hash_index_del(&kq_write_cbs, (zend_ulong)cb->io.fd);
	}

	/* Defer the syscall — will be flushed in poll() */
	kq_changelist_push(cb->io.fd, filter, EV_DELETE, NULL);
}

static int eventloop_kqueue_poll(double timeout)
{
	struct timespec ts;
	int ret;
	int i;
	eventloop_callback *cb;

	if (timeout < 0) {
		ts.tv_sec = 1;
		ts.tv_nsec = 0;
	} else {
		ts.tv_sec = (time_t)timeout;
		ts.tv_nsec = (long)((timeout - (double)ts.tv_sec) * 1000000000.0);
	}

	/* Single syscall: apply all pending changes AND wait for events */
	ret = kevent(kq_fd, kq_changes, (int)kq_nchanges, kq_events, KQUEUE_MAX_EVENTS, &ts);
	kq_nchanges = 0;

	if (ret < 0) {
		if (UNEXPECTED(errno != EINTR)) {
			return -1;
		}
		return 0;
	}

	for (i = 0; i < ret; i++) {
		cb = (eventloop_callback *)kq_events[i].udata;
		if (cb && (cb->flags & EVENTLOOP_CB_FLAG_ENABLED) &&
		    !(cb->flags & EVENTLOOP_CB_FLAG_CANCELLED)) {
			eventloop_dispatch_callback(cb);
		}
	}

	return ret;
}

static eventloop_driver kqueue_driver = {
	"kqueue",
	eventloop_kqueue_init,
	eventloop_kqueue_shutdown,
	eventloop_kqueue_add,
	eventloop_kqueue_remove,
	eventloop_kqueue_poll,
};

eventloop_driver *eventloop_driver_kqueue_get(void)
{
	return &kqueue_driver;
}

#endif /* HAVE_KQUEUE */
