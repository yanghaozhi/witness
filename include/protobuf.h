#ifndef __PHP_PROTOBUF_MACRO_H__
#define __PHP_PROTOBUF_MACRO_H__

#include <limits.h>
#include <stdint.h>
#include <string.h>

#if (__cplusplus) || (__STDC_VERSION__ < 199901L)
#define restrict __restrict__
#endif

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifndef likely
#ifdef __GNUC__ 
#define likely(x)   __builtin_expect(!!(x), 1)
//#define likely(x)   (x)
#else
#define likely(x)   (x)
#endif
#endif

#ifndef unlikely
#ifdef __GNUC__ 
#define unlikely(x) __builtin_expect(!!(x), 0)
//#define unlikely(x) (x)
#else
#define unlikely(x) (x)
#endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////

/*********************************************************************************************
 * file encoding : utf-8
 * 追加一个变量
 *
 * 参数      : 类型     : 解释
 *
 * type      : (varint) : 变量的protobuf类型。参考protobuf类型，可选有：sint32/sint64/uint32/uint64/bool/enum/fixed32/fixed64/sfixed32/sfixed64/double/float/string/bytes/embedded
 * field     : int      : 区域id。
 * value     : (varint) : 变量值。（对于string/bytes/embedded类型，传入NULL表示不需要写入实际数据，只需要写入标签头，数据自行拼接）
 * buf       : char*    : 写入的缓冲区
 * size      : int      : 缓冲区可写大小
 * value_len : int      : 变量长度（可选）。该参数仅对string/bytes/embedded类型有效，其余类型忽略
 *
 * 返回值    : int      : 已写入长度，如果失败，返回0
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-12-23                    create
**********************************************************************************************/
#define protobuf_append(type, field, value, buf, size, ...)  ({protobuf_append_##type(impl, field, value, buf, size, ##__VA_ARGS__);})

////////////////////////////////////////////////////////////////////////////////////////////////////////

/*********************************************************************************************
 * file encoding : utf-8
 * 追加一个打包变量
 *
 * 参数      : 类型     : 解释
 *
 * type      : (varint) : 变量的protobuf类型。参考protobuf类型，可选有：sint32/sint64/uint32/uint64/bool/enum/fixed32/fixed64/sfixed32/sfixed64/double/float
 * field     : int      : 区域id。
 * value     : (varint) : 变量值。
 * buf       : char*    : 写入的缓冲区
 * size      : int      : 缓冲区可写大小
 *
 * 返回值    : int      : 已写入长度，如果失败，返回0
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-12-23                    create
**********************************************************************************************/
#define protobuf_append_packed(type, value, buf, size)  ({protobuf_append_##type(packed_impl, 0, value, buf, size);})


/*********************************************************************************************
 * file encoding : utf-8
 * 解析一个变量
 *
 * 参数      : 类型     : 解释
 *
 * param     : void*    : 回调参数
 * buf       : char*    : 读出的缓冲区
 * size      : int      : 缓冲区可读大小
 * cb        : func     : 回调函数，成功解析到变量后会回调该函数
 *
 * 返回值    : int      : 已读出长度，如果失败，返回-1
 * 
 * 回调函数
 * param     : void*    : 回调参数
 * field     : int      : 区域id。
 * wire_type : int      : 变量的实际存储类型
 * value     : uint64_t : 变量值。如果需要再转为具体的c/c++类型，需要调用protobuf_parse_value
 * extra     : char*    : 附加指针。该指针在位string/buffer/embedded中有效：
 * 如果类型为string/buffer，该指针指向string/buffer的首地址
 * value中存放的是该string/buffer的数据长度
 * 另外，对于string类型，在末尾不会有'\0'字节的终结符
 * 如果类型为embedded，该指针指向嵌套区域的首地址
 * value中存放的是该嵌套区域的长度
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-12-23                    create
**********************************************************************************************/
static inline int protobuf_parse(void* param, const char* restrict buf, int size, bool (*cb)(void* param, int field, int wire_type, uint64_t value, const char* restrict extra));

