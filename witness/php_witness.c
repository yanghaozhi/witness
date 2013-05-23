/*
 *  Copyright (c) 2013 UCWeb Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  witness module
 *
 *  Author: <oswitness@ucweb.com>
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/head.h"
#include "ext/standard/php_var.h"
#include "Zend/zend_compile.h"
#include "php_witness.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <time.h>
#include <poll.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

//#include <bzlib.h>
#include <sqlite3.h>

#include "version.h"
#include "php_common.h"

#define WITNESS_URL_TOKEN 		"$token$"

#define WITNESS_SERIALIZE_LEN	(16 * 1024)

#define WITNESS_TYPE_CALL		0
#define WITNESS_TYPE_RET		1
#define WITNESS_TYPE_VAR		2

#define WITNESS_CREATE_TABLE	\
	"CREATE TABLE funcs(func INT UNIQUE, scope TEXT, name TEXT, file TEXT, argc INT, required INT, line_start INT, line_end INT, PRIMARY KEY (scope, name));" \
	"CREATE TABLE args(func INT, seq INT, class TEXT, name TEXT, PRIMARY KEY (func, seq));" \
	"CREATE TABLE calls(call INT UNIQUE, prev INT, func INT, params INT, retval TEXT, line INT, time INT);" \
	"CREATE TABLE argvs(call INT, seq INT, value TEXT);"		\
	"CREATE TABLE globals(name TEXT UNIQUE, value TEXT);"		\
	"CREATE TABLE locals(call INT, name TEXT, value TEXT);"

#define WITNESS_DUMP_TABLE										\
	"begin transaction;" 										\
	"create table dump_db.funcs as select * from funcs;" 		\
	"create table dump_db.args as select * from args;"			\
	"create table dump_db.calls as select * from calls;" 		\
	"create table dump_db.argvs as select * from argvs;" 		\
	"create table dump_db.globals as select * from globals;"	\
	"create table dump_db.locals as select * from locals;"		\
	"commit transaction;"

#define WITNESS_FUNCS_FUNC		1
#define WITNESS_FUNCS_SCOPE		2
#define WITNESS_FUNCS_NAME		3
#define WITNESS_FUNCS_FILE		4
#define WITNESS_FUNCS_ARGC		5
#define WITNESS_FUNCS_REQUIRED	6
#define WITNESS_FUNCS_START		7
#define WITNESS_FUNCS_END		8
#define WITNESS_INSERT_FUNCS	"INSERT OR IGNORE INTO funcs VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8);"
#define WITNESS_STMT_FUNCS_ID	0

#define WITNESS_ARGS_SEQ		1
#define WITNESS_ARGS_CLASS		2
#define WITNESS_ARGS_NAME		3
#define WITNESS_ARGS_SCOPE		4
#define WITNESS_ARGS_FUNC		5
#define WITNESS_INSERT_ARGS		"INSERT OR IGNORE INTO args SELECT func, ?1, ?2, ?3 FROM funcs WHERE scope = ?4 AND name = ?5 LIMIT 1;"
#define WITNESS_STMT_ARGS_ID	5

#define WITNESS_CALLS_CALL		1
#define WITNESS_CALLS_PREV		2
#define WITNESS_CALLS_PARAMS	3
#define WITNESS_CALLS_RETVAL	4
#define WITNESS_CALLS_LINE		5
#define WITNESS_CALLS_SCOPE		6
#define WITNESS_CALLS_FUNC		7
#define WITNESS_INSERT_CALLS	"INSERT INTO calls SELECT ?1, ?2, func, ?3, ?4, ?5, '' FROM funcs WHERE scope = ?6 AND name = ?7 LIMIT 1;"
#define WITNESS_STMT_CALLS_ID	1

#define WITNESS_ARGVS_CALL		1
#define WITNESS_ARGVS_SEQ		2
#define WITNESS_ARGVS_VALUE		3
#define WITNESS_INSERT_ARGVS	"INSERT INTO argvs VALUES (?1, ?2, ?3);"
#define WITNESS_STMT_ARGVS_ID	2

#define WITNESS_GLOBALS_NAME	1
#define WITNESS_GLOBALS_VALUE	2
#define WITNESS_INSERT_GLOBALS	"INSERT INTO globals VALUES (?1, ?2);"
#define WITNESS_STMT_GLOBALS_ID	3

#define WITNESS_RETVAL_VAL		1
#define WITNESS_TIME_VAL		2
#define WITNESS_RETVAL_ID		3
#define WITNESS_UPDATE_RETVAL	"UPDATE calls SET retval = ?1, time = ?2 WHERE call = ?3;"
#define WITNESS_STMT_RETVAL_ID	4

#define WITNESS_LOCALS_CALL		1
#define WITNESS_LOCALS_NAME		2
#define WITNESS_LOCALS_VALUE	3
#define WITNESS_INSERT_LOCALS	"INSERT INTO locals VALUES (?1, ?2, ?3);"
#define WITNESS_STMT_LOCALS_ID	6

/* If you declare any globals in php_witness.h uncomment this:
   */
   ZEND_DECLARE_MODULE_GLOBALS(witness)

/* True global resources - no need for thread safety here */
static int le_witness;

struct zend_env
{
	void (*zend_execute)(zend_op_array* ops TSRMLS_DC);
	void (*zend_execute_internal)(zend_execute_data* execute_data, int ret TSRMLS_DC);
};

static int	 g_record    = -1;
static char* g_token     = NULL;
static int   g_token_len = -1;

static struct zend_env g_run_backup = {NULL, NULL};
static struct zend_env g_env_backup = {NULL, NULL};

static void witness_clear(void);
static bool witness_start_trace(const char* token, int len, bool stack);

/* {{{ witness_functions[]
 *
 * Every user visible function must have an entry in witness_functions[].
 */
ZEND_BEGIN_ARG_INFO(witness_arg, 0)
	ZEND_ARG_INFO(0, token)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(witness_assert_arg, 0)
	ZEND_ARG_INFO(0, expression)
ZEND_END_ARG_INFO()

const zend_function_entry witness_functions[] = {
	PHP_FE(witness_set_token, witness_arg)
	PHP_FE(witness_start, witness_arg)
	PHP_FE(witness_stop, NULL)
	PHP_FE(witness_assert, witness_assert_arg)
	PHP_FE(witness_dump, witness_arg)
	PHP_FE(witness_stack_info, NULL)
	PHP_FE(confirm_witness_compiled,	NULL)		/* For testing, remove later. */
	{NULL, NULL, NULL}	/* Must be the last line in witness_functions[] */
};
/* }}} */

/* {{{ witness_module_entry
*/
zend_module_entry witness_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"witness",
	witness_functions,
	PHP_MINIT(witness),
	PHP_MSHUTDOWN(witness),
	PHP_RINIT(witness),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(witness),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(witness),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_WITNESS
ZEND_GET_MODULE(witness)
#endif

	/* {{{ PHP_INI
	*/
	/* Remove comments and fill if you need to have entries in php.ini
	   */
	   PHP_INI_BEGIN()
	   STD_PHP_INI_ENTRY("witness.max_stack", "16384", PHP_INI_ALL, OnUpdateLong, max_stack, zend_witness_globals, witness_globals)
	   STD_PHP_INI_ENTRY("witness.temp_path", "/dev/shm/", PHP_INI_ALL, OnUpdateString, temp_path, zend_witness_globals, witness_globals)
	   STD_PHP_INI_ENTRY("witness.post_url", "", PHP_INI_ALL, OnUpdateString, post_url, zend_witness_globals, witness_globals)
	   STD_PHP_INI_ENTRY("witness.record_internal", "false", PHP_INI_ALL, OnUpdateBool, record_internal, zend_witness_globals, witness_globals)
	   STD_PHP_INI_ENTRY("witness.auto_start_token", "", PHP_INI_ALL, OnUpdateString, auto_start_token, zend_witness_globals, witness_globals)
	   STD_PHP_INI_ENTRY("witness.enable_dump", "false", PHP_INI_ALL, OnUpdateBool, enable_dump, zend_witness_globals, witness_globals)
	   STD_PHP_INI_ENTRY("witness.enable_assert", "false", PHP_INI_ALL, OnUpdateBool, enable_assert, zend_witness_globals, witness_globals)
	   STD_PHP_INI_ENTRY("witness.keep_record", "false", PHP_INI_ALL, OnUpdateBool, keep_record, zend_witness_globals, witness_globals)
	   PHP_INI_END()
	/* }}} */

	/* {{{ php_witness_init_globals
	*/
	/* Uncomment this function if you have INI entries
	   static void php_witness_init_globals(zend_witness_globals *witness_globals)
	   {
	   witness_globals->global_value = 0;
	   witness_globals->global_string = NULL;
	   }
	   */
	/* }}} */

	/* {{{ PHP_MINIT_FUNCTION
	*/
PHP_MINIT_FUNCTION(witness)
{
	/* If you have INI entries, uncomment these lines 
	   */
	   REGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
*/
PHP_MSHUTDOWN_FUNCTION(witness)
{
	/* uncomment this line if you have INI entries
	   */
	   UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
*/
PHP_RINIT_FUNCTION(witness)
{
	g_env_backup.zend_execute = zend_execute;
	g_env_backup.zend_execute_internal = zend_execute_internal;

	if (WITNESS_G(auto_start_token)[0] != '\0')
	{
		char* name = "_COOKIE";
		zval** result = NULL;
		if (zend_hash_find(&EG(symbol_table), name, strlen(name) + 1, (void**)&result) == SUCCESS)
		{
			name = WITNESS_G(auto_start_token);
			int name_len = strlen(name);
			if (zend_hash_find(Z_ARRVAL_PP(result), name, name_len + 1, (void**)&result) == SUCCESS)
			{
                int buf_len = Z_STRLEN_PP(result);
				char buf[buf_len + 1];
				int64_t id = -1;
				int ttl = -1;

				memcpy(buf, Z_STRVAL_PP(result), buf_len);
				buf[sizeof(buf) - 1] = '\0';

				char* p = strchr(buf, ':');
				if (p != NULL)
				{
					*p = '\0';
					ttl = atoi(buf);
					if (--ttl > 0)
					{
						witness_start_trace(p + 1, strlen(p + 1), false);

						snprintf(buf, sizeof(buf), "%d:%s", ttl, p + 1);
						buf[sizeof(buf) - 1] = '\0';
                        buf_len = strlen(buf);
					}

					if (php_setcookie(name, name_len, buf, buf_len, (ttl > 0) ? time(NULL) + 600 : time(NULL) - 1000, NULL, 0, NULL, 0, 0, 1, 0) < 0)
					{
						log_error(errno, "set cookie error : %s - %s", name, buf);
					}
				}
			}
		}
	}
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
*/
PHP_RSHUTDOWN_FUNCTION(witness)
{
	witness_clear();

	zend_execute = g_env_backup.zend_execute;
	zend_execute_internal = g_env_backup.zend_execute_internal;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
*/
PHP_MINFO_FUNCTION(witness)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "witness support", "enabled");
	php_info_print_table_header(2, "author", "yanghz");
	php_info_print_table_header(2, "hg version", HG_VERSION);
	php_info_print_table_header(2, "build info", __DATE__ __TIME__);
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	   */
	   DISPLAY_INI_ENTRIES();
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_witness_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_witness_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "witness", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
   */

#define witness_is_recording(f) ((f == -1) ? false : true)
#define witness_reset_recording(f) close(f);f = -1;

#define witness_iov_set_start(x) struct iovec iovs[1024];uint32_t iovs_num[1024];int iovs_count=0;
#define witness_iov_set_reset(x) iovs_num[0]=0;iovs_count=0;
#define witness_iov_set_str(x) iovs[iovs_count].iov_base=(void*)(x);iovs[iovs_count].iov_len=strlen(x)+1;iovs_num[0]+=iovs[iovs_count].iov_len;++iovs_count;
#define witness_iov_set_int(x) iovs_num[iovs_count]=(uint32_t)x;iovs[iovs_count].iov_base=&iovs_num[iovs_count];iovs[iovs_count].iov_len=sizeof(iovs_num[0]);iovs_num[0]+=iovs[iovs_count].iov_len;++iovs_count;
#define witness_iov_set_end(x) int send_size=-1;do{send_size=writev(x,iovs,iovs_count);}while((send_size<0)&&((errno==EINTR)||(errno==EAGAIN)));
 
#define witness_iov_get_start(x) const char* _p = x;
#define witness_iov_get_str(x, y, z) const char* x = _p; int x##_len = strlen(_p); sqlite3_bind_text(stmts[y], z, _p, x##_len, NULL); _p += (x##_len + 1);
#define witness_iov_get_int(x, y, z) uint32_t x = *(uint32_t*)_p;sqlite3_bind_int(stmts[y], z, x); _p += sizeof(uint32_t);
#define witness_iov_get_end(x) sqlite3_step(stmts[x]);sqlite3_reset(stmts[x]);
#define witness_iov_get_commit(x) sqlite3_step(stmts[x]);sqlite3_reset(stmts[x]);
#define witness_iov_get_reset(x) sqlite3_reset(stmts[x]);

#define witness_serialize(ret, val, str_buf, obj_buf, index)                    \
	if (val != NULL)                                                            \
	{                                                                           \
        if (Z_TYPE_PP((val)) != IS_OBJECT)                                      \
        {                                                                       \
            php_serialize_data_t var_hash;                                      \
            PHP_VAR_SERIALIZE_INIT(var_hash);                                   \
            php_var_serialize(&(str_buf)[index], (val), &var_hash TSRMLS_CC);   \
            PHP_VAR_SERIALIZE_DESTROY(var_hash);                                \
            ret = (str_buf)[index].c;                                           \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            zend_class_entry *ce = Z_OBJCE_PP((val));                           \
            if (ce != NULL)                                                     \
            {                                                                   \
                snprintf((obj_buf)[index], sizeof((obj_buf)[index]), "Object:%s::%s", (ce->type == ZEND_USER_CLASS) ? ce->filename : "", ce->name);    \
                ((obj_buf)[index])[sizeof((obj_buf)[index]) - 1] = '\0';        \
                ret = (obj_buf)[index];                                         \
            }                                                                   \
        }                                                                       \
	}

#ifdef EX
#undef EX
#define EX(element) ((execute_data)->element)
#endif

#define EX_T(offset) (*(temp_variable *)((char *) EX(Ts) + offset))

#define RETURN_VALUE_USED(opline) (!((opline)->result.u.EA.type & EXT_TYPE_UNUSED))

static bool witness_post(char* url, int len, const char* file)
{
	int fd = open(file, O_RDONLY);
	if (fd == -1)
	{
		log_error(errno, "can not open db file : %s", file);
		return false;
	}

	char size[32];
	struct stat st;
	if (fstat(fd, &st) != 0)
	{
		log_error(errno, "can not get db file state");
		return false;
	}
	snprintf(size, sizeof(size), "%llu", (unsigned long long)st.st_size);

	const char* header = "http://";

	char* host = url + ((strncasecmp(url, header, strlen(header)) == 0) ? strlen(header) : 0);
	char* port = "80";
	char* path = "";

	int t = strcspn(host, ":/");
	switch (host[t])
	{
	case ':':
		{
			host[t] = '\0';
			port = host + t + 1;
			char* p = strchr(port, '/');
			if (p != NULL)
			{
				*p = '\0';
				path = p + 1;
			}
		}
		break;
	case '/':
		host[t] = '\0';
		path = host + t + 1;
		break;
	case '\0':
		break;
	default:
		log_error(common, "post url may be invalid : %s", url);
		return false;
	}

	struct sockaddr_in addr;
	struct hostent* site = gethostbyname(host);
	if (site == NULL)
	{
		log_error(errno, "unknown host name : %s", host);
		return false;
	}
	memcpy(&addr.sin_addr, site->h_addr, site->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(port));

	int s = socket(AF_INET,SOCK_STREAM, 0);
	if(s < 0)
	{
		log_error(errno, "create socket error");
		return false;
	}

	if (connect(s, (const struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
	{
		switch (errno)
		{
		case EINTR:
		case EISCONN:
			break;
		default:
			log_error(errno, "can not connect to remote host %s:%s", host, port);
			return false;
		}
	}

	struct iovec iovs[16];
	int iovs_count = 0;
	int total_bytes = 0;

#define SET_IOVS(x) iovs[iovs_count].iov_base=x;iovs[iovs_count].iov_len=strlen(x);total_bytes+=iovs[iovs_count].iov_len;++iovs_count;

	SET_IOVS("POST /");
	SET_IOVS(path);
	SET_IOVS(" HTTP/1.1\r\nHost: ");
	SET_IOVS(host);
	SET_IOVS("\r\nContent-Length: ");
	SET_IOVS(size);
	SET_IOVS("\r\nConnection: Close\r\n\r\n");

#undef SET_IOVS

	int send_size = -1;
	do
	{
		send_size = writev(s, iovs, iovs_count);
	} while ((send_size < 0) && (errno == EINTR));
	if (send_size != total_bytes)
	{
		log_error(errno, "send http header error, total %d, sent %d", total_bytes, send_size);
		return false;
	}

	off_t offset = 0;
	send_size = 0;
	do
	{
		int i = sendfile(s, fd, &offset, st.st_size);
		if (i < 0)
		{
			log_error(errno, "send file error");
			return false;
		}
		send_size += i;
	} while (send_size < st.st_size);

	while (read(s, url, len) > 0)
	{
		;
	}

	close(s);

	return true;
}

static bool witness_insert_globals(struct sqlite3_stmt** stmts, const char* name, const char* value)
{
    if (sqlite3_bind_text(stmts[WITNESS_STMT_GLOBALS_ID], WITNESS_GLOBALS_NAME, name, -1, NULL) != SQLITE_OK)
    {
        return false;
    }

    if (sqlite3_bind_text(stmts[WITNESS_STMT_GLOBALS_ID], WITNESS_GLOBALS_VALUE, value, -1, NULL) != SQLITE_OK)
    {
        return false;
    }

    if (sqlite3_step(stmts[WITNESS_STMT_GLOBALS_ID]) != SQLITE_DONE)
    {
        return false;
    }

    if (sqlite3_reset(stmts[WITNESS_STMT_GLOBALS_ID]) != SQLITE_OK)
    {
        return false;
    }

	return true;
}

static bool witness_parse_call(struct sqlite3* db, struct sqlite3_stmt** stmts, const char* buf, int size, int call_id, int prev_id)
{
	sqlite3_exec(db, "begin transaction;", 0, 0, NULL);

	witness_iov_get_start(buf);

	sqlite3_bind_int(stmts[WITNESS_STMT_FUNCS_ID], WITNESS_FUNCS_FUNC, call_id);
	witness_iov_get_str(file, WITNESS_STMT_FUNCS_ID, WITNESS_FUNCS_FILE);
	witness_iov_get_str(scope, WITNESS_STMT_FUNCS_ID, WITNESS_FUNCS_SCOPE);
	witness_iov_get_str(func, WITNESS_STMT_FUNCS_ID, WITNESS_FUNCS_NAME);
	witness_iov_get_int(argc, WITNESS_STMT_FUNCS_ID, WITNESS_FUNCS_ARGC);
	witness_iov_get_int(required, WITNESS_STMT_FUNCS_ID, WITNESS_FUNCS_REQUIRED);
	witness_iov_get_int(line_start, WITNESS_STMT_FUNCS_ID, WITNESS_FUNCS_START);
	witness_iov_get_int(line_end, WITNESS_STMT_FUNCS_ID, WITNESS_FUNCS_END);
	witness_iov_get_commit(WITNESS_STMT_FUNCS_ID);

	witness_iov_get_int(prev_line, WITNESS_STMT_CALLS_ID, WITNESS_CALLS_LINE);
	witness_iov_get_int(given, WITNESS_STMT_CALLS_ID, WITNESS_CALLS_PARAMS);
	sqlite3_bind_int(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_CALL, call_id);
	sqlite3_bind_int(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_PREV, prev_id);
	sqlite3_bind_text(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_FUNC, func, func_len, 0);
	sqlite3_bind_text(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_SCOPE, scope, scope_len, 0);
	witness_iov_get_commit(WITNESS_STMT_CALLS_ID);

	for (int i = 0; i < given; i++)
	{
		witness_iov_get_int(index, WITNESS_STMT_ARGVS_ID, WITNESS_ARGVS_SEQ);
		witness_iov_get_str(arg_class, WITNESS_STMT_ARGS_ID, WITNESS_ARGS_CLASS);
		witness_iov_get_str(arg_name, WITNESS_STMT_ARGS_ID, WITNESS_ARGS_NAME);
		witness_iov_get_str(arg_value, WITNESS_STMT_ARGVS_ID, WITNESS_ARGVS_VALUE);

		sqlite3_bind_int(stmts[WITNESS_STMT_ARGVS_ID], WITNESS_ARGVS_CALL, call_id);
		witness_iov_get_commit(WITNESS_STMT_ARGVS_ID);

		sqlite3_bind_int(stmts[WITNESS_STMT_ARGS_ID], WITNESS_ARGS_SEQ, index);
		sqlite3_bind_text(stmts[WITNESS_STMT_ARGS_ID], WITNESS_ARGS_SCOPE, scope, scope_len, NULL);
		sqlite3_bind_text(stmts[WITNESS_STMT_ARGS_ID], WITNESS_ARGS_FUNC, func, func_len, NULL);
		witness_iov_get_commit(WITNESS_STMT_ARGS_ID);
	}

	sqlite3_exec(db, "commit transaction;", 0, 0, NULL);

	return true;
}

static bool witness_read_pipe(int pipe, struct sqlite3* db, struct sqlite3_stmt** stmts)
{
	witness_insert_globals(stmts, "type", "trace");

	const int max = WITNESS_G(max_stack);

	struct 
	{
		int				call_id;
		struct timespec	begin;
	} stack[max], *top;
	int call_seq = 1;
	top = stack;
	top->call_id = 0;
	clock_gettime(CLOCK_MONOTONIC, &top->begin);

	uint32_t header[2];
	int ret = 0;
	bool result = false;
	while ((ret = read(pipe, header, sizeof(header))) == sizeof(header))
	{
		char buf[header[0]];
		char* cur = buf;
		int read_len = header[0] - sizeof(header);
		do
		{
			ret = read(pipe, cur, read_len);
			if (ret < 0)
			{
				log_error(errno, "read pipe error");
				return false;
			}
			read_len -= ret;
			cur += ret;
		} while (read_len > 0);

		switch (header[1])
		{
		case WITNESS_TYPE_CALL:
			if (top - stack >= max - 1)
			{
				log_warn(errno, "stack overflow, max : %d", max);
				return result;
			}
			++top;
			top->call_id = ++call_seq;
			clock_gettime(CLOCK_MONOTONIC, &top->begin);
			witness_parse_call(db, stmts, buf, header[0] - sizeof(header), top->call_id, (top - 1)->call_id);
			break;
		case WITNESS_TYPE_RET:
			{
				struct timespec now;
				clock_gettime(CLOCK_MONOTONIC, &now);
				int use = (now.tv_sec - top->begin.tv_sec) * 1000 * 1000 + (now.tv_nsec - top->begin.tv_nsec) * 0.001;

				witness_iov_get_start(buf);
				witness_iov_get_str(value, WITNESS_STMT_RETVAL_ID, WITNESS_RETVAL_VAL);
				sqlite3_bind_int(stmts[WITNESS_STMT_RETVAL_ID], WITNESS_RETVAL_ID, top->call_id);
				sqlite3_bind_int(stmts[WITNESS_STMT_RETVAL_ID], WITNESS_TIME_VAL, use);
				witness_iov_get_end(WITNESS_STMT_RETVAL_ID);
				--top;
			}
			break;
		case WITNESS_TYPE_VAR:
			{
				witness_iov_get_start(buf);
				witness_iov_get_str(name, WITNESS_STMT_GLOBALS_ID, WITNESS_GLOBALS_NAME);
				witness_iov_get_str(value, WITNESS_STMT_GLOBALS_ID, WITNESS_GLOBALS_VALUE);
				if (strcasecmp(name, "end") == 0)
				{
					result = true;
					witness_iov_get_reset(WITNESS_STMT_GLOBALS_ID);
				}
				else
				{
					witness_iov_get_end(WITNESS_STMT_GLOBALS_ID);
				}
			}
			break;
		}
	}

	return result;
}

static int witness_dump_func(int id, struct sqlite3* db, struct sqlite3_stmt** stmts, const zend_execute_data* data TSRMLS_DC)
{
	zend_function* func = data->function_state.function;
	if ((func == NULL) || (func->common.function_name == NULL))
	{
		return -1;
	}

	const char* func_name = func->common.function_name;
	const char* func_scope = (func->common.scope == NULL) ? "" : func->common.scope->name;
	const char* func_file = (func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.filename : "";

	sqlite3_exec(db, "begin transaction;", 0, 0, NULL);

	sqlite3_reset(stmts[WITNESS_STMT_FUNCS_ID]);
	sqlite3_bind_int(stmts[WITNESS_STMT_FUNCS_ID], WITNESS_FUNCS_FUNC, id);
	sqlite3_bind_text(stmts[WITNESS_STMT_FUNCS_ID], WITNESS_FUNCS_NAME, func_name, -1, NULL);
	sqlite3_bind_text(stmts[WITNESS_STMT_FUNCS_ID], WITNESS_FUNCS_SCOPE, func_scope, -1, NULL);
	sqlite3_bind_text(stmts[WITNESS_STMT_FUNCS_ID], WITNESS_FUNCS_FILE, func_file, -1, NULL);
	sqlite3_bind_int(stmts[WITNESS_STMT_FUNCS_ID], WITNESS_FUNCS_ARGC, func->common.num_args);
	sqlite3_bind_int(stmts[WITNESS_STMT_FUNCS_ID], WITNESS_FUNCS_ARGC, func->common.required_num_args);
	sqlite3_bind_int(stmts[WITNESS_STMT_FUNCS_ID], WITNESS_FUNCS_START, (*func_file != '\0') ? func->op_array.line_start : 0);
	sqlite3_bind_int(stmts[WITNESS_STMT_FUNCS_ID], WITNESS_FUNCS_END, (*func_file != '\0') ? func->op_array.line_end : 0);
	sqlite3_step(stmts[WITNESS_STMT_FUNCS_ID]);

	int given = (int)(zend_uintptr_t)data->function_state.arguments[0];

	sqlite3_reset(stmts[WITNESS_STMT_CALLS_ID]);
	sqlite3_bind_int(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_CALL, id);
	sqlite3_bind_int(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_PREV, id - 1);
	sqlite3_bind_int(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_PARAMS, given);
	sqlite3_bind_text(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_FUNC, func_name, -1, NULL);
	sqlite3_bind_text(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_SCOPE, func_scope, -1, NULL);
	sqlite3_bind_int(stmts[WITNESS_STMT_CALLS_ID], WITNESS_CALLS_LINE, (data->opline != NULL) ? data->opline->lineno : 0);
	sqlite3_step(stmts[WITNESS_STMT_CALLS_ID]);

	if (given > 0)
	{
		smart_str str_buf[given];
		memset(str_buf, 0, sizeof(str_buf));
		char obj_buf[WITNESS_SERIALIZE_LEN][given];
		for (int i = 0; i < given; i++)
		{
			sqlite3_reset(stmts[WITNESS_STMT_ARGS_ID]);
			sqlite3_bind_int(stmts[WITNESS_STMT_ARGS_ID], WITNESS_ARGS_SEQ, i);
			if (i < func->common.num_args)
			{
				sqlite3_bind_text(stmts[WITNESS_STMT_ARGS_ID], WITNESS_ARGS_CLASS, (func->common.arg_info[i].class_name != NULL) ? func->common.arg_info[i].class_name : "", -1, NULL);
				sqlite3_bind_text(stmts[WITNESS_STMT_ARGS_ID], WITNESS_ARGS_NAME, func->common.arg_info[i].name, -1, NULL);
			}
			sqlite3_bind_text(stmts[WITNESS_STMT_ARGS_ID], WITNESS_ARGS_FUNC, func_name, -1, NULL);
			sqlite3_bind_text(stmts[WITNESS_STMT_ARGS_ID], WITNESS_ARGS_SCOPE, func_scope, -1, NULL);
			sqlite3_step(stmts[WITNESS_STMT_ARGS_ID]);

			const char* show = "";
			zval** z = (zval**)&data->function_state.arguments[i - given];
			witness_serialize(show, z, str_buf, obj_buf, i);
			sqlite3_reset(stmts[WITNESS_STMT_ARGVS_ID]);
			sqlite3_bind_int(stmts[WITNESS_STMT_ARGVS_ID], WITNESS_ARGVS_CALL, id);
			sqlite3_bind_int(stmts[WITNESS_STMT_ARGVS_ID], WITNESS_ARGVS_SEQ, i);
			sqlite3_bind_text(stmts[WITNESS_STMT_ARGVS_ID], WITNESS_ARGVS_VALUE, show, -1, NULL);
			sqlite3_step(stmts[WITNESS_STMT_ARGVS_ID]);
		}
	}

	int locals = data->op_array->last_var;
	if (locals > 0)
	{
		smart_str str_buf[locals];
		memset(str_buf, 0, sizeof(str_buf));
		char obj_buf[WITNESS_SERIALIZE_LEN][locals];
		for (int i = 0; i < locals; i++)
		{
			const char* show = "";
			witness_serialize(show, data->CVs[i], str_buf, obj_buf, i);

			sqlite3_reset(stmts[WITNESS_STMT_LOCALS_ID]);
			sqlite3_bind_int(stmts[WITNESS_STMT_LOCALS_ID], WITNESS_LOCALS_CALL, id - 1);
			sqlite3_bind_text(stmts[WITNESS_STMT_LOCALS_ID], WITNESS_LOCALS_NAME, data->op_array->vars[i].name, data->op_array->vars[i].name_len, NULL);
			sqlite3_bind_text(stmts[WITNESS_STMT_LOCALS_ID], WITNESS_LOCALS_VALUE, show, -1, NULL);
			sqlite3_step(stmts[WITNESS_STMT_LOCALS_ID]);
		}
	}

	sqlite3_exec(db, "commit transaction;", 0, 0, NULL);

	return 0;
}

static bool witness_dump_stack(int fd, struct sqlite3* db, struct sqlite3_stmt** stmts)
{
	witness_insert_globals(stmts, "type", "coredump");

	zend_execute_data* data = EG(current_execute_data);

	int count = 0;
	int max = WITNESS_G(max_stack);
	zend_execute_data* stack[max];
	while ((data != NULL) && (count < max))
	{
		stack[count++] = data;
		data = data->prev_execute_data;
	}

	zval** value = NULL;
	for (zend_hash_internal_pointer_reset(&EG(symbol_table)); zend_hash_get_current_data(&EG(symbol_table), (void**)&value) == SUCCESS; zend_hash_move_forward(&EG(symbol_table)))
	{
		char* key_str = NULL;
		unsigned long key_int = -1;
		if (zend_hash_get_current_key(&EG(symbol_table), &key_str, &key_int, false) == HASH_KEY_IS_STRING)
		{
			if (memcmp(key_str, "GLOBALS", strlen("GLOBALS") + 1) != 0)
			{
				const char* show = "";
				char class_buf[WITNESS_SERIALIZE_LEN];
				smart_str str_buf = {0};

				witness_serialize(show, value, &str_buf, &class_buf, 0);

				witness_insert_globals(stmts, key_str, show);
			}
		}
	}
	
	int id = 0;
	if (count > 0)
	{
		for (int i = count - 1; i >= 0; i--)
		{
			witness_dump_func(++id, db, stmts, stack[i]);
		}
	}

	return true;
}

static void witness_child(int fd, const char* token, int token_len, const char* prefix, bool (*proc)(int, struct sqlite3*, struct sqlite3_stmt**))
{
	errno = 0;
    struct sqlite3* db = NULL;
    if (sqlite3_open(":memory:", &db) < 0)
    {
		log_error(errno, "can not create memory db by sqlite3 : %s", sqlite3_errmsg(db));
        return;
    }

    if (sqlite3_exec(db, WITNESS_CREATE_TABLE, NULL, NULL, NULL) != SQLITE_OK)
	{
		log_error(errno, "create table error : %s", WITNESS_CREATE_TABLE);
		return;
	}

    const char* sqls[] =
	{
		WITNESS_INSERT_FUNCS, 		//0
		WITNESS_INSERT_CALLS, 		//1
		WITNESS_INSERT_ARGVS, 		//2
		WITNESS_INSERT_GLOBALS,		//3
		WITNESS_UPDATE_RETVAL,		//4
		WITNESS_INSERT_ARGS,		//5
		WITNESS_INSERT_LOCALS,		//6
	};
    struct sqlite3_stmt* stmts[sizeof(sqls)/sizeof(sqls[0])];
	for (int i = 0; i < sizeof(sqls)/sizeof(sqls[0]); i++)
	{
		if (sqlite3_prepare(db, sqls[i], -1, &stmts[i], 0) != SQLITE_OK)
		{
			log_error(errno, "prepare sql error : %s", sqls[i]);
			return;
		}
	}

	witness_insert_globals(stmts, "token", token);

	struct timeval tv;
	struct tm t;
	char buf[1024];
	if (gettimeofday(&tv, NULL) == 0)
	{
		struct tm* tp = localtime_r(&tv.tv_sec, &t);
		sprintf(buf, "%04d-%02d-%02d.%02d:%02d:%02d.%d", tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec, (int)tv.tv_usec);
		witness_insert_globals(stmts, "start", buf);
	}

	if (proc(fd, db, stmts) == true)
	{
		if (gettimeofday(&tv, NULL) == 0)
		{
			struct tm* tp = localtime_r(&tv.tv_sec, &t);
			sprintf(buf, "%04d-%02d-%02d.%02d:%02d:%02d.%d", tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec, (int)tv.tv_usec);
			witness_insert_globals(stmts, "end", buf);
		}
	}

	for (int i = 0; i < sizeof(stmts)/sizeof(stmts[0]); i++)
	{
		if (sqlite3_finalize(stmts[i]) != SQLITE_OK)
		{
			log_warn(errno, "finalize sql error : %s", sqls[i]);
		}
	}

	char name[1024];
	snprintf(name, sizeof(name), "%s/witness_%s_%s_%d_%d", WITNESS_G(temp_path), prefix, token, tv.tv_sec, tv.tv_usec);
	name[sizeof(name) - 1] = '\0';

    //dump table
	snprintf(buf, sizeof(buf), "attach '%s' as dump_db", name);
	buf[sizeof(buf) - 1] = '\0';
    if (sqlite3_exec(db, buf, 0, 0, NULL) != SQLITE_OK)
    {
		log_error(errno, "can not create attach db : %s, msg : %s", name, sqlite3_errmsg(db));
		return;
    }
    if (sqlite3_exec(db, WITNESS_DUMP_TABLE, 0, 0, NULL) != SQLITE_OK)
    {
		log_error(errno, "dump db error : %s", sqlite3_errmsg(db));
        return;
    }

    if (sqlite3_close(db) < 0)
    {
		log_warn(errno, "close db error");
        return;
    }

	if (WITNESS_G(post_url)[0] != '\0')
	{
		const char* start = WITNESS_G(post_url);
		const char* end = start;
		char* dest = buf;
		int can_copy = sizeof(buf) - 1;
		while ((end = strstr(start, WITNESS_URL_TOKEN)) != NULL)
		{
			int s = end - start;
			if (can_copy <= (s + token_len))
			{
				log_warn(common, "post url is too long : %s", start);
				return;
			}
			memcpy(dest, start, s);
			dest += s;
			memcpy(dest, token, token_len);
			dest += token_len;
			can_copy -= (s + token_len);
			start = end + strlen(WITNESS_URL_TOKEN);
		}
		strncpy(dest, start, can_copy);
		buf[sizeof(buf) - 1] = '\0';

		if ((witness_post(buf, sizeof(buf), name) == true) || (WITNESS_G(keep_record) == false))
		{
			unlink(name);
		}
	}

	//_exit(0);
}

static void witness_clear(void)
{
	if (witness_is_recording(g_record) == true)
	{
		witness_iov_set_start();
		witness_iov_set_int(0);
		witness_iov_set_int(WITNESS_TYPE_VAR);
		witness_iov_set_str("end");
		witness_iov_set_str("");
		witness_iov_set_end(g_record);

		witness_reset_recording(g_record);

		zend_execute = g_run_backup.zend_execute;
		if (WITNESS_G(record_internal) == true)
		{
			zend_execute_internal = g_run_backup.zend_execute_internal;
		}
	}

	if (g_token != NULL)
	{
		free(g_token);
		g_token = NULL;
		g_token_len = -1;
	}
}

static void witness_func_info(int file, const zend_execute_data* data TSRMLS_DC)
{
	zend_function* func = data->function_state.function;
	if ((func == NULL) || (func->common.function_name == NULL))
	{
		return;
	}

	witness_iov_set_start();

	//len
	witness_iov_set_int(0);

	//type
	witness_iov_set_int(WITNESS_TYPE_CALL);

	//file
	witness_iov_set_str((func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.filename : "");

	//func scope
	witness_iov_set_str((func->common.scope == NULL) ? "" : func->common.scope->name);

	//func name
	witness_iov_set_str(func->common.function_name);

	//argc
	witness_iov_set_int(func->common.num_args);

	//required
	witness_iov_set_int(func->common.required_num_args);

	//line start
	witness_iov_set_int((func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.line_start : 0);

	//line end
	witness_iov_set_int((func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.line_end : 0);

	//prev run line
	witness_iov_set_int((data->opline != NULL) ? data->opline->lineno : 0);

	int argc = (int)(zend_uintptr_t)data->function_state.arguments[0];
	//given
	witness_iov_set_int(argc);

	smart_str str_buf[argc];
	memset(str_buf, 0, sizeof(str_buf));
	char obj_buf[WITNESS_SERIALIZE_LEN][argc];
	for (int i = 0; i < argc; i++)
	{
		//arg id
		witness_iov_set_int(i);

		if (i < func->common.num_args)
		{
			//arg class name
			witness_iov_set_str((func->common.arg_info[i].class_name != NULL) ? func->common.arg_info[i].class_name : "");

			//arg name
			witness_iov_set_str(func->common.arg_info[i].name);
		}
		else
		{
			//arg class name
			witness_iov_set_str("");

			//arg name
			witness_iov_set_str("");
		}

		const char* show = "";

		zval** z = (zval**)&data->function_state.arguments[i - argc];
		witness_serialize(show, z, str_buf, obj_buf, i);

		//arg value
		witness_iov_set_str(show);
	}

	witness_iov_set_end(file);
}

static void witness_ret_info(int file, const zend_execute_data* data TSRMLS_DC)
{
	zend_function* func = data->function_state.function;
	if ((func == NULL) || (func->common.function_name == NULL))
	{
		return;
	}

	witness_iov_set_start();

	//len
	witness_iov_set_int(0);

	//type
	witness_iov_set_int(WITNESS_TYPE_RET);

	zval** ret = NULL;
	const char* show = "";
	char class_buf[WITNESS_SERIALIZE_LEN];
	smart_str str_buf = {0};
	if ((data->opline != NULL) && (RETURN_VALUE_USED(data->opline) == true))
	{
		if (func->type == ZEND_USER_FUNCTION)
		{
			ret = EG(return_value_ptr_ptr);
		}
		else
		{
			const zend_execute_data* execute_data = data;
			ret = EX_T(data->opline->result.u.var).var.ptr_ptr;
		}

		if ((ret != NULL) && (*ret != NULL))
		{
			witness_serialize(show, ret, &str_buf, &class_buf, 0);
		}
	}
	else
	{
		show = (data->opline != NULL) ? "return_value_not_used" : "";
	}

	//retval
	witness_iov_set_str(show);

	witness_iov_set_end(file);
}

static void witness_stack_info(int file)
{
	zend_execute_data* data = EG(current_execute_data);

	int count = 0;
	int max = WITNESS_G(max_stack);
	zend_execute_data* stack[max];
	while (data != NULL)
	{
		stack[count] = data;
		data = data->prev_execute_data;
		if (++count > max)
		{
			break;
		}
	}

	if (count > 0)
	{
		for (int i = count - 1; i >= 0; i--)
		{
			witness_func_info(file, stack[i]);
		}
	}
}

static void witness_var_info(int file, int count, ...)
{
	va_list names;
	va_start(names, count);


	for (int i = 0; i < count; i++)
	{
		witness_iov_set_start();

		//len
		witness_iov_set_int(0);

		//type
		witness_iov_set_int(WITNESS_TYPE_VAR);

		const char* name = va_arg(names, const char*);

		//var name
		witness_iov_set_str(name);

		zval** result = NULL;
		const char* show = "";
		char class_buf[WITNESS_SERIALIZE_LEN];
		smart_str str_buf = {0};
		if (zend_hash_find(&EG(symbol_table), name, strlen(name) + 1, (void**)&result) == SUCCESS)
		{
			witness_serialize(show, result, &str_buf, &class_buf, 0);
		}

		//var value
		witness_iov_set_str(show);

		witness_iov_set_end(file);
	}

	va_end(names);
}

ZEND_DLEXPORT void witness_execute(zend_op_array* ops TSRMLS_DC)
{
	zend_execute_data* data = EG(current_execute_data);
	if ((witness_is_recording(g_record) == true) && (data != NULL))
	{
		witness_func_info(g_record, data);
	}

	g_run_backup.zend_execute(ops TSRMLS_CC);

	if ((witness_is_recording(g_record) == true) && (data != NULL))
	{
		witness_ret_info(g_record, data);
	}
}

ZEND_DLEXPORT void witness_execute_internal(zend_execute_data* execute_data, int ret TSRMLS_DC)
{
	zend_execute_data* data = EG(current_execute_data);
	if ((witness_is_recording(g_record) == true) && (data != NULL))
	{
		witness_func_info(g_record, data);
	}

	if (g_run_backup.zend_execute_internal == NULL)
	{
		/* no old override to begin with. so invoke the builtin's implementation  */
		zend_op* opline = EX(opline);
		((zend_internal_function*) EX(function_state).function)->handler(opline->extended_value, EX_T(opline->result.u.var).var.ptr, EX(function_state).function->common.return_reference ? &EX_T(opline->result.u.var).var.ptr : NULL, EX(object), ret TSRMLS_CC);

	}
	else
	{
		/* call the old override */
		g_run_backup.zend_execute_internal(execute_data, ret TSRMLS_CC);
	}

	if ((witness_is_recording(g_record) == true) && (data != NULL))
	{
		witness_ret_info(g_record, data);
	}
}

static bool witness_start_trace(const char* token, int len, bool stack)
{
	if (witness_is_recording(g_record) == true)
	{
		return true;
	}

	int fds[2];
	if (pipe(fds) < 0)
	{
		log_error(errno, "create pipe error");
		return false;
	}

    signal(SIGCHLD, SIG_IGN);

	switch (fork())
	{
	case 0:
		//child process
		close(fds[1]);
		witness_child(fds[0], token, len, "trace", witness_read_pipe);
		close(fds[0]);
		_exit(0);
	case -1:
		witness_reset_recording(g_record);
		log_error(errno, "fork child error for token %s", token);
		return false;
	default:
		close(fds[0]);
		g_record = fds[1];
		break;
	}

	g_run_backup.zend_execute = zend_execute;
	g_run_backup.zend_execute_internal = zend_execute_internal;

	zend_execute = witness_execute;
	if (WITNESS_G(record_internal) == true)
	{
		zend_execute_internal = witness_execute_internal;
	}

	witness_var_info(g_record, 7, "_POST", "_GET", "_SESSION", "_ENV", "_COOKIE", "_SERVER", "_FILES");

	if (stack == true)
	{
		witness_stack_info(g_record);
	}

	return true;
}

static bool witness_start_dump(const char* token, int len)
{
    signal(SIGCHLD, SIG_IGN);

	errno = 0;
	switch (fork())
	{
	case 0:
		//child process
		witness_child(-1, token, len, "core", witness_dump_stack);
		_exit(0);
	case -1:
		log_error(errno, "fork child error for token %s", token);
		return false;
	default:
		return true;
	}
}

PHP_FUNCTION(witness_set_token) {
	char* token      = "";
	int token_len    = -1;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &token, &token_len) == FAILURE)
	{
		return;
	}

	if ((token == NULL) || (token_len < 0))
	{
		RETURN_FALSE;
	}

	if (g_token != NULL)
	{
		free(g_token);
	}

	g_token_len = token_len;
	g_token = malloc(g_token_len + 1);
	memcpy(g_token, token, g_token_len);
	g_token[g_token_len] = '\0';

	RETURN_TRUE;
}

PHP_FUNCTION(witness_start) {
	char* token      = "";
	int token_len    = -1;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &token, &token_len) == FAILURE)
	{
		return;
	}

	if ((*token == '\0') && (g_token != NULL))
	{
		token = g_token;
		token_len = g_token_len;
	}

	if (witness_start_trace(token, token_len, true) == true)
	{
		RETURN_TRUE;
	}
	else
	{
		log_warn(errno, "can not start trace for token : %s", token);
		RETURN_FALSE;
	}
}

PHP_FUNCTION(witness_stop) {

	if (witness_is_recording(g_record) == false)
	{
		RETURN_FALSE;
	}

	witness_clear();

	RETURN_TRUE;
}

PHP_FUNCTION(witness_dump) {
	char* token   = "";
	int token_len = -1;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &token, &token_len) == FAILURE)
	{
		return;
	}

	if (WITNESS_G(enable_dump) == false)
	{
		RETURN_FALSE;
	}

	if ((*token == '\0') && (g_token != NULL))
	{
		token = g_token;
		token_len = g_token_len;
	}

	if (witness_start_dump(token, token_len) == true)
	{
		RETURN_TRUE;
	}
	else
	{
		if (WITNESS_G(enable_dump) == true)
		{
			log_warn(errno, "can not create dump info for token : %s", token);
		}
		RETURN_FALSE;
	}
}

PHP_FUNCTION(witness_assert) {
	bool expression = true;
	char* token     = "";
	int token_len   = -1;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b|s", &expression, &token, &token_len) == FAILURE)
	{
		return;
	}

	if (WITNESS_G(enable_assert) == false)
	{
		if (expression == true)
		{
			RETURN_TRUE;
		}
		else
		{
			RETURN_FALSE;
		}
	}

	if (expression == true)
	{
		RETURN_TRUE;
	}

	if ((*token == '\0') && (g_token != NULL))
	{
		token = g_token;
		token_len = g_token_len;
	}

	if (witness_start_dump(token, token_len) == false)
	{
		if (WITNESS_G(enable_dump) == true)
		{
			log_warn(errno, "assert failure, but can not dump info for token : %s", token);
		}
	}

	RETURN_FALSE;
}

PHP_FUNCTION(witness_stack_info) {
	long depth = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &depth) == FAILURE)
	{
		return;
	}

	zend_execute_data* data = EG(current_execute_data);
	zend_execute_data* prev = data;
	for (int i = -1; i < depth && data != NULL; i++)
	{
		prev = data;
		data = prev->prev_execute_data;
	}

	if (data == NULL)
	{
		log_warn(common, "invalid depth of stack %d", depth);
		RETURN_NULL();
	}

	zend_function* func = data->function_state.function;

	array_init(return_value);

	add_assoc_string(return_value, "file_name", (func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.filename : "", 1);
	add_assoc_string(return_value, "scope_name", (func->common.scope == NULL) ? "" : func->common.scope->name, 1);
	add_assoc_string(return_value, "function_name", func->common.function_name, 1);
	add_assoc_long(return_value, "line_start", (func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.line_start : 0);
	add_assoc_long(return_value, "line_end", (func->type != ZEND_INTERNAL_FUNCTION) ? func->op_array.line_end : 0);
	add_assoc_long(return_value, "line_current", (prev->opline != NULL) ? prev->opline->lineno : 0);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
