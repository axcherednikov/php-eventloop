/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php_eventloop.h"

#ifdef HAVE_POLL

#include <poll.h>

#define POLL_INITIAL_CAPACITY 64

static struct pollfd *pollfds;
static uint32_t pollfds_size;
static uint32_t pollfds_capacity;

/* Map from pollfd index -> callback pointer */
static eventloop_callback **pollfd_cbs;

/* Map from fd -> pollfd index for quick removal */
static HashTable fd_to_index;

static int eventloop_poll_init(void)
{
	pollfds = ecalloc(POLL_INITIAL_CAPACITY, sizeof(struct pollfd));
	pollfd_cbs = ecalloc(POLL_INITIAL_CAPACITY, sizeof(eventloop_callback *));
	pollfds_size = 0;
	pollfds_capacity = POLL_INITIAL_CAPACITY;
	zend_hash_init(&fd_to_index, 16, NULL, NULL, 0);

	return SUCCESS;
}

static void eventloop_poll_shutdown(void)
{
	if (pollfds) {
		efree(pollfds);
		pollfds = NULL;
	}
	if (pollfd_cbs) {
		efree(pollfd_cbs);
		pollfd_cbs = NULL;
	}
	pollfds_size = 0;
	pollfds_capacity = 0;
	zend_hash_destroy(&fd_to_index);
}

static int eventloop_poll_add(eventloop_callback *cb)
{
	uint32_t idx;
	zend_ulong key;
	zval zv;

	if (pollfds_size >= pollfds_capacity) {
		pollfds_capacity *= 2;
		pollfds = erealloc(pollfds, sizeof(struct pollfd) * pollfds_capacity);
		pollfd_cbs = erealloc(pollfd_cbs, sizeof(eventloop_callback *) * pollfds_capacity);
	}

	idx = pollfds_size++;
	pollfds[idx].fd = cb->io.fd;
	pollfds[idx].events = (cb->type == EVENTLOOP_CB_READABLE) ? POLLIN : POLLOUT;
	pollfds[idx].revents = 0;
	pollfd_cbs[idx] = cb;

	/* Store index for quick removal. Use a composite key: fd + type to allow
	 * both readable and writable on the same fd. */
	key = ((zend_ulong)cb->io.fd << 1) | (cb->type == EVENTLOOP_CB_WRITABLE ? 1 : 0);
	ZVAL_LONG(&zv, idx);
	zend_hash_index_update(&fd_to_index, key, &zv);

	return SUCCESS;
}

static void eventloop_poll_remove(eventloop_callback *cb)
{
	zend_ulong key;
	zval *idx_zv;
	uint32_t idx;

	key = ((zend_ulong)cb->io.fd << 1) | (cb->type == EVENTLOOP_CB_WRITABLE ? 1 : 0);
	idx_zv = zend_hash_index_find(&fd_to_index, key);

	if (!idx_zv) {
		return;
	}

	idx = (uint32_t)Z_LVAL_P(idx_zv);
	zend_hash_index_del(&fd_to_index, key);

	/* Swap with last entry to keep array compact */
	pollfds_size--;
	if (idx < pollfds_size) {
		eventloop_callback *moved;
		zend_ulong moved_key;
		zval zv;

		pollfds[idx] = pollfds[pollfds_size];
		pollfd_cbs[idx] = pollfd_cbs[pollfds_size];

		/* Update the swapped entry's index in the map */
		moved = pollfd_cbs[idx];
		moved_key = ((zend_ulong)moved->io.fd << 1) |
			(moved->type == EVENTLOOP_CB_WRITABLE ? 1 : 0);
		ZVAL_LONG(&zv, idx);
		zend_hash_index_update(&fd_to_index, moved_key, &zv);
	}
}

static int eventloop_poll_poll(double timeout)
{
	int timeout_ms;
	int ret;
	uint32_t n;
	uint32_t i;

	if (pollfds_size == 0) {
		/* No fds to poll -- just sleep for the timeout duration */
		if (timeout > 0) {
			usleep((useconds_t)(timeout * 1000000));
		}
		return 0;
	}

	if (timeout < 0) {
		timeout_ms = 1000;
	} else {
		timeout_ms = (int)(timeout * 1000);
	}

	ret = poll(pollfds, pollfds_size, timeout_ms);

	if (ret < 0) {
		if (UNEXPECTED(errno != EINTR)) {
			return -1;
		}
		return 0;
	}

	if (ret == 0) {
		return 0;
	}

	/* Dispatch events. Iterate a snapshot of the current size since
	 * callbacks may modify the array. */
	n = pollfds_size;
	for (i = 0; i < n && i < pollfds_size; i++) {
		if (pollfds[i].revents != 0) {
			eventloop_dispatch_callback(pollfd_cbs[i]);
			pollfds[i].revents = 0;
		}
	}

	return ret;
}

static eventloop_driver poll_driver = {
	"poll",
	eventloop_poll_init,
	eventloop_poll_shutdown,
	eventloop_poll_add,
	eventloop_poll_remove,
	eventloop_poll_poll,
};

eventloop_driver *eventloop_driver_poll_get(void)
{
	return &poll_driver;
}

#endif /* HAVE_POLL */
