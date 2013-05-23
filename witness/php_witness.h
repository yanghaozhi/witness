/*
 * *  Copyright (c) 2013 UCWeb Inc.
 * *
 * *  Licensed under the Apache License, Version 2.0 (the "License");
 * *  you may not use this file except in compliance with the License.
 * *  You may obtain a copy of the License at
 * *
 * *      http://www.apache.org/licenses/LICENSE-2.0
 * *
 * *  witness module
 * *
 * *  Author: <oswitness@ucweb.com>
 * *
 * */


#ifndef PHP_WITNESS_H
#define PHP_WITNESS_H

extern zend_module_entry witness_module_entry;
#define phpext_witness_ptr &witness_module_entry

#ifdef PHP_WIN32
#	define PHP_WITNESS_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_WITNESS_API __attribute__ ((visibility("default")))
#else
#	define PHP_WITNESS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(witness);
PHP_MSHUTDOWN_FUNCTION(witness);
PHP_RINIT_FUNCTION(witness);
PHP_RSHUTDOWN_FUNCTION(witness);
PHP_MINFO_FUNCTION(witness);

PHP_FUNCTION(witness_set_token);
PHP_FUNCTION(witness_start);
PHP_FUNCTION(witness_stop);
PHP_FUNCTION(witness_assert);
PHP_FUNCTION(witness_dump);
PHP_FUNCTION(witness_stack_info);

PHP_FUNCTION(confirm_witness_compiled);	/* For testing, remove later. */

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     
*/

ZEND_BEGIN_MODULE_GLOBALS(witness)
	long		max_stack;
	char*		temp_path;
	char*		post_url;
	zend_bool	record_internal;
	char*		auto_start_token;
	zend_bool	enable_dump;
	zend_bool	enable_assert;
	zend_bool	keep_record;
ZEND_END_MODULE_GLOBALS(witness)

/* In every utility function you add that needs to use variables 
   in php_witness_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as WITNESS_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define WITNESS_G(v) TSRMG(witness_globals_id, zend_witness_globals *, v)
#else
#define WITNESS_G(v) (witness_globals.v)
#endif

#endif	/* PHP_WITNESS_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
