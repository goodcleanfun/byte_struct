#ifndef BYTE_STRUCT_H
#define BYTE_STRUCT_H

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "lex_order/lex_order.h"

static const char BYTE_STRUCT_FORMAT_CHAR = 'c';
static const char BYTE_STRUCT_FORMAT_INT8 = 'b';
static const char BYTE_STRUCT_FORMAT_UINT8 = 'B';
static const char BYTE_STRUCT_FORMAT_INT16 = 'h';
static const char BYTE_STRUCT_FORMAT_UINT16 = 'H';
static const char BYTE_STRUCT_FORMAT_INT32 = 'i';
static const char BYTE_STRUCT_FORMAT_UINT32 = 'I';
static const char BYTE_STRUCT_FORMAT_INT64 = 'l';
static const char BYTE_STRUCT_FORMAT_UINT64 = 'L';
static const char BYTE_STRUCT_FORMAT_FLOAT = 'f';
static const char BYTE_STRUCT_FORMAT_DOUBLE = 'd';
static const char BYTE_STRUCT_FORMAT_PTR = 'p';

typedef enum {
    BYTE_STRUCT_TYPE_CHAR,
    BYTE_STRUCT_TYPE_INT8,
    BYTE_STRUCT_TYPE_UINT8,
    BYTE_STRUCT_TYPE_INT16,
    BYTE_STRUCT_TYPE_UINT16,
    BYTE_STRUCT_TYPE_INT32,
    BYTE_STRUCT_TYPE_UINT32,
    BYTE_STRUCT_TYPE_INT64,
    BYTE_STRUCT_TYPE_UINT64,
    BYTE_STRUCT_TYPE_FLOAT,
    BYTE_STRUCT_TYPE_DOUBLE,
    BYTE_STRUCT_TYPE_PTR
} byte_struct_type_t;

typedef enum {
    BYTE_STRUCT_BIG_ENDIAN,
    BYTE_STRUCT_LITTLE_ENDIAN,
    BYTE_STRUCT_NATIVE_ENDIAN,
    BYTE_STRUCT_SORTABLE
} byte_order_t;

typedef struct type_offset {
    size_t offset;
    size_t count;
    byte_struct_type_t type;
} type_offset_t;

typedef struct byte_struct {
    byte_order_t byte_order;
    size_t num_fields;
    size_t total_size;
    type_offset_t type_offsets[];
} byte_struct_t;

static bool byte_struct_type_and_size(char c, byte_struct_type_t *type, size_t *size) {
    switch (c) {
        case BYTE_STRUCT_FORMAT_CHAR:
            *size = sizeof(char);
            *type = BYTE_STRUCT_TYPE_CHAR;
            return true;
        case BYTE_STRUCT_FORMAT_INT8:
            *size =  sizeof(int8_t);
            *type = BYTE_STRUCT_TYPE_INT8;
            return true;
        case BYTE_STRUCT_FORMAT_UINT8:
            *size = sizeof(uint8_t);
            *type = BYTE_STRUCT_TYPE_UINT8;
            return true;
        case BYTE_STRUCT_FORMAT_INT16:
            *size = sizeof(int16_t);
            *type = BYTE_STRUCT_TYPE_INT16;
        case BYTE_STRUCT_FORMAT_UINT16:
            *size = sizeof(uint16_t);
            *type = BYTE_STRUCT_TYPE_UINT16;
            return true;
        case BYTE_STRUCT_FORMAT_INT32:
            *size = sizeof(int32_t);
            *type = BYTE_STRUCT_TYPE_INT32;
            return true;
        case BYTE_STRUCT_FORMAT_UINT32:
            *size = sizeof(uint32_t);
            *type = BYTE_STRUCT_TYPE_UINT32;
            return true;
        case BYTE_STRUCT_FORMAT_INT64:
            *size = sizeof(int64_t);
            *type = BYTE_STRUCT_TYPE_INT64;
            return true;
        case BYTE_STRUCT_FORMAT_UINT64:
            *size = sizeof(uint64_t);
            *type = BYTE_STRUCT_TYPE_UINT64;
            return true;
        case BYTE_STRUCT_FORMAT_FLOAT:
            *size = sizeof(float);
            *type = BYTE_STRUCT_TYPE_FLOAT;
            return true;
        case BYTE_STRUCT_FORMAT_DOUBLE:
            *size = sizeof(double);
            *type = BYTE_STRUCT_TYPE_DOUBLE;
            return true;
        case BYTE_STRUCT_FORMAT_PTR:
            *size = sizeof(void *);
            *type = BYTE_STRUCT_TYPE_PTR;
            return true;
        default:
            return false;
    }
}

static byte_struct_t *byte_struct_new_len_options(const char *format, size_t len, byte_order_t byte_order) {
    if (format == NULL || len == 0) return NULL;

    size_t num_fields = 0;
    size_t i = 0, j = 0;

    bool prev_was_type = false;

    for (i = 0; i < len; i++) {
        if (format[i] == '[') {
            if (!prev_was_type) {
                return NULL;
            }
            j = i + 1;
            while (j < len && format[j] != ']') {
                if (format[j] < '0' || format[j] > '9') {
                    return NULL;
                }
                j++;
            }
            if (j < i + 2 || j == len) {
                return NULL;
            }
            i = j;
            prev_was_type = false;
        } else {
            char c = format[i];
            if ((c == BYTE_STRUCT_FORMAT_CHAR) ||
                (c == BYTE_STRUCT_FORMAT_INT8) ||
                (c == BYTE_STRUCT_FORMAT_UINT8) ||
                (c == BYTE_STRUCT_FORMAT_INT16) ||
                (c == BYTE_STRUCT_FORMAT_UINT16) ||
                (c == BYTE_STRUCT_FORMAT_INT32) ||
                (c == BYTE_STRUCT_FORMAT_UINT32) ||
                (c == BYTE_STRUCT_FORMAT_INT64) ||
                (c == BYTE_STRUCT_FORMAT_UINT64) ||
                (c == BYTE_STRUCT_FORMAT_FLOAT) ||
                (c == BYTE_STRUCT_FORMAT_DOUBLE) ||
                (c == BYTE_STRUCT_FORMAT_PTR)) {
                    num_fields++;
            } else {
                return NULL;
            }
            prev_was_type = true;
        }
    }

    byte_struct_t *s = malloc(sizeof(byte_struct_t) + num_fields * sizeof(type_offset_t));
    if (s == NULL) return NULL;
    s->num_fields = num_fields;
    s->byte_order = byte_order;
    size_t total_size = 0;
    size_t prev_size = 0;

    size_t current_field = 0;

    size_t current_size = 0;
    const size_t max_array_div_10 = SIZE_MAX / 10;
    const size_t max_array_mod_10 = SIZE_MAX % 10;

    i = 0;
    char format_type = '\0';
    byte_struct_type_t type;

    size_t count = 1;

    while (i < len) {
        if (format[i] == '[') {
            count = 0;
            size_t j = i + 1;
            while (j < len && format[j] != ']') {
                uint8_t digit = format[j] - '0';
                if (count > max_array_div_10 || (count == max_array_div_10 && digit > max_array_mod_10)) {
                    free(s);
                    return NULL;
                }
                count = (count * 10) + (size_t)digit;
                j++;
            }

            if ((SIZE_MAX - total_size) / prev_size < (count - 1)) {
                free(s);
                return NULL;
            }
            // We validated above that the [ is not the first character.
            // Use count - 1 since we already accounted for the first element
            total_size += (prev_size * (count - 1));
            if (j == len) {
                // closing brace at the end of the format string
                s->type_offsets[current_field++] = (type_offset_t){.offset = total_size, .type = type, .count = count};
            // we know the current_field is at least 1 from validation above
            } else {
                s->type_offsets[current_field - 1].count = count;
            }
            i = j + 1;
        } else {
            format_type = format[i];
            size_t type_size = 0;
            if (!byte_struct_type_and_size(format_type, &type, &type_size)) {
                free(s);
                return NULL;
            }
            if (SIZE_MAX - total_size < type_size) {
                free(s);
                return NULL;
            }
            type_offset_t type_offset = {.offset = total_size, .type = type, .count = 1};
            s->type_offsets[current_field++] = type_offset;
            i++;
            count = 1;
            prev_size = type_size;
            total_size += type_size;
        }
    }
    s->total_size = total_size;
    return s;
}


static void byte_struct_pack_int8(byte_struct_t *s, uint8_t *data, int8_t value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_int8(data, value);
    } else {
        data[0] = value;
    }
}

static void byte_struct_pack_int8_array(byte_struct_t *s, uint8_t *data, int8_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_int8(data + i, values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(int8_t));
    }
}

static void byte_struct_pack_uint8(byte_struct_t *s, uint8_t *data, uint8_t value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_int8(data, value);
    } else {
        data[0] = value;
    }
}

static void byte_struct_pack_uint8_array(byte_struct_t *s, uint8_t *data, uint8_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_int8(data + i, values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(uint8_t));
    }
}

static void byte_struct_pack_int16(byte_struct_t *s, uint8_t *data, int16_t value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_int16(data, value);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        write_uint16_big_endian(data, value);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        write_uint16_little_endian(data, value);
    } else {
        memcpy(data, &value, sizeof(int16_t));
    }
}

static void byte_struct_pack_int16_array(byte_struct_t *s, uint8_t *data, int16_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_int16(data + i * sizeof(int16_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint16_big_endian(data + i * sizeof(int16_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint16_little_endian(data + i * sizeof(int16_t), values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(int16_t));
    }
}

static void byte_struct_pack_uint16(byte_struct_t *s, uint8_t *data, uint16_t value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_uint16(data, value);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        write_uint16_big_endian(data, value);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        write_uint16_little_endian(data, value);
    } else {
        memcpy(data, &value, sizeof(uint16_t));
    }
}

static void byte_struct_pack_uint16_array(byte_struct_t *s, uint8_t *data, uint16_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_uint16(data + i * sizeof(uint16_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint16_big_endian(data + i * sizeof(uint16_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint16_little_endian(data + i * sizeof(uint16_t), values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(uint16_t));
    }
}

static void byte_struct_pack_int32(byte_struct_t *s, uint8_t *data, int32_t value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_int32(data, value);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        write_uint32_big_endian(data, value);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        write_uint32_little_endian(data, value);
    } else {
        memcpy(data, &value, sizeof(int32_t));
    }
}

static void byte_struct_pack_int32_array(byte_struct_t *s, uint8_t *data, int32_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_int32(data + i * sizeof(int32_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint32_big_endian(data + i * sizeof(int32_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint32_little_endian(data + i * sizeof(int32_t), values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(int32_t));
    }
}

static void byte_struct_pack_uint32(byte_struct_t *s, uint8_t *data, uint32_t value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_uint32(data, value);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        write_uint32_big_endian(data, value);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        write_uint32_little_endian(data, value);
    } else {
        memcpy(data, &value, sizeof(uint32_t));
    }
}

static void byte_struct_pack_uint32_array(byte_struct_t *s, uint8_t *data, uint32_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_uint32(data + i * sizeof(uint32_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint32_big_endian(data + i * sizeof(uint32_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint32_little_endian(data + i * sizeof(uint32_t), values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(uint32_t));
    }
}


static void byte_struct_pack_float(byte_struct_t *s, uint8_t *data, float value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_float(data, value);
    } else {
        memcpy(data, &value, sizeof(float));
    }
}

static void byte_struct_pack_float_array(byte_struct_t *s, uint8_t *data, float *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_float(data + i * sizeof(float), values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(float));
    }
}

static void byte_struct_pack_int64(byte_struct_t *s, uint8_t *data, int64_t value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_int64(data, value);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        write_uint64_big_endian(data, value);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        write_uint64_little_endian(data, value);
    } else {
        memcpy(data, &value, sizeof(int64_t));
    }
}

static void byte_struct_pack_int64_array(byte_struct_t *s, uint8_t *data, int64_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_int64(data + i * sizeof(int64_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint64_big_endian(data + i * sizeof(int64_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint64_little_endian(data + i * sizeof(int64_t), values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(int64_t));
    }
}

static void byte_struct_pack_uint64(byte_struct_t *s, uint8_t *data, uint64_t value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_uint64(data, value);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        write_uint64_big_endian(data, value);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        write_uint64_little_endian(data, value);
    } else {
        memcpy(data, &value, sizeof(uint64_t));
    }
}

static void byte_struct_pack_uint64_array(byte_struct_t *s, uint8_t *data, uint64_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_uint64(data + i * sizeof(uint64_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint64_big_endian(data + i * sizeof(uint64_t), values[i]);
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            write_uint64_little_endian(data + i * sizeof(uint64_t), values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(uint64_t));
    }
}

#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
static void byte_struct_pack_ptr(byte_struct_t *s, uint8_t *data, void *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_uint64(data, (uint64_t)(uintptr_t)value);
    } else {
        memcpy(data, &value, sizeof(void *));
    }
}
static void byte_struct_pack_ptr_array(byte_struct_t *s, uint8_t *data, void **values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_uint64(data + i * sizeof(void *), (uint64_t)(uintptr_t)values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(void *));
    }
}
static void byte_struct_unpack_ptr(byte_struct_t *s, uint8_t *data, void **value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = (void *)(uintptr_t)lex_ordered_read_uint64(data);
    } else {
        memcpy(value, data, sizeof(void *));
    }
}
static void byte_struct_unpack_ptr_array(byte_struct_t *s, uint8_t *data, void ***values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            (*values)[i] = (void *)(uintptr_t)lex_ordered_read_uint64(data + i * sizeof(void *));
        }
    } else {
        memcpy(values, data, n * sizeof(void **));
    }
}
#elif UINTPTR_MAX == 0xFFFFFFFF
static void byte_struct_pack_ptr(byte_struct_t *s, uint8_t *data, void *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_uint32(data, (uint32_t)(uintptr_t)value);
    } else {
        memcpy(data, &value, sizeof(void *));
    }
}
static void byte_struct_pack_ptr_array(byte_struct_t *s, uint8_t *data, void **values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_uint32(data + i * sizeof(void *), (uint32_t)(uintptr_t)values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(void *));
    }
}
static void byte_struct_unpack_ptr(byte_struct_t *s, uint8_t *data, void **value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = (void *)(uintptr_t)lex_ordered_read_uint32(data);
    } else {
        memcpy(value, data, sizeof(void *));
    }
}
static void byte_struct_unpack_ptr_array(byte_struct_t *s, uint8_t *data, void ***values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            (*values)[i] = (void *)(uintptr_t)lex_ordered_read_uint32(data + i * sizeof(void *));
        }
    } else {
        memcpy(values, data, n * sizeof(void **));
    }
}
#else
#error "Unsupported pointer size"
#endif

static void byte_struct_pack_double(byte_struct_t *s, uint8_t *data, double value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        lex_ordered_write_double(data, value);
    } else {
        memcpy(data, &value, sizeof(double));
    }
}

static void byte_struct_pack_double_array(byte_struct_t *s, uint8_t *data, double *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            lex_ordered_write_double(data + i * sizeof(double), values[i]);
        }
    } else {
        memcpy(data, values, n * sizeof(double));
    }
}

bool byte_struct_pack(byte_struct_t *s, uint8_t *data, ...) {
    if (s == NULL || s->num_fields == 0) return false;
    if (data == NULL) return false;
    va_list args;
    va_start(args, data);
    for (size_t i = 0; i < s->num_fields; i++) {
        type_offset_t type_offset = s->type_offsets[i];
        switch (type_offset.type) {
            case BYTE_STRUCT_TYPE_CHAR:
                if (type_offset.count == 1) {
                    char value = (char)va_arg(args, int);
                    memcpy(data + type_offset.offset, &value, sizeof(char));
                } else {
                    char *value = va_arg(args, char *);
                    memcpy(data + type_offset.offset, value, type_offset.count * sizeof(char));
                }
                break;
            case BYTE_STRUCT_TYPE_INT8:
                if (type_offset.count == 1) {
                    int8_t value = (int8_t)va_arg(args, int);
                    byte_struct_pack_int8(s, data + type_offset.offset, value);
                } else {
                    int8_t *value = va_arg(args, int8_t *);
                    byte_struct_pack_int8_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_UINT8:
                if (type_offset.count == 1) {
                    uint8_t value = (uint8_t)va_arg(args, int);
                    byte_struct_pack_uint8(s, data + type_offset.offset, value);
                } else {
                    uint8_t *value = va_arg(args, uint8_t *);
                    byte_struct_pack_uint8_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_INT16:
                if (type_offset.count == 1) {
                    int16_t value = (int16_t)va_arg(args, int);
                    byte_struct_pack_int16(s, data + type_offset.offset, value);
                } else {
                    int16_t *value = va_arg(args, int16_t *);
                    byte_struct_pack_int16_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_UINT16:
                if (type_offset.count == 1) {
                    uint16_t value = (uint16_t)va_arg(args, int);
                    byte_struct_pack_uint16(s, data + type_offset.offset, value);
                } else {
                    uint16_t *value = va_arg(args, uint16_t *);
                    byte_struct_pack_uint16_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_INT32:
                if (type_offset.count == 1) {
                    int32_t value = (int32_t)va_arg(args, int);
                    byte_struct_pack_int32(s, data + type_offset.offset, value);
                } else {
                    int32_t *value = va_arg(args, int32_t *);
                    byte_struct_pack_int32_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_UINT32:
                if (type_offset.count == 1) {
                    uint32_t value = (uint32_t)va_arg(args, int);
                    byte_struct_pack_uint32(s, data + type_offset.offset, value);
                } else {
                    uint32_t *value = va_arg(args, uint32_t *);
                    byte_struct_pack_uint32_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_INT64:
                if (type_offset.count == 1) {
                    int64_t value = va_arg(args, int64_t);
                    byte_struct_pack_int64(s, data + type_offset.offset, value);
                } else {
                    int64_t *value = va_arg(args, int64_t *);
                    byte_struct_pack_int64_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_UINT64:
                if (type_offset.count == 1) {
                    uint64_t value = va_arg(args, uint64_t);
                    byte_struct_pack_uint64(s, data + type_offset.offset, value);
                } else {
                    uint64_t *value = va_arg(args, uint64_t *);
                    byte_struct_pack_uint64_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_FLOAT:
                if (type_offset.count == 1) {
                    float value = (float)va_arg(args, double);
                    byte_struct_pack_float(s, data + type_offset.offset, value);
                } else {
                    float *value = va_arg(args, float *);
                    byte_struct_pack_float_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_DOUBLE:
                if (type_offset.count == 1) {
                    double value = va_arg(args, double);
                    byte_struct_pack_double(s, data + type_offset.offset, value);
                } else {
                    double *value = va_arg(args, double *);
                    byte_struct_pack_double_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_PTR:
                if (type_offset.count == 1) {
                    void *value = va_arg(args, void *);
                    byte_struct_pack_ptr(s, data + type_offset.offset, value);
                } else {
                    void **value = va_arg(args, void **);
                    byte_struct_pack_ptr_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
        }
    }
    va_end(args);
    return true;
}


static void byte_struct_unpack_int8(byte_struct_t *s, uint8_t *data, int8_t *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_int8(data);
    } else {
        memcpy(value, data, sizeof(int8_t));
    }
}

static void byte_struct_unpack_int8_array(byte_struct_t *s, uint8_t *data, int8_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_int8(data + i);
        }
    } else {
        memcpy(values, data, n * sizeof(int8_t));
    }
}

static void byte_struct_unpack_uint8(byte_struct_t *s, uint8_t *data, uint8_t *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_int8(data);
    } else {
        memcpy(value, data, sizeof(uint8_t));
    }
}

static void byte_struct_unpack_uint8_array(byte_struct_t *s, uint8_t *data, uint8_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_int8(data + i);
        }
    } else {
        memcpy(values, data, n * sizeof(uint8_t));
    }
}

static void byte_struct_unpack_int16(byte_struct_t *s, uint8_t *data, int16_t *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_int16(data);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        *value = read_uint16_big_endian(data);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        *value = read_uint16_little_endian(data);
    } else {
        memcpy(value, data, sizeof(int16_t));
    }
}

static void byte_struct_unpack_int16_array(byte_struct_t *s, uint8_t *data, int16_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_int16(data + i * sizeof(int16_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint16_big_endian(data + i * sizeof(int16_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint16_little_endian(data + i * sizeof(int16_t));
        }
    } else {
        memcpy(values, data, n * sizeof(int16_t));
    }
}

static void byte_struct_unpack_uint16(byte_struct_t *s, uint8_t *data, uint16_t *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_uint16(data);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        *value = read_uint16_big_endian(data);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        *value = read_uint16_little_endian(data);
    } else {
        memcpy(value, data, sizeof(uint16_t));
    }
}

static void byte_struct_unpack_uint16_array(byte_struct_t *s, uint8_t *data, uint16_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_uint16(data + i * sizeof(uint16_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint16_big_endian(data + i * sizeof(uint16_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint16_little_endian(data + i * sizeof(uint16_t));
        }
    } else {
        memcpy(values, data, n * sizeof(uint16_t));
    }
}

static void byte_struct_unpack_int32(byte_struct_t *s, uint8_t *data, int32_t *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_int32(data);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        *value = read_uint32_big_endian(data);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        *value = read_uint32_little_endian(data);
    } else {
        memcpy(value, data, sizeof(int32_t));
    }
}

static void byte_struct_unpack_int32_array(byte_struct_t *s, uint8_t *data, int32_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_int32(data + i * sizeof(int32_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint32_big_endian(data + i * sizeof(int32_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint32_little_endian(data + i * sizeof(int32_t));
        }
    } else {
        memcpy(values, data, n * sizeof(int32_t));
    }
}

static void byte_struct_unpack_uint32(byte_struct_t *s, uint8_t *data, uint32_t *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_uint32(data);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        *value = read_uint32_big_endian(data);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        *value = read_uint32_little_endian(data);
    } else {
        memcpy(value, data, sizeof(uint32_t));
    }
}

static void byte_struct_unpack_uint32_array(byte_struct_t *s, uint8_t *data, uint32_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_uint32(data + i * sizeof(uint32_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint32_big_endian(data + i * sizeof(uint32_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint32_little_endian(data + i * sizeof(uint32_t));
        }
    } else {
        memcpy(values, data, n * sizeof(uint32_t));
    }
}

static void byte_struct_unpack_int64(byte_struct_t *s, uint8_t *data, int64_t *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_int64(data);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        *value = read_uint64_big_endian(data);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        *value = read_uint64_little_endian(data);
    } else {
        memcpy(value, data, sizeof(int64_t));
    }
}

static void byte_struct_unpack_int64_array(byte_struct_t *s, uint8_t *data, int64_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_int64(data + i * sizeof(int64_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint64_big_endian(data + i * sizeof(int64_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint64_little_endian(data + i * sizeof(int64_t));
        }
    } else {
        memcpy(values, data, n * sizeof(int64_t));
    }
}

static void byte_struct_unpack_uint64(byte_struct_t *s, uint8_t *data, uint64_t *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_uint64(data);
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        *value = read_uint64_big_endian(data);
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        *value = read_uint64_little_endian(data);
    } else {
        memcpy(value, data, sizeof(uint64_t));
    }
}

static void byte_struct_unpack_uint64_array(byte_struct_t *s, uint8_t *data, uint64_t *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_uint64(data + i * sizeof(uint64_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_BIG_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint64_big_endian(data + i * sizeof(uint64_t));
        }
    } else if (s->byte_order == BYTE_STRUCT_LITTLE_ENDIAN) {
        for (size_t i = 0; i < n; i++) {
            values[i] = read_uint64_little_endian(data + i * sizeof(uint64_t));
        }
    } else {
        memcpy(values, data, n * sizeof(uint64_t));
    }
}


static void byte_struct_unpack_float(byte_struct_t *s, uint8_t *data, float *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_float(data);
    } else {
        memcpy(value, data, sizeof(float));
    }
}

static void byte_struct_unpack_float_array(byte_struct_t *s, uint8_t *data, float *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_float(data + i * sizeof(float));
        }
    } else {
        memcpy(values, data, n * sizeof(float));
    }
}

static void byte_struct_unpack_double(byte_struct_t *s, uint8_t *data, double *value) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        *value = lex_ordered_read_double(data);
    } else {
        memcpy(value, data, sizeof(double));
    }
}

static void byte_struct_unpack_double_array(byte_struct_t *s, uint8_t *data, double *values, size_t n) {
    if (s->byte_order == BYTE_STRUCT_SORTABLE) {
        for (size_t i = 0; i < n; i++) {
            values[i] = lex_ordered_read_double(data + i * sizeof(double));
        }
    } else {
        memcpy(values, data, n * sizeof(double));
    }
}

bool byte_struct_unpack(byte_struct_t *s, uint8_t *data, size_t data_len, ...) {
    if (s == NULL || data == NULL || data_len < s->total_size || s->num_fields == 0) return false;
    va_list args;
    va_start(args, data_len);
    for (size_t i = 0; i < s->num_fields; i++) {
        type_offset_t type_offset = s->type_offsets[i];
        switch(type_offset.type) {
            case BYTE_STRUCT_TYPE_CHAR:
                if (type_offset.count == 1) {
                    char value = va_arg(args, int);
                    memcpy(&value, data + type_offset.offset, sizeof(char));
                } else {
                    char *value = va_arg(args, char *);
                    memcpy(value, data + type_offset.offset, type_offset.count * sizeof(char));
                }
                break;
            case BYTE_STRUCT_TYPE_INT8:
                if (type_offset.count == 1) {
                    int8_t *value = va_arg(args, int8_t *);
                    byte_struct_unpack_int8(s, data + type_offset.offset, value);
                } else {
                    int8_t *value = va_arg(args, int8_t *);
                    byte_struct_unpack_int8_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_UINT8:
                if (type_offset.count == 1) {
                    uint8_t *value = va_arg(args, uint8_t *);
                    byte_struct_unpack_uint8(s, data + type_offset.offset, value);
                } else {
                    uint8_t *value = va_arg(args, uint8_t *);
                    byte_struct_unpack_uint8_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_INT16:
                if (type_offset.count == 1) {
                    int16_t *value = va_arg(args, int16_t *);
                    byte_struct_unpack_int16(s, data + type_offset.offset, value);
                } else {
                    int16_t *value = va_arg(args, int16_t *);
                    byte_struct_unpack_int16_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_UINT16:
                if (type_offset.count == 1) {
                    uint16_t *value = va_arg(args, uint16_t *);
                    byte_struct_unpack_uint16(s, data + type_offset.offset, value);
                } else {
                    uint16_t *value = va_arg(args, uint16_t *);
                    byte_struct_unpack_uint16_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_INT32:
                if (type_offset.count == 1) {
                    int32_t *value = va_arg(args, int32_t *);
                    byte_struct_unpack_int32(s, data + type_offset.offset, value);
                } else {
                    int32_t *value = va_arg(args, int32_t *);
                    byte_struct_unpack_int32_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_UINT32:
                if (type_offset.count == 1) {
                    uint32_t *value = va_arg(args, uint32_t *);
                    byte_struct_unpack_uint32(s, data + type_offset.offset, value);
                } else {
                    uint32_t *value = va_arg(args, uint32_t *);
                    byte_struct_unpack_uint32_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_INT64:
                if (type_offset.count == 1) {
                    int64_t *value = va_arg(args, int64_t *);
                    byte_struct_unpack_int64(s, data + type_offset.offset, value);
                } else {
                    int64_t *value = va_arg(args, int64_t *);
                    byte_struct_unpack_int64_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_UINT64:
                if (type_offset.count == 1) {
                    uint64_t *value = va_arg(args, uint64_t *);
                    byte_struct_unpack_uint64(s, data + type_offset.offset, value);
                } else {
                    uint64_t *value = va_arg(args, uint64_t *);
                    byte_struct_unpack_uint64_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_FLOAT:
                if (type_offset.count == 1) {
                    float *value = va_arg(args, float *);
                    byte_struct_unpack_float(s, data + type_offset.offset, value);
                } else {
                    float *value = va_arg(args, float *);
                    byte_struct_unpack_float_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_DOUBLE:
                if (type_offset.count == 1) {
                    double *value = va_arg(args, double *);
                    byte_struct_unpack_double(s, data + type_offset.offset, value);
                } else {
                    double *value = va_arg(args, double *);
                    byte_struct_unpack_double_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
            case BYTE_STRUCT_TYPE_PTR:
                if (type_offset.count == 1) {
                    void **value = va_arg(args, void **);
                    byte_struct_unpack_ptr(s, data + type_offset.offset, value);
                } else {
                    void ***value = va_arg(args, void ***);
                    byte_struct_unpack_ptr_array(s, data + type_offset.offset, value, type_offset.count);
                }
                break;
        }
    }

    va_end(args);
    return true;
}


static byte_struct_t *byte_struct_new(const char *format) {
    return byte_struct_new_len_options(format, strlen(format), BYTE_STRUCT_BIG_ENDIAN);
}

void byte_struct_destroy(byte_struct_t *s) {
    if (s == NULL) return;
    free(s);
}

#endif