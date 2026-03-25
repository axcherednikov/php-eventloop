/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php_eventloop.h"

#ifdef PHP_WIN32
# include "win32/select.h"
#else
# include <sys/select.h>
#endif

static fd_set read_fds;
static fd_set write_fds;
static php_socket_t max_fd;

/* Track which callbacks are registered, keyed by fd */
static HashTable read_cbs;
static HashTable write_cbs;

static int eventloop_select_init(void)
{
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	max_fd = 0;
	zend_hash_init(&read_cbs, 8, NULL, NULL, 0);
	zend_hash_init(&write_cbs, 8, NULL, NULL, 0);

	return SUCCESS;
}

static void eventloop_select_shutdown(void)
{
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	zend_hash_destroy(&read_cbs);
	zend_hash_destroy(&write_cbs);
}

static int eventloop_select_add(eventloop_callback *cb)
{
	if (cb->io.fd < 0 || cb->io.fd >= FD_SETSIZE) {
		php_error_docref(NULL, E_WARNING,
			"File descriptor %d exceeds FD_SETSIZE (%d)", (int)cb->io.fd, FD_SETSIZE);
		return FAILURE;
	}

	if (cb->type == EVENTLOOP_CB_READABLE) {
		FD_SET(cb->io.fd, &read_fds);
		zend_hash_index_update_ptr(&read_cbs, (zend_ulong)cb->io.fd, cb);
	} else {
		FD_SET(cb->io.fd, &write_fds);
		zend_hash_index_update_ptr(&write_cbs, (zend_ulong)cb->io.fd, cb);
	}

	if (cb->io.fd > max_fd) {
		max_fd = cb->io.fd;
	}

	return SUCCESS;
}

static void eventloop_select_remove(eventloop_callback *cb)
{
	if (cb->io.fd < 0 || cb->io.fd >= FD_SETSIZE) {
		return;
	}

	if (cb->type == EVENTLOOP_CB_READABLE) {
		FD_CLR(cb->io.fd, &read_fds);
		zend_hash_index_del(&read_cbs, (zend_ulong)cb->io.fd);
	} else {
		FD_CLR(cb->io.fd, &write_fds);
		zend_hash_index_del(&write_cbs, (zend_ulong)cb->io.fd);
	}
}

static int eventloop_select_poll(double timeout)
{
	fd_set tmp_read, tmp_write;
	struct timeval tv;
	eventloop_callback *cb;
	zend_ulong fd;
	int ret;

	memcpy(&tmp_read, &read_fds, sizeof(fd_set));
	memcpy(&tmp_write, &write_fds, sizeof(fd_set));

	if (timeout < 0) {
		/* Block indefinitely */
		tv.tv_sec = 1;
		tv.tv_usec = 0;
	} else {
		tv.tv_sec = (long)timeout;
		tv.tv_usec = (long)((timeout - (double)tv.tv_sec) * 1000000.0);
	}

	ret = select((int)(max_fd + 1), &tmp_read, &tmp_write, NULL, &tv);

	if (ret < 0) {
#ifndef PHP_WIN32
		if (errno == EINTR) {
			return 0;
		}
#endif
		return -1;
	}

	if (ret == 0) {
		return 0;
	}

	/* Dispatch readable events */
	ZEND_HASH_FOREACH_NUM_KEY_PTR(&read_cbs, fd, cb) {
		if (FD_ISSET((php_socket_t)fd, &tmp_read)) {
			eventloop_dispatch_callback(cb);
		}
	} ZEND_HASH_FOREACH_END();

	/* Dispatch writable events */
	ZEND_HASH_FOREACH_NUM_KEY_PTR(&write_cbs, fd, cb) {
		if (FD_ISSET((php_socket_t)fd, &tmp_write)) {
			eventloop_dispatch_callback(cb);
		}
	} ZEND_HASH_FOREACH_END();

	return ret;
}

static eventloop_driver select_driver = {
	"select",
	eventloop_select_init,
	eventloop_select_shutdown,
	eventloop_select_add,
	eventloop_select_remove,
	eventloop_select_poll,
};

eventloop_driver *eventloop_driver_select_get(void)
{
	return &select_driver;
}