/*********************************************************************************************
 * file encoding : utf-8
 * 解析变量压缩的重复变量
 *
 * 参数      : 类型     : 解释
 *
 * type      : (varint) : 变量的protobuf类型。参考protobuf类型，可选有：sint32/sint64/uint32/uint64/bool/enum/fixed32/fixed64/sfixed32/sfixed64/double/float/string/bytes/embedded
 * value     : uint64_t : 变量值。
 *
 * 返回值    : (varint) : 根据type类型，把value的类型转换为c/c++类型
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-12-23                    create
**********************************************************************************************/
#define protobuf_parse_packed(type, buffer, len, cb, ...)   ({protobuf_parse_packed_##type(buffer, len, cb, ##__VA_ARGS__);})

/*********************************************************************************************
 * file encoding : utf-8
 * 解析变量类型
 *
 * 参数      : 类型     : 解释
 *
 * type      : (varint) : 变量的protobuf类型。参考protobuf类型，可选有：sint32/sint64/uint32/uint64/bool/enum/fixed32/fixed64/sfixed32/sfixed64/double/float/string/bytes/embedded
 * value     : uint64_t : 变量值。
 *
 * 返回值    : (varint) : 根据type类型，把value的类型转换为c/c++类型
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-12-23                    create
**********************************************************************************************/
#define protobuf_parse_value(type, value)   ({protobuf_parse_##type(value);})

////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline char* protobuf_write_varint(uint64_t value, char* restrict buf)
{
    do
    {
        *buf++ = (value & 0x7F) | 0x80;
        value >>= 7;
    } while (value != 0);
    *(buf - 1) &= ~(1 << 7); 
    return buf;
}

static inline const char* protobuf_read_varint(const char* restrict buf, int size, uint64_t* ret)
{
    if (unlikely(size <= 0))
    {
        return NULL;
    }

    *ret = 0;
    int bits = 0;
    int msb = 0;
    do
    {
        *ret |= (((uint64_t)(*buf & 0x7F)) << bits);
        bits += 7;
    } while (((msb = (*buf++ & 0x80)) != 0) && (bits < sizeof(*ret) * CHAR_BIT) && (--size > 0));

    return (likely(msb == 0)) ? buf : NULL;
}

#define protobuf_zigzag_en(x, s, u) ({s t = x;u o = (t << 1) ^ (t >> ((sizeof(s) * CHAR_BIT) - 1));o;})
#define protobuf_zigzag_de(x, s, u) ({u t = x;s o = (t >> 1) ^ -((s)(t & 0x01));o;})

////////////////////////////////////////////////////////////////////////////////////////////////////////

#define protobuf_append_varint_impl(field, value, buf, size)            \
({                                                                      \
    int ret = 0;                                                        \
    if (likely(size > sizeof(value) + 2 + sizeof(field)))               \
    {                                                                   \
        char* p = protobuf_write_varint(field << 3, buf);               \
        ret = protobuf_write_varint(value, p) - buf;                    \
    }                                                                   \
    ret;                                                                \
 })

#define protobuf_append_varint_packed_impl(field, value, buf, size)     \
({                                                                      \
    int ret = 0;                                                        \
    if (likely(size > sizeof(value) + 2))                               \
    {                                                                   \
        ret = protobuf_write_varint(value, buf) - buf;                  \
    }                                                                   \
    ret;                                                                \
 })

#define protobuf_append_uint64(ext, field, value, buf, size) ({uint64_t tmp = value; protobuf_append_varint_##ext(field, tmp, buf, size);})
#define protobuf_append_uint32(ext, field, value, buf, size) ({uint32_t tmp = value; protobuf_append_varint_##ext(field, tmp, buf, size);})

#define protobuf_append_sint64(ext, field, value, buf, size) ({protobuf_append_varint_##ext(field, protobuf_zigzag_en(value, int64_t, uint64_t), buf, size);})
#define protobuf_append_sint32(ext, field, value, buf, size) ({protobuf_append_varint_##ext(field, protobuf_zigzag_en(value, int32_t, uint32_t), buf, size);})

//to avoid bool is defined as _Bool
#define protobuf_append__Bool(ext, field, value, buf, size) ({bool tmp = value; protobuf_append_varint_##ext(field, tmp, buf, size);})
#define protobuf_append_bool(ext, field, value, buf, size) ({bool tmp = value; protobuf_append_varint_##ext(field, tmp, buf, size);})
#define protobuf_append_enum(ext, field, value, buf, size) ({uint32_t tmp = value; protobuf_append_varint_##ext(field, tmp, buf, size);})

