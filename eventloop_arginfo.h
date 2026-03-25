/* This is a generated file, edit eventloop.stub.php instead.
 * Stub hash: a5ff8681841e805fb58814f39c67798fbdc49e5c */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_EventLoop_InvalidCallbackError___construct, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, callbackId, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, message, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_InvalidCallbackError_getCallbackId, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_queue, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, closure, Closure, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_defer, 0, 1, IS_STRING, 0)
	ZEND_ARG_OBJ_INFO(0, closure, Closure, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_delay, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, delay, IS_DOUBLE, 0)
	ZEND_ARG_OBJ_INFO(0, closure, Closure, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_repeat, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, interval, IS_DOUBLE, 0)
	ZEND_ARG_OBJ_INFO(0, closure, Closure, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_onReadable, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, stream, IS_MIXED, 0)
	ZEND_ARG_OBJ_INFO(0, closure, Closure, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_EventLoop_EventLoop_onWritable arginfo_class_EventLoop_EventLoop_onReadable

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_onSignal, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, signal, IS_LONG, 0)
	ZEND_ARG_OBJ_INFO(0, closure, Closure, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_enable, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, callbackId, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_EventLoop_EventLoop_disable arginfo_class_EventLoop_EventLoop_enable

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_cancel, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, callbackId, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_EventLoop_EventLoop_reference arginfo_class_EventLoop_EventLoop_enable

#define arginfo_class_EventLoop_EventLoop_unreference arginfo_class_EventLoop_EventLoop_enable

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_isEnabled, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, callbackId, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_EventLoop_EventLoop_isReferenced arginfo_class_EventLoop_EventLoop_isEnabled

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_EventLoop_EventLoop_getType, 0, 1, EventLoop\\CallbackType, 0)
	ZEND_ARG_TYPE_INFO(0, callbackId, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_getIdentifiers, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_run, 0, 0, IS_VOID, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_EventLoop_EventLoop_stop arginfo_class_EventLoop_EventLoop_run

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_isRunning, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_EventLoop_setErrorHandler, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, errorHandler, Closure, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_EventLoop_EventLoop_getErrorHandler, 0, 0, Closure, 1)
ZEND_END_ARG_INFO()

#if defined(HAVE_FIBERS)
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_class_EventLoop_EventLoop_getSuspension, 0, 0, EventLoop\\Suspension, 0)
ZEND_END_ARG_INFO()
#endif

#define arginfo_class_EventLoop_EventLoop_getDriver arginfo_class_EventLoop_InvalidCallbackError_getCallbackId

#if defined(HAVE_FIBERS)
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_Suspension_suspend, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_Suspension_resume, 0, 0, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, value, IS_MIXED, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_EventLoop_Suspension_throw, 0, 1, IS_VOID, 0)
	ZEND_ARG_OBJ_INFO(0, throwable, Throwable, 0)
ZEND_END_ARG_INFO()
#endif

ZEND_METHOD(EventLoop_InvalidCallbackError, __construct);
ZEND_METHOD(EventLoop_InvalidCallbackError, getCallbackId);
ZEND_METHOD(EventLoop_EventLoop, queue);
ZEND_METHOD(EventLoop_EventLoop, defer);
ZEND_METHOD(EventLoop_EventLoop, delay);
ZEND_METHOD(EventLoop_EventLoop, repeat);
ZEND_METHOD(EventLoop_EventLoop, onReadable);
ZEND_METHOD(EventLoop_EventLoop, onWritable);
ZEND_METHOD(EventLoop_EventLoop, onSignal);
ZEND_METHOD(EventLoop_EventLoop, enable);
ZEND_METHOD(EventLoop_EventLoop, disable);
ZEND_METHOD(EventLoop_EventLoop, cancel);
ZEND_METHOD(EventLoop_EventLoop, reference);
ZEND_METHOD(EventLoop_EventLoop, unreference);
ZEND_METHOD(EventLoop_EventLoop, isEnabled);
ZEND_METHOD(EventLoop_EventLoop, isReferenced);
ZEND_METHOD(EventLoop_EventLoop, getType);
ZEND_METHOD(EventLoop_EventLoop, getIdentifiers);
ZEND_METHOD(EventLoop_EventLoop, run);
ZEND_METHOD(EventLoop_EventLoop, stop);
ZEND_METHOD(EventLoop_EventLoop, isRunning);
ZEND_METHOD(EventLoop_EventLoop, setErrorHandler);
ZEND_METHOD(EventLoop_EventLoop, getErrorHandler);
#if defined(HAVE_FIBERS)
ZEND_METHOD(EventLoop_EventLoop, getSuspension);
#endif
ZEND_METHOD(EventLoop_EventLoop, getDriver);
#if defined(HAVE_FIBERS)
ZEND_METHOD(EventLoop_Suspension, suspend);
ZEND_METHOD(EventLoop_Suspension, resume);
ZEND_METHOD(EventLoop_Suspension, throw);
#endif

