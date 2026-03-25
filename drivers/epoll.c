/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php_eventloop.h"

#ifdef HAVE_EPOLL

#include <sys/epoll.h>
#include <unistd.h>

#define EPOLL_MAX_EVENTS 256

static int epoll_fd = -1;
static struct epoll_event *events;

/* Map fd -> callback for readable/writable */
static HashTable fd_read_cbs;
static HashTable fd_write_cbs;

/* Pending fd set — fds that need epoll_ctl sync before wait */
static HashTable dirty_fds;

static uint32_t compute_epoll_events(php_socket_t fd)
{
	uint32_t ev = 0;

	if (zend_hash_index_exists(&fd_read_cbs, (zend_ulong)fd)) {
		ev |= EPOLLIN;
	}
	if (zend_hash_index_exists(&fd_write_cbs, (zend_ulong)fd)) {
		ev |= EPOLLOUT;
	}

	return ev;
}

/* Track which fds are currently registered in the kernel */
static HashTable kernel_fds;

static void epoll_flush_pending(void)
{
	zend_ulong fd;
	struct epoll_event ev;
	uint32_t wanted;
	bool in_kernel;

	ZEND_HASH_FOREACH_NUM_KEY(&dirty_fds, fd) {
		wanted = compute_epoll_events((php_socket_t)fd);
		in_kernel = zend_hash_index_exists(&kernel_fds, fd);

		if (wanted == 0 && in_kernel) {
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, (int)fd, NULL);
			zend_hash_index_del(&kernel_fds, fd);
		} else if (wanted != 0) {
			memset(&ev, 0, sizeof(ev));
			ev.events = wanted;
			ev.data.fd = (int)fd;

			if (in_kernel) {
				epoll_ctl(epoll_fd, EPOLL_CTL_MOD, (int)fd, &ev);
			} else {
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, (int)fd, &ev) == 0) {
					zval tmp;
					ZVAL_TRUE(&tmp);
					zend_hash_index_update(&kernel_fds, fd, &tmp);
				}
			}
		}
	} ZEND_HASH_FOREACH_END();

	zend_hash_clean(&dirty_fds);
}

static int eventloop_epoll_init(void)
{
	epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (UNEXPECTED(epoll_fd < 0)) {
		php_error_docref(NULL, E_WARNING, "Failed to create epoll instance: %s",
			strerror(errno));
		return FAILURE;
	}

	events = ecalloc(EPOLL_MAX_EVENTS, sizeof(struct epoll_event));
	zend_hash_init(&fd_read_cbs, 16, NULL, NULL, 0);
	zend_hash_init(&fd_write_cbs, 16, NULL, NULL, 0);
	zend_hash_init(&dirty_fds, 16, NULL, NULL, 0);
	zend_hash_init(&kernel_fds, 16, NULL, NULL, 0);

	return SUCCESS;
}

static void eventloop_epoll_shutdown(void)
{
	if (epoll_fd >= 0) {
		close(epoll_fd);
		epoll_fd = -1;
	}
	if (events) {
		efree(events);
		events = NULL;
	}
	zend_hash_destroy(&fd_read_cbs);
	zend_hash_destroy(&fd_write_cbs);
	zend_hash_destroy(&dirty_fds);
	zend_hash_destroy(&kernel_fds);
}

static int eventloop_epoll_add(eventloop_callback *cb)
{
	zval tmp;

	if (cb->type == EVENTLOOP_CB_READABLE) {
		zend_hash_index_update_ptr(&fd_read_cbs, (zend_ulong)cb->io.fd, cb);
	} else {
		zend_hash_index_update_ptr(&fd_write_cbs, (zend_ulong)cb->io.fd, cb);
	}

	/* Mark fd as dirty — will be synced in poll() */
	ZVAL_TRUE(&tmp);
	zend_hash_index_update(&dirty_fds, (zend_ulong)cb->io.fd, &tmp);

	return SUCCESS;
}

static void eventloop_epoll_remove(eventloop_callback *cb)
{
	zval tmp;

	if (cb->type == EVENTLOOP_CB_READABLE) {
		zend_hash_index_del(&fd_read_cbs, (zend_ulong)cb->io.fd);
	} else {
		zend_hash_index_del(&fd_write_cbs, (zend_ulong)cb->io.fd);
	}

	/* Mark fd as dirty — will be synced in poll() */
	ZVAL_TRUE(&tmp);
	zend_hash_index_update(&dirty_fds, (zend_ulong)cb->io.fd, &tmp);
}

static int eventloop_epoll_poll(double timeout)
{
	int timeout_ms;
	int ret;
	int i;
	eventloop_callback *cb;

	/* Flush all pending add/remove operations */
	epoll_flush_pending();

	if (timeout < 0) {
		timeout_ms = 1000;
	} else {
		timeout_ms = (int)(timeout * 1000);
	}

	ret = epoll_wait(epoll_fd, events, EPOLL_MAX_EVENTS, timeout_ms);

	if (ret < 0) {
		if (UNEXPECTED(errno != EINTR)) {
			return -1;
		}
		return 0;
	}

	for (i = 0; i < ret; i++) {
		if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR)) {
			cb = zend_hash_index_find_ptr(
				&fd_read_cbs, (zend_ulong)events[i].data.fd);
			if (cb) {
				eventloop_dispatch_callback(cb);
			}
		}
		if (events[i].events & (EPOLLOUT | EPOLLHUP | EPOLLERR)) {
			cb = zend_hash_index_find_ptr(
				&fd_write_cbs, (zend_ulong)events[i].data.fd);
			if (cb) {
				eventloop_dispatch_callback(cb);
			}
		}
	}

	return ret;
}

static eventloop_driver epoll_driver = {
	"epoll",
	eventloop_epoll_init,
	eventloop_epoll_shutdown,
	eventloop_epoll_add,
	eventloop_epoll_remove,
	eventloop_epoll_poll,
};

eventloop_driver *eventloop_driver_epoll_get(void)
{
	return &epoll_driver;
}

#endif /* HAVE_EPOLL */
