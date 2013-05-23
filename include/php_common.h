#ifndef __PHP_EXTENSION_COMMON_H__
#define __PHP_EXTENSION_COMMON_H__

#include <errno.h>
#include <string.h>
#include <stdint.h>

////////////////////////////////////////////////////////////////////////
// atomic
////////////////////////////////////////////////////////////////////////

#define ARR_SIZE(x) (sizeof(x) / sizeof(x[0]))

#ifdef DEBUG
void print_hex(const void* p, int size, const char* prefix)
{
    const char* pp = (const char*)p;
    zend_printf("%s ", prefix);
    for (int i = 0; i < size; i++)
    {
        zend_printf("%02x ", (uint8_t)pp[i]);
    }
    zend_printf("\n");
}
#else
#define print_hex(x, ...)
#define printf(x, ...)
#define zend_printf(x, ...)
#endif

////////////////////////////////////////////////////////////////////////
// logs
////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define log_printf(format, ...) zend_printf("%s:%d === "format"\n", __FILE__,  __LINE__, ##__VA_ARGS__)
#define log_debug(ext, format, ...) log_printf(format, ##__VA_ARGS__)
#else
#define log_printf(format, ...)
#define log_debug(ext, format, ...)
#endif

#define log_error(ext, format, ...) log_printf(format, ##__VA_ARGS__);php_printf_##ext(E_ERROR, format, ##__VA_ARGS__)

#define log_warn(ext, format, ...) log_printf(format, ##__VA_ARGS__);php_printf_##ext(E_WARNING, format, ##__VA_ARGS__)

#define log_info(ext, format, ...) log_printf(format, ##__VA_ARGS__);php_printf_##ext(E_NOTICE, format, ##__VA_ARGS__)

#define php_printf_common(level, format, ...) php_error(level, "%s:%d === \""format"\"", __FILE__,  __LINE__, ##__VA_ARGS__)

#define php_printf_errno(level, format, ...) php_error(level, "%s:%d === \""format". errno : %d - %s\"", __FILE__,  __LINE__, ##__VA_ARGS__, errno, strerror(errno))


////////////////////////////////////////////////////////////////////////
// atomic
////////////////////////////////////////////////////////////////////////

#if ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) < 40100)
#error can not use builtin atomic operations
#endif

/* get var's value */
#define ATOMIC_GET(var) var

/* set var to value, no return value */
#define ATOMIC_SET(var, value) (void)__sync_lock_test_and_set(&(var), (value))

/* set var to value, and return var's previous value */
#define ATOMIC_SWAP(var, value)  __sync_lock_test_and_set(&(var), (value))

/* if current value of var equal comp, then it will be set to value.
   the return value is always the var's previous value */
#define ATOMIC_CAS(var, comp, value) __sync_val_compare_and_swap(&(var), (comp), (value))

/* set var to 0(or NULL), no return value */
#define ATOMIC_CLEAR(var) (void)__sync_lock_release(&(var))

/* maths/bitop of var by value, and return the var's new value */
#define ATOMIC_ADD(var, value)   __sync_add_and_fetch(&(var), (value))
#define ATOMIC_SUB(var, value)   __sync_sub_and_fetch(&(var), (value))
#define ATOMIC_OR(var, value)    __sync_or_and_fetch(&(var), (value))
#define ATOMIC_AND(var, value)   __sync_and_and_fetch(&(var), (value))
#define ATOMIC_XOR(var, value)   __sync_xor_and_fetch(&(var), (value))

#endif