static const zend_function_entry class_EventLoop_InvalidCallbackError_methods[] = {
	ZEND_ME(EventLoop_InvalidCallbackError, __construct, arginfo_class_EventLoop_InvalidCallbackError___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(EventLoop_InvalidCallbackError, getCallbackId, arginfo_class_EventLoop_InvalidCallbackError_getCallbackId, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static const zend_function_entry class_EventLoop_EventLoop_methods[] = {
	ZEND_ME(EventLoop_EventLoop, queue, arginfo_class_EventLoop_EventLoop_queue, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, defer, arginfo_class_EventLoop_EventLoop_defer, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, delay, arginfo_class_EventLoop_EventLoop_delay, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, repeat, arginfo_class_EventLoop_EventLoop_repeat, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, onReadable, arginfo_class_EventLoop_EventLoop_onReadable, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, onWritable, arginfo_class_EventLoop_EventLoop_onWritable, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, onSignal, arginfo_class_EventLoop_EventLoop_onSignal, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, enable, arginfo_class_EventLoop_EventLoop_enable, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, disable, arginfo_class_EventLoop_EventLoop_disable, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, cancel, arginfo_class_EventLoop_EventLoop_cancel, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, reference, arginfo_class_EventLoop_EventLoop_reference, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, unreference, arginfo_class_EventLoop_EventLoop_unreference, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, isEnabled, arginfo_class_EventLoop_EventLoop_isEnabled, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, isReferenced, arginfo_class_EventLoop_EventLoop_isReferenced, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, getType, arginfo_class_EventLoop_EventLoop_getType, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, getIdentifiers, arginfo_class_EventLoop_EventLoop_getIdentifiers, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, run, arginfo_class_EventLoop_EventLoop_run, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, stop, arginfo_class_EventLoop_EventLoop_stop, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, isRunning, arginfo_class_EventLoop_EventLoop_isRunning, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, setErrorHandler, arginfo_class_EventLoop_EventLoop_setErrorHandler, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_ME(EventLoop_EventLoop, getErrorHandler, arginfo_class_EventLoop_EventLoop_getErrorHandler, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
#if defined(HAVE_FIBERS)
	ZEND_ME(EventLoop_EventLoop, getSuspension, arginfo_class_EventLoop_EventLoop_getSuspension, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
#endif
	ZEND_ME(EventLoop_EventLoop, getDriver, arginfo_class_EventLoop_EventLoop_getDriver, ZEND_ACC_PUBLIC|ZEND_ACC_STATIC)
	ZEND_FE_END
};

#if defined(HAVE_FIBERS)
static const zend_function_entry class_EventLoop_Suspension_methods[] = {
	ZEND_ME(EventLoop_Suspension, suspend, arginfo_class_EventLoop_Suspension_suspend, ZEND_ACC_PUBLIC)
	ZEND_ME(EventLoop_Suspension, resume, arginfo_class_EventLoop_Suspension_resume, ZEND_ACC_PUBLIC)
	ZEND_ME(EventLoop_Suspension, throw, arginfo_class_EventLoop_Suspension_throw, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};
#endif

static zend_class_entry *register_class_EventLoop_CallbackType(void)
{
	zend_class_entry *class_entry = zend_register_internal_enum("EventLoop\\CallbackType", IS_UNDEF, NULL);

	zend_enum_add_case_cstr(class_entry, "Defer", NULL);

	zend_enum_add_case_cstr(class_entry, "Delay", NULL);

	zend_enum_add_case_cstr(class_entry, "Repeat", NULL);

	zend_enum_add_case_cstr(class_entry, "Readable", NULL);

	zend_enum_add_case_cstr(class_entry, "Writable", NULL);

	zend_enum_add_case_cstr(class_entry, "Signal", NULL);

	return class_entry;
}

static zend_class_entry *register_class_EventLoop_InvalidCallbackError(zend_class_entry *class_entry_Error)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "EventLoop", "InvalidCallbackError", class_EventLoop_InvalidCallbackError_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, class_entry_Error, ZEND_ACC_FINAL);

	zval property_callbackId_default_value;
	ZVAL_UNDEF(&property_callbackId_default_value);
	zend_string *property_callbackId_name = zend_string_init("callbackId", sizeof("callbackId") - 1, true);
	zend_declare_typed_property(class_entry, property_callbackId_name, &property_callbackId_default_value, ZEND_ACC_PUBLIC|ZEND_ACC_READONLY, NULL, (zend_type) ZEND_TYPE_INIT_MASK(MAY_BE_STRING));
	zend_string_release_ex(property_callbackId_name, true);

	return class_entry;
}

static zend_class_entry *register_class_EventLoop_EventLoop(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "EventLoop", "EventLoop", class_EventLoop_EventLoop_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, ZEND_ACC_FINAL);

	return class_entry;
}

#if defined(HAVE_FIBERS)
static zend_class_entry *register_class_EventLoop_Suspension(void)
{
	zend_class_entry ce, *class_entry;

	INIT_NS_CLASS_ENTRY(ce, "EventLoop", "Suspension", class_EventLoop_Suspension_methods);
	class_entry = zend_register_internal_class_with_flags(&ce, NULL, ZEND_ACC_FINAL);

	return class_entry;
}
#endif
