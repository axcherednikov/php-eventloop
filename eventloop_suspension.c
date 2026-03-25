/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php_eventloop.h"

#ifdef HAVE_FIBERS
#include "zend_exceptions.h"
#include "zend_enum.h"
#include "php_eventloop_compat.h"
#include "eventloop_arginfo.h"

typedef struct _eventloop_suspension_obj {
	zval fiber_zv;         /* Fiber object zval (to call methods on it) */
	bool pending;
	bool suspended;
	zval resume_value;
	zval resume_exception; /* Throwable to throw into fiber */
	zend_object std;
} eventloop_suspension_obj;

static zend_object_handlers suspension_object_handlers;

static inline eventloop_suspension_obj *suspension_from_obj(zend_object *obj)
{
	return (eventloop_suspension_obj *)((char *)obj - XtOffsetOf(eventloop_suspension_obj, std));
}

#define Z_SUSPENSION_P(zv) suspension_from_obj(Z_OBJ_P(zv))

static zend_object *suspension_create_object(zend_class_entry *ce)
{
	eventloop_suspension_obj *suspension;

	suspension = zend_object_alloc(sizeof(eventloop_suspension_obj), ce);

	zend_object_std_init(&suspension->std, ce);
	object_properties_init(&suspension->std, ce);

	suspension->std.handlers = &suspension_object_handlers;

	ZVAL_NULL(&suspension->fiber_zv);
	suspension->pending = false;
	suspension->suspended = false;
	ZVAL_NULL(&suspension->resume_value);
	ZVAL_NULL(&suspension->resume_exception);

	return &suspension->std;
}

static void suspension_free_object(zend_object *obj)
{
	eventloop_suspension_obj *suspension;

	suspension = suspension_from_obj(obj);

	zval_ptr_dtor(&suspension->fiber_zv);
	zval_ptr_dtor(&suspension->resume_value);
	zval_ptr_dtor(&suspension->resume_exception);

	zend_object_std_dtor(&suspension->std);
}

static zend_function *suspension_get_constructor(zend_object *obj)
{
	zend_throw_error(NULL,
		"Cannot directly construct EventLoop\\Suspension");
	return NULL;
}

/* EventLoop\Suspension::suspend(): mixed */
ZEND_METHOD(EventLoop_Suspension, suspend)
{
	eventloop_suspension_obj *suspension;
	zend_fiber *fiber;
	zval exception;

	ZEND_PARSE_PARAMETERS_NONE();

	suspension = Z_SUSPENSION_P(ZEND_THIS);

	if (UNEXPECTED(suspension->suspended)) {
		zend_throw_error(NULL,
			"EventLoop\\Suspension::suspend() cannot be called on an already suspended Suspension");
		RETURN_THROWS();
	}

	fiber = EG(active_fiber);
	if (UNEXPECTED(!fiber)) {
		zend_throw_error(NULL,
			"EventLoop\\Suspension::suspend() must be called from within a fiber");
		RETURN_THROWS();
	}

	/* Store reference to the fiber object */
	ZVAL_OBJ_COPY(&suspension->fiber_zv, &fiber->std);
	suspension->suspended = true;
	suspension->pending = false;

	/* Suspend the current fiber -- control returns to the event loop.
	 * The value returned by zend_fiber_suspend becomes the return of
	 * Fiber::resume() in the caller's context. We pass NULL since the
	 * caller (the event loop) doesn't need a value from suspend. */
	zend_fiber_suspend(fiber, NULL, return_value);

	/* When we get here, the fiber has been resumed */
	suspension->suspended = false;
	zval_ptr_dtor(&suspension->fiber_zv);
	ZVAL_NULL(&suspension->fiber_zv);

	/* Check if we should throw an exception */
	if (Z_TYPE(suspension->resume_exception) != IS_NULL) {
		ZVAL_COPY_VALUE(&exception, &suspension->resume_exception);
		ZVAL_NULL(&suspension->resume_exception);
		zend_throw_exception_object(&exception);
		RETURN_THROWS();
	}

	/* Return the resume value */
	if (Z_TYPE(suspension->resume_value) != IS_NULL) {
		ZVAL_COPY(return_value, &suspension->resume_value);
		zval_ptr_dtor(&suspension->resume_value);
		ZVAL_NULL(&suspension->resume_value);
	}
}

/* EventLoop\Suspension::resume(mixed $value = null): void */
ZEND_METHOD(EventLoop_Suspension, resume)
{
	zval *value = NULL;
	eventloop_suspension_obj *suspension;
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_ZVAL(value)
	ZEND_PARSE_PARAMETERS_END();

	suspension = Z_SUSPENSION_P(ZEND_THIS);

	if (UNEXPECTED(!suspension->suspended)) {
		zend_throw_error(NULL,
			"EventLoop\\Suspension::resume() can only be called on a suspended Suspension");
		RETURN_THROWS();
	}

	if (UNEXPECTED(suspension->pending)) {
		zend_throw_error(NULL,
			"EventLoop\\Suspension::resume() has already been called");
		RETURN_THROWS();
	}

	suspension->pending = true;

	/* Store the value to be returned by suspend() */
	if (value) {
		ZVAL_COPY(&suspension->resume_value, value);
	}

	/* Resume the fiber via the public API.
	 * zend_fiber_resume passes the value which becomes the return
	 * of Fiber::suspend() inside the fiber. */
	ZEND_ASSERT(Z_TYPE(suspension->fiber_zv) == IS_OBJECT);
	fiber = (zend_fiber *)Z_OBJ(suspension->fiber_zv);
	zend_fiber_resume(fiber, value, NULL);
}

/* EventLoop\Suspension::throw(\Throwable $throwable): void */
ZEND_METHOD(EventLoop_Suspension, throw)
{
	zval *throwable;
	eventloop_suspension_obj *suspension;
	zend_fiber *fiber;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_OBJECT_OF_CLASS(throwable, zend_ce_throwable)
	ZEND_PARSE_PARAMETERS_END();

	suspension = Z_SUSPENSION_P(ZEND_THIS);

	if (UNEXPECTED(!suspension->suspended)) {
		zend_throw_error(NULL,
			"EventLoop\\Suspension::throw() can only be called on a suspended Suspension");
		RETURN_THROWS();
	}

	if (UNEXPECTED(suspension->pending)) {
		zend_throw_error(NULL,
			"A resume or throw operation is already pending");
		RETURN_THROWS();
	}

	suspension->pending = true;

	/* Store the exception. Resume the fiber with NULL value.
	 * When suspend() returns, it will check resume_exception and throw it. */
	ZVAL_COPY(&suspension->resume_exception, throwable);

	ZEND_ASSERT(Z_TYPE(suspension->fiber_zv) == IS_OBJECT);
	fiber = (zend_fiber *)Z_OBJ(suspension->fiber_zv);
	zend_fiber_resume(fiber, NULL, NULL);
}

void eventloop_suspension_init(void)
{
	eventloop_suspension_ce = register_class_EventLoop_Suspension();
	eventloop_suspension_ce->create_object = suspension_create_object;
	EVENTLOOP_SET_DEFAULT_HANDLERS(eventloop_suspension_ce, &suspension_object_handlers);

	memcpy(&suspension_object_handlers, zend_get_std_object_handlers(),
		sizeof(zend_object_handlers));
	suspension_object_handlers.offset = XtOffsetOf(eventloop_suspension_obj, std);
	suspension_object_handlers.free_obj = suspension_free_object;
	suspension_object_handlers.get_constructor = suspension_get_constructor;
	suspension_object_handlers.clone_obj = NULL;
}

void eventloop_suspension_shutdown(void)
{
}

#endif /* HAVE_FIBERS */
