#include <stdint.h>
#include <float.h>
#include "greatest/greatest.h"

#include "byte_struct.h"

TEST test_byte_struct(void) {
    byte_struct_t *s = byte_struct_new("bI[4]f");
    ASSERT_EQ(s->num_fields, 3);
    ASSERT_EQ(s->total_size, sizeof(int8_t) + sizeof(uint32_t) * 4 + sizeof(float));
    ASSERT_EQ(s->type_offsets[0].offset, 0);
    ASSERT_EQ(s->type_offsets[0].type, BYTE_STRUCT_TYPE_INT8);
    ASSERT_EQ(s->type_offsets[0].count, 1);
    ASSERT_EQ(s->type_offsets[1].offset, sizeof(int8_t));
    ASSERT_EQ(s->type_offsets[1].count, 4);
    ASSERT_EQ(s->type_offsets[1].type, BYTE_STRUCT_TYPE_UINT32);
    ASSERT_EQ(s->type_offsets[2].offset, sizeof(int8_t) + sizeof(uint32_t) * 4);
    ASSERT_EQ(s->type_offsets[2].type, BYTE_STRUCT_TYPE_FLOAT);
    ASSERT_EQ(s->type_offsets[2].count, 1);

    uint8_t *data = malloc(s->total_size);
    ASSERT_NEQ(data, NULL);
    bool success = byte_struct_pack(s, data, (int8_t)1, (uint32_t[]){2, 3, 4, 5}, (float)6.0f);
    ASSERT(success);

    int8_t b = 0;
    uint32_t au[4] = {0};
    float n = 0.0f;
    success = byte_struct_unpack(s, data, s->total_size, &b, &au, &n);
    ASSERT(success);

    struct value_struct {
        int8_t b;
        uint32_t au[4];
        float n;
    };

    struct value_struct v;
    memset(&v, 0, sizeof(struct value_struct));
    success = byte_struct_unpack(s, data, s->total_size, &v.b, &v.au, &v.n);
    ASSERT(success);

    ASSERT_EQ(v.b, 1);
    ASSERT_EQ(v.au[0], 2);
    ASSERT_EQ(v.au[1], 3);
    ASSERT_EQ(v.au[2], 4);
    ASSERT_EQ(v.au[3], 5);
    ASSERT_IN_RANGE(v.n, 6.0f, FLT_EPSILON);

    free(data);

    ASSERT_EQ(v.b, 1);
    ASSERT_EQ(v.au[0], 2);
    ASSERT_EQ(v.au[1], 3);
    ASSERT_EQ(v.au[2], 4);
    ASSERT_EQ(v.au[3], 5);
    ASSERT_IN_RANGE(v.n, 6.0f, FLT_EPSILON);

    byte_struct_t *f = byte_struct_new("bI[-4]f");
    ASSERT_EQ(f, NULL);

    size_t max_size = SIZE_MAX;
    char format[50] = "";
    sprintf(format, "bI[%zu]f", max_size);
    f = byte_struct_new(format);
    ASSERT_EQ(f, NULL);

    max_size--;
    sprintf(format, "bI[%zu]f", max_size);
    f = byte_struct_new(format);
    ASSERT_EQ(f, NULL);

    max_size = (SIZE_MAX / sizeof(uint32_t)) + 1;
    sprintf(format, "bI[%zu]f", max_size);
    f = byte_struct_new(format);
    ASSERT_EQ(f, NULL);

    max_size--;
    sprintf(format, "bI[%zu]f", max_size);
    f = byte_struct_new(format);
    ASSERT_EQ(f, NULL);


    byte_struct_t *p = byte_struct_new("d[2]");
    ASSERT_NEQ(p, NULL);

    uint8_t *pdata = malloc(p->total_size);
    success = byte_struct_pack(p, pdata, (double[]){1.0, 2.0});
    ASSERT(success);
    double pd[2] = {0};
    success = byte_struct_unpack(p, pdata, p->total_size, &pd);
    ASSERT(success);
    ASSERT_IN_RANGE(pd[0], 1.0, DBL_EPSILON);
    ASSERT_IN_RANGE(pd[1], 2.0, DBL_EPSILON);
    free(pdata);

    uint8_t *p2_data = malloc(p->total_size * 2);
    ASSERT_NEQ(p2_data, NULL);
    uint8_t *p1 = p2_data;
    uint8_t *p2 = p1 + p->total_size;

    success = byte_struct_pack(p, p1, (double[]){-1.0, 2.0});
    ASSERT(success);

    success = byte_struct_pack(p, p2, (double[]){-2.0, 3.0});
    ASSERT(success);

    ASSERT(memcmp(p2, p1, p->total_size) < 0);

    success = byte_struct_pack(p, p2, (double[]){-1.0, 4.0});

    ASSERT(success);
    ASSERT(memcmp(p2, p1, p->total_size) > 0);

    free(p2_data);

    byte_struct_destroy(s);
    byte_struct_destroy(f);
    byte_struct_destroy(p);
    PASS();
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int32_t main(int32_t argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line options, initialization. */

    RUN_TEST(test_byte_struct);

    GREATEST_MAIN_END();        /* display results */
}
