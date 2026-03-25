/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "php_eventloop.h"
#include "ext/standard/php_string.h"

static zend_string *eventloop_generate_id(void)
{
	char buf[24];

	int len = snprintf(buf, sizeof(buf), "eL%" PRIx64, EVENTLOOP_G(next_id)++);

	return zend_string_init(buf, len, 0);
}

eventloop_callback *eventloop_cb_create(eventloop_cb_type type, zval *closure)
{
	eventloop_callback *cb;

	cb = ecalloc(1, sizeof(eventloop_callback));
	cb->id = eventloop_generate_id();
	cb->type = type;
	cb->flags = EVENTLOOP_CB_FLAG_ENABLED | EVENTLOOP_CB_FLAG_REFERENCED;
	ZVAL_COPY(&cb->closure, closure);
	cb->heap_index = UINT32_MAX;

	zend_hash_add_ptr(&EVENTLOOP_G(callbacks), cb->id, cb);

	return cb;
}

void eventloop_cb_free(eventloop_callback *cb)
{
	ZEND_ASSERT(cb != NULL);

	if (cb->type == EVENTLOOP_CB_READABLE || cb->type == EVENTLOOP_CB_WRITABLE) {
		zval_ptr_dtor(&cb->io.stream);
	}
	zval_ptr_dtor(&cb->closure);
	zend_string_release(cb->id);
	efree(cb);
}

eventloop_callback *eventloop_cb_find(const zend_string *id)
{
	return zend_hash_find_ptr(&EVENTLOOP_G(callbacks), (zend_string *)id);
}

void eventloop_cb_enable(eventloop_callback *cb)
{
	ZEND_ASSERT(cb != NULL);

	if (cb->flags & EVENTLOOP_CB_FLAG_CANCELLED) {
		return;
	}

	if (UNEXPECTED(cb->flags & EVENTLOOP_CB_FLAG_ENABLED)) {
		return;
	}

	cb->flags |= EVENTLOOP_CB_FLAG_ENABLED;

	switch (cb->type) {
		case EVENTLOOP_CB_READABLE:
		case EVENTLOOP_CB_WRITABLE:
			if (EXPECTED(EVENTLOOP_G(driver))) {
				EVENTLOOP_G(driver)->add(cb);
			}
			break;
		case EVENTLOOP_CB_DELAY:
			cb->delay.expiry = eventloop_now() + cb->delay.delay;
			eventloop_timer_heap_push(&EVENTLOOP_G(timer_heap), cb);
			break;
		case EVENTLOOP_CB_REPEAT:
			cb->repeat.expiry = eventloop_now() + cb->repeat.interval;
			eventloop_timer_heap_push(&EVENTLOOP_G(timer_heap), cb);
			break;
		default:
			break;
	}
}

void eventloop_cb_disable(eventloop_callback *cb)
{
	ZEND_ASSERT(cb != NULL);

	if (UNEXPECTED(!(cb->flags & EVENTLOOP_CB_FLAG_ENABLED))) {
		return;
	}

	cb->flags &= ~EVENTLOOP_CB_FLAG_ENABLED;

	switch (cb->type) {
		case EVENTLOOP_CB_READABLE:
		case EVENTLOOP_CB_WRITABLE:
			if (EXPECTED(EVENTLOOP_G(driver))) {
				EVENTLOOP_G(driver)->remove(cb);
			}
			break;
		case EVENTLOOP_CB_DELAY:
		case EVENTLOOP_CB_REPEAT:
			if (cb->heap_index != UINT32_MAX) {
				eventloop_timer_heap_remove(&EVENTLOOP_G(timer_heap), cb);
			}
			break;
		default:
			break;
	}
}

void eventloop_cb_cancel(eventloop_callback *cb)
{
	ZEND_ASSERT(cb != NULL);

	if (UNEXPECTED(cb->flags & EVENTLOOP_CB_FLAG_CANCELLED)) {
		return;
	}

	eventloop_cb_disable(cb);
	cb->flags |= EVENTLOOP_CB_FLAG_CANCELLED;
	cb->flags &= ~EVENTLOOP_CB_FLAG_ENABLED;

	zend_hash_del(&EVENTLOOP_G(callbacks), cb->id);
}

bool eventloop_has_referenced_callbacks(void)
{
	eventloop_callback *cb;

	ZEND_HASH_FOREACH_PTR(&EVENTLOOP_G(callbacks), cb) {
		if ((cb->flags & EVENTLOOP_CB_FLAG_REFERENCED) &&
		    (cb->flags & EVENTLOOP_CB_FLAG_ENABLED) &&
		    !(cb->flags & EVENTLOOP_CB_FLAG_CANCELLED)) {
			return true;
		}
	} ZEND_HASH_FOREACH_END();

	return false;
}

uint32_t eventloop_referenced_count(void)
{
	uint32_t count = 0;
	eventloop_callback *cb;

	ZEND_HASH_FOREACH_PTR(&EVENTLOOP_G(callbacks), cb) {
		if ((cb->flags & EVENTLOOP_CB_FLAG_REFERENCED) &&
		    (cb->flags & EVENTLOOP_CB_FLAG_ENABLED) &&
		    !(cb->flags & EVENTLOOP_CB_FLAG_CANCELLED)) {
			count++;
		}
	} ZEND_HASH_FOREACH_END();

	return count;
}
