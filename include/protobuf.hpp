#ifndef __PHP_PROTOBUF_TEMPLATE_H__
#define __PHP_PROTOBUF_TEMPLATE_H__

#include <limits.h>
#include <stdint.h>

#ifndef restrict
#define restrict
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
 * value     : (varint) : 变量值。
 * buf       : char*    : 写入的缓冲区
 * size      : int      : 缓冲区可写大小
 * value_len : int      : 变量长度（可选）。该参数仅对buffer类型有效，其余类型忽略
 *
 * 返回值    : int      : 已写入长度，如果失败，返回0
---------------------------------------------------------------------------------------------
Author                 Date                          Comments
yanghz                 2011-12-23                    create
**********************************************************************************************/
#define protobuf_append(type, field, value, buf, size, ...)  (protobuf_append_##type(field, value, buf, size, ##__VA_ARGS__))

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
#define protobuf_append_packed(type, value, buf, size)  (protobuf_append_packed_##type(value, buf, size))


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
#define protobuf_parse_packed(type, buffer, len, cb, ...)   (protobuf_parse_packed_##type(buffer, len, cb, ##__VA_ARGS__))

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
#define protobuf_parse_value(type, value)   (protobuf_parse_##type(value))

////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> static inline char* protobuf_write_varint(T value, char* restrict buf)
{
    do
    {
        *buf++ = (value & 0x7F) | 0x80;
        value >>= 7;
    } while (value != 0);
    *(buf - 1) &= ~(1 << 7);
    return buf;
}

template <typename T> static inline const char* protobuf_read_varint(const char* restrict buf, int size, T* ret)
{
    if (size <= 0)
    {
        return NULL;
    }

    int bits = 0;
    int msb = 0;
    do
    {
        *ret |= (((T)(*buf & 0x7F)) << bits);
        bits += 7;
    } while (((msb = (*buf++ & 0x80)) != 0) && (bits < sizeof(T) * CHAR_BIT) && (--size > 0));

    return (msb == 0) ? buf : NULL;
}

template <typename S, typename U> static inline U protobuf_zigzag_en(S x)
{
    return (x << 1) ^ (x >> ((sizeof(S) * CHAR_BIT) - 1));
}
template <typename S, typename U> static inline S protobuf_zigzag_de(U x)
{
    return ((x >> 1) ^ -((S)(x & 0x01)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename U> static inline int protobuf_append_varint_impl(uint32_t field, char* buf, int size, U value)
{
    int ret = 0;
    if (size > sizeof(value) + 2 + sizeof(field))
    {
        char* p = protobuf_write_varint<T>(field << 3, buf);
        p = protobuf_write_varint<T>(value, p);
        ret = p - buf;
    }
    return ret;
}

template <typename T, typename U> static inline int protobuf_append_varint_packed_impl(char* buf, int size, U value)
{
    return (size > sizeof(value) + 2) ? protobuf_write_varint<T>(value, buf) - buf : 0;
}

#define protobuf_append_varint_func(x, y, ...)                                  \
template <typename T> static inline                                             \
int protobuf_append_##x(uint32_t field, T value, char* buf, int size)           \
{                                                                               \
    return protobuf_append_varint_impl<y>(field, buf, size, ##__VA_ARGS__);     \
}                                                                               \
template <typename T> static inline                                             \
int protobuf_append_packed_##x(T value, char* buf, int size)                    \
{                                                                               \
    return protobuf_append_varint_packed_impl<y>(buf, size, ##__VA_ARGS__);     \
}

protobuf_append_varint_func(uint64, uint64_t, value);
protobuf_append_varint_func(uint32, uint32_t, value);
protobuf_append_varint_func(sint64, uint64_t, protobuf_zigzag_en<int64_t, uint64_t>(value));
protobuf_append_varint_func(sint32, uint32_t, protobuf_zigzag_en<int64_t, uint32_t>(value));

protobuf_append_varint_func(bool, uint32_t, value);
protobuf_append_varint_func(enum, uint32_t, value);

////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T> static inline int protobuf_append_fixed_impl(uint32_t field, const uint8_t wire_type, const void* value, char* restrict buf, int buf_size)
{
    if (buf_size < sizeof(field) + 1 + sizeof(T))
    {
        return 0;
    }
    char* p = protobuf_write_varint(((field << 3) | wire_type), buf);
    *(T*)p = *(T*)value;
    return (p - buf) + sizeof(T);
}

template <typename T> static inline int protobuf_append_fixed_packed_impl(const uint8_t wire_type, const void* value, char* restrict buf, int buf_size)
{
    if (buf_size < 1 + sizeof(T))
    {
        return 0;
    }
    *(T*)buf = *(T*)value;
    return sizeof(T);
}

#define protobuf_append_fixed_func(x, y, z)                             \
template <typename T> static inline                                     \
int protobuf_append_##x(uint32_t field, T value, char* buf, int size)   \
{                                                                       \
    y tmp = value;                                                      \
    return protobuf_append_fixed_impl<y>(field, z, &tmp, buf, size);    \
}                                                                       \
template <typename T> static inline                                     \
int protobuf_append_packed_##x(T value, char* buf, int size)            \
{                                                                       \
    y tmp = value;                                                      \
    return protobuf_append_fixed_packed_impl<y>(z, &tmp, buf, size);    \
}

protobuf_append_fixed_func(fixed64, uint64_t, 0x01);
protobuf_append_fixed_func(sfixed64, int64_t, 0x01);
protobuf_append_fixed_func(double, double, 0x01);

protobuf_append_fixed_func(fixed32, uint32_t, 0x05);
protobuf_append_fixed_func(sfixed32, int32_t, 0x05);
protobuf_append_fixed_func(float, float, 0x05);

////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int protobuf_append_len_delimited(const int field, const void* value, int value_len, char* restrict buf, int buf_size)
{
    if (buf_size < sizeof(field) + 1 + sizeof(value_len) + value_len)
    {
        return 0;
    }
    char* p = protobuf_write_varint(((field << 3) | 0x02), buf);
    p = protobuf_write_varint(value_len, p);
    memcpy(p, value, value_len);
    return (p - buf) + value_len;
}

#define protobuf_append_string(field, value, buf, size) (protobuf_append_len_delimited(field, value, strlen(value), buf, size))
#define protobuf_append_bytes(field, value, buf, size, value_len) (protobuf_append_len_delimited(field, value, value_len, buf, size))
#define protobuf_append_embedded protobuf_append_bytes

////////////////////////////////////////////////////////////////////////////////////////////////////////

static inline int protobuf_parse(void* param, const char* restrict buf, int size, bool (*cb)(void* param, int field, int wire_type, uint64_t value, const char* restrict extra))
{
    uint64_t key = 0;
    uint64_t value = 0;
    const char* extra = NULL;
    const char* p = protobuf_read_varint(buf, size, &key);
    if (p == NULL)
    {
        return -1;
    }
    int wire_type = key & 0x07;
    int field = key >> 3;
    size -= (p - buf);

#define READ_VARINT p=protobuf_read_varint(p, size, &value);if(p==NULL){return -1;}
#define READ_FIXED(x) if(size<sizeof(x)){return -1;}value=*(x*)p;p+=sizeof(x);

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

    return (cb(param, field, wire_type, value, extra) == true) ? p - buf : -1;
}

#define protobuf_parse_uint32(value)   static_cast<uint32_t>(value)
#define protobuf_parse_uint64(value)   static_cast<uint64_t>(value)
#define protobuf_parse_sint32(value)   protobuf_zigzag_de<int32_t, uint32_t>(value)
#define protobuf_parse_sint64(value)   protobuf_zigzag_de<int64_t, uint64_t>(value)
#define protobuf_parse_bool(value)     static_cast<bool>(value)
#define protobuf_parse_enum(value)     static_cast<uint32_t>(value)

#define protobuf_parse_fixed32(value)  static_cast<uint32_t>(value)
#define protobuf_parse_fixed64(value)  static_cast<uint64_t>(value)
#define protobuf_parse_sfixed32(value) static_cast<uint32_t>(value)
#define protobuf_parse_sfixed64(value) static_cast<uint64_t>(value)
#define protobuf_parse_double(value)   (*(double*)&value)
#define protobuf_parse_float(value)    (*(float*)&value)

////////////////////////////////////////////////////////////////////////////////////////////////////////

#define protobuf_parse_varint_packed_impl(buf, len, cb, ...)        \
({                                                                  \
    const char* p = buf;                                            \
    const char* end = buf + len;                                    \
    uint64_t value = 0;                                             \
    bool result = true;                                             \
    do                                                              \
    {                                                               \
        p = protobuf_read_varint(p, end - p, &value);               \
        if ((p != NULL) && (cb(value, ##__VA_ARGS__) == false))     \
        {                                                           \
            result = false;                                         \
            break;                                                  \
        }                                                           \
    } while (end > p);                                              \
    result;                                                         \
})

#define protobuf_parse_fixed_packed_impl(type, buf, len, cb, ...)   \
({                                                                  \
    const char* p = buf;                                            \
    const char* end = buf + len;                                    \
    bool result = true;                                             \
    do                                                              \
    {                                                               \
        if (cb(*(type*)p, ##__VA_ARGS__) == false)                  \
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