////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int protobuf_append_fixed_impl(const int field, const uint8_t wire_type, const void* value, int value_len, char* restrict buf, int buf_size)
{
    if (unlikely(buf_size < sizeof(field) + 1 + value_len))
    {
        return 0;
    }
    char* p = protobuf_write_varint(((field << 3) | wire_type), buf);
    memcpy(p, value, value_len);
    return (p - buf) + value_len;
}

static inline int protobuf_append_fixed_packed_impl(const int field, const uint8_t wire_type, const void* value, int value_len, char* restrict buf, int buf_size)
{
    if (unlikely(buf_size < 1 + value_len))
    {
        return 0;
    }
    memcpy(buf, value, value_len);
    return value_len;
}

#define protobuf_append_fixed64(ext, field, value, buf, size) ({uint64_t tmp = value; protobuf_append_fixed_##ext(field, 0x01, &tmp, sizeof(tmp), buf, size);})
#define protobuf_append_sfixed64(ext, field, value, buf, size) ({int64_t tmp = value; protobuf_append_fixed_##ext(field, 0x01, &tmp, sizeof(tmp), buf, size);})
#define protobuf_append_double(ext, field, value, buf, size) ({double tmp = value; protobuf_append_fixed_##ext(field, 0x01, &tmp, sizeof(tmp), buf, size);})
#define protobuf_append_fixed32(ext, field, value, buf, size) ({uint32_t tmp = value; protobuf_append_fixed_##ext(field, 0x05, &tmp, sizeof(tmp), buf, size);})
#define protobuf_append_sfixed32(ext, field, value, buf, size) ({int32_t tmp = value; protobuf_append_fixed_##ext(field, 0x05, &tmp, sizeof(tmp), buf, size);})
#define protobuf_append_float(ext, field, value, buf, size) ({float tmp = value; protobuf_append_fixed_##ext(field, 0x05, &tmp, sizeof(tmp), buf, size);})

////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int protobuf_append_len_delimited_impl(const int field, const void* value, int value_len, char* restrict buf, int buf_size)
{
    if (unlikely(buf_size < sizeof(field) + 1 + sizeof(value_len) + value_len))
    {
        return 0;
    }
    char* p = protobuf_write_varint(((field << 3) | 0x02), buf);
    p = protobuf_write_varint(value_len, p);
    if (value == NULL)
    {
        return (p - buf);
    }
    else
    {
        memcpy(p, value, value_len);
        return (p - buf) + value_len;
    }
}

//to make error
#define protobuf_append_len_delimited_packed_impl() you_should_not_call_this_function()

#define protobuf_append_string(ext, field, value, buf, size) ({protobuf_append_len_delimited_##ext(field, value, strlen(value), buf, size);})
#define protobuf_append_bytes(ext, field, value, buf, size, value_len) ({protobuf_append_len_delimited_##ext(field, value, value_len, buf, size);})
#define protobuf_append_embedded protobuf_append_bytes

////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int protobuf_parse(void* param, const char* restrict buf, int size, bool (*cb)(void* param, int field, int wire_type, uint64_t value, const char* restrict extra))
{
    uint64_t key = 0;
    uint64_t value = 0;
    const char* extra = NULL;
    const char* p = protobuf_read_varint(buf, size, &key);
    if (unlikely(p == NULL))
    {
        return -1;
    }
    int wire_type = key & 0x07;
    int field = key >> 3;
    size -= (p - buf);

#define READ_VARINT p=protobuf_read_varint(p, size, &value);if(unlikely(p==NULL)){return -1;}
#define READ_FIXED(x) if(unlikely(size<sizeof(x))){return -1;}value=*(x*)p;p+=sizeof(x);

    switch (wire_type)
    {
    case 0:
        READ_VARINT;
        break;
    case 1:
        READ_FIXED(uint64_t);
        break;
    case 2:
        READ_VARINT;
        extra = p;
        p += value;
        break;
    case 5:
        READ_FIXED(uint32_t);
        break;
    default:
        return -1;
    }

#undef READ_VARINT
#undef READ_FIXED

    return (likely(cb(param, field, wire_type, value, extra) == true)) ? p - buf : -1;
}

#define protobuf_parse_uint32(value)   ((uint32_t)value)
#define protobuf_parse_uint64(value)   ((uint64_t)value)
#define protobuf_parse_sint32(value)   protobuf_zigzag_de(value, int32_t, uint32_t)
#define protobuf_parse_sint64(value)   protobuf_zigzag_de(value, int64_t, uint64_t)
#define protobuf_parse_bool(value)     ((bool)value)
#define protobuf_parse_enum(value)     ((uint32_t)value)

#define protobuf_parse_fixed32(value)  ((uint32_t)value)
#define protobuf_parse_fixed64(value)  ((uint64_t)value)
#define protobuf_parse_sfixed32(value) ((int32_t)value)
#define protobuf_parse_sfixed64(value) ((int64_t)value)
#define protobuf_parse_double(value)   (*(double*)&value)
#define protobuf_parse_float(value)    (*(float*)&value)

#define protobuf_parse_varint_packed_impl(buf, len, cb, ...)                \
({                                                                          \
    const char* p = buf;                                                    \
    const char* end = buf + len;                                            \
    uint64_t value = 0;                                                     \
    bool result = true;                                                     \
    do                                                                      \
    {                                                                       \
        p = protobuf_read_varint(p, end - p, &value);                       \
        if (unlikely((p != NULL) && (cb(value, ##__VA_ARGS__) == false)))   \
        {                                                                   \
            result = false;                                                 \
            break;                                                          \
        }                                                                   \
    } while (end > p);                                                      \
    result;                                                                 \
})

#define protobuf_parse_fixed_packed_impl(type, buf, len, cb, ...)   \
({                                                                  \
    const char* p = buf;                                            \
    const char* end = buf + len;                                    \
    bool result = true;                                             \
    do                                                              \
    {                                                               \
        if (unlikely(cb(*(type*)p, ##__VA_ARGS__) == false))        \
        {                                                           \
            result = false;                                         \
            break;                                                  \
        }                                                           \
        p += sizeof(type);                                          \
    } while (end > p);                                              \
    result;                                                         \
})

#define protobuf_parse_packed_uint32(buf, len, cb, ...) ({protobuf_parse_varint_packed_impl(buf, len, cb, ##__VA_ARGS__);})
#define protobuf_parse_packed_sint32(buf, len, cb, ...) ({protobuf_parse_varint_packed_impl(buf, len, cb, ##__VA_ARGS__);})
#define protobuf_parse_packed_uint64(buf, len, cb, ...) ({protobuf_parse_varint_packed_impl(buf, len, cb, ##__VA_ARGS__);})
#define protobuf_parse_packed_sint64(buf, len, cb, ...) ({protobuf_parse_varint_packed_impl(buf, len, cb, ##__VA_ARGS__);})

#define protobuf_parse_packed_enum(buf, len, cb, ...) ({protobuf_parse_varint_packed_impl(buf, len, cb, ##__VA_ARGS__);})
#define protobuf_parse_packed_bool(buf, len, cb, ...) ({protobuf_parse_varint_packed_impl(buf, len, cb, ##__VA_ARGS__);})
//to avoid bool is defined as _Bool
#define protobuf_parse_packed__Bool(buf, len, cb, ...) ({protobuf_parse_varint_packed_impl(buf, len, cb, ##__VA_ARGS__);})

#define protobuf_parse_packed_fixed32(buf, len, cb, ...) ({protobuf_parse_fixed_packed_impl(uint32_t, buf, len, cb, ##__VA_ARGS__);})
#define protobuf_parse_packed_sfixed32(buf, len, cb, ...) ({protobuf_parse_fixed_packed_impl(uint32_t, buf, len, cb, ##__VA_ARGS__);})
#define protobuf_parse_packed_fixed64(buf, len, cb, ...) ({protobuf_parse_fixed_packed_impl(uint64_t, buf, len, cb, ##__VA_ARGS__);})
#define protobuf_parse_packed_sfixed64(buf, len, cb, ...) ({protobuf_parse_fixed_packed_impl(uint64_t, buf, len, cb, ##__VA_ARGS__);})
#define protobuf_parse_packed_double(buf, len, cb, ...) ({protobuf_parse_fixed_packed_impl(uint64_t, buf, len, cb, ##__VA_ARGS__);})
#define protobuf_parse_packed_float(buf, len, cb, ...) ({protobuf_parse_fixed_packed_impl(uint32_t, buf, len, cb, ##__VA_ARGS__);})

////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif

