/*
 * Copyright (c) 2026 Aleksandr Cherednikov
 * Licensed under the MIT License. See LICENSE for details.
 */

#ifndef PHP_EVENTLOOP_COMPAT_H
#define PHP_EVENTLOOP_COMPAT_H

#include "php.h"
#include "zend_API.h"

/*
 * Compatibility shims for building across PHP 8.1 – 8.5+.
 *
 * The extension targets PHP >= 8.1 (Fibers). Internal Zend APIs evolved
 * between releases, so we polyfill the newer signatures here and let
 * each call-site use a single, modern spelling.
 */

/* ------------------------------------------------------------------ */
/* zend_register_internal_class_with_flags()  — added in PHP 8.4      */
/* In 8.1-8.3 we register + set flags manually.                       */
/* ------------------------------------------------------------------ */
#if PHP_VERSION_ID < 80400
static inline zend_class_entry *zend_register_internal_class_with_flags(
	zend_class_entry *ce, zend_class_entry *parent, uint32_t flags)
{
	zend_class_entry *registered = zend_register_internal_class_ex(ce, parent);
	registered->ce_flags |= flags;
	return registered;
}
#endif

/* ------------------------------------------------------------------ */
/* ce->default_object_handlers  — added in PHP 8.3                    */
/* ------------------------------------------------------------------ */
#if PHP_VERSION_ID >= 80300
# define EVENTLOOP_SET_DEFAULT_HANDLERS(ce, h) \
	(ce)->default_object_handlers = (h)
#else
# define EVENTLOOP_SET_DEFAULT_HANDLERS(ce, h) \
	/* not available before 8.3 — handlers set in create_object */
#endif

/* ------------------------------------------------------------------ */
/* Fiber suspend / resume internal API                                 */
/*                                                                     */
/* PHP 8.4+ exposes zend_fiber_suspend() / zend_fiber_resume() with a  */
/* three-arg signature.  Earlier versions do not export these symbols,  */
/* so we call the userland Fiber::suspend() / $fiber->resume() via     */
/* zend_call_method, which performs the same context switch.            */
/* ------------------------------------------------------------------ */
#include "zend_fibers.h"
#include "zend_interfaces.h"

#if PHP_VERSION_ID < 80400

static inline void eventloop_compat_fiber_suspend(
	zend_fiber *fiber, zval *value, zval *return_value)
{
	(void)fiber; /* Fiber::suspend() operates on the active fiber */
	if (value && Z_TYPE_P(value) != IS_NULL) {
		zend_call_method_with_1_params(NULL, zend_ce_fiber, NULL, "suspend", return_value, value);
	} else {
		zend_call_method_with_0_params(NULL, zend_ce_fiber, NULL, "suspend", return_value);
	}
}

static inline void eventloop_compat_fiber_resume(
	zend_fiber *fiber, zval *value, zval *error)
{
	zval fiber_zv, retval;
	(void)error;
	ZVAL_OBJ(&fiber_zv, &fiber->std);
	if (value && Z_TYPE_P(value) != IS_NULL) {
		zend_call_method_with_1_params(&fiber_zv, zend_ce_fiber, NULL, "resume", &retval, value);
	} else {
		zend_call_method_with_0_params(&fiber_zv, zend_ce_fiber, NULL, "resume", &retval);
	}
	zval_ptr_dtor(&retval);
}

# define zend_fiber_suspend(f, v, r) eventloop_compat_fiber_suspend((f), (v), (r))
# define zend_fiber_resume(f, v, e)  eventloop_compat_fiber_resume((f), (v), (e))

#endif /* PHP_VERSION_ID < 80400 */

#endif /* PHP_EVENTLOOP_COMPAT_H */
