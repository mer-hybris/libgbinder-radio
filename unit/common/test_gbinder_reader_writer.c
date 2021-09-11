/*
 * Copyright (C) 2021 Jolla Ltd.
 * Copyright (C) 2021 Slava Monich <slava.monich@jolla.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#include "test_gbinder.h"

#include <gutil_macros.h>

typedef enum test_gbinder_data_type {
    DATA_TYPE_INT32,
    DATA_TYPE_BUFFER,
    DATA_TYPE_LOCAL_OBJ
} DATA_TYPE;

typedef struct test_gbinder_data_item TestGBinderDataItem;
struct test_gbinder_data_item {
    TestGBinderDataItem* next;
    DATA_TYPE type;
    union {
        gint32 i32;
        struct {
            const void* buf;
            gsize size;
        } blob;
        GBinderLocalObject* obj;
    } data;
    void (*destroy)(TestGBinderDataItem*);
};

struct test_gbinder_data {
    guint32 refcount;
    TestGBinderDataItem* items;
};

typedef struct test_gbinder_reader {
    TestGBinderDataItem* item;
} TestGBinderReader;

typedef struct test_gbinder_writer {
    TestGBinderData* data;
} TestGBinderWriter;

static inline TestGBinderReader* test_gbinder_reader_cast(GBinderReader* reader)
    { return (TestGBinderReader*)reader; }

static inline TestGBinderWriter* test_gbinder_writer_cast(GBinderWriter* writer)
    { return (TestGBinderWriter*)writer; }

static
void
test_gbinder_data_item_destroy_local_obj(
    TestGBinderDataItem* item)
{
    g_assert_cmpint(item->type, == ,DATA_TYPE_LOCAL_OBJ);
    gbinder_local_object_unref(item->data.obj);
}

static
TestGBinderDataItem*
test_gbinder_data_item_new(
    DATA_TYPE type)
{
    TestGBinderDataItem* item = g_new0(TestGBinderDataItem, 1);

    item->type = type;
    return item;
}

static
void
test_gbinder_data_item_free(
    TestGBinderDataItem* item)
{
    if (item) {
        test_gbinder_data_item_free(item->next);
        if (item->destroy) {
            item->destroy(item);
        }
        g_free(item);
    }
}

static
void
test_gbinder_data_free(
    TestGBinderData* data)
{
    test_gbinder_data_item_free(data->items);
    g_free(data);
}

static
guint
test_gbinder_data_count_buffers(
    TestGBinderData* data)
{
    TestGBinderDataItem* item;
    guint n;

    for (n = 0, item = data->items; item; item = item->next) {
        if (item->type == DATA_TYPE_BUFFER) {
            n++;
        }
    }
    return n;
}

static
void
test_gbinder_data_append(
    TestGBinderData* data,
    TestGBinderDataItem* item)
{
    TestGBinderDataItem* last = data->items;

    if (last) {
        while (last->next) {
            last = last->next;
        }
        last->next = item;
    } else {
        data->items = item;
    }
}

/*==========================================================================*
 * Internal API
 *==========================================================================*/

TestGBinderData*
test_gbinder_data_new(
    void)
{
    TestGBinderData* data = g_new0(TestGBinderData, 1);

    g_atomic_int_set(&data->refcount, 1);
    return data;
}

TestGBinderData*
test_gbinder_data_ref(
    TestGBinderData* data)
{
    if (data) {
        g_assert_cmpint(data->refcount, > ,0);
        g_atomic_int_inc(&data->refcount);
    }
    return data;
}

void
test_gbinder_data_unref(
    TestGBinderData* data)
{
    if (data) {
        g_assert_cmpint(data->refcount, > ,0);
        if (g_atomic_int_dec_and_test(&data->refcount)) {
            test_gbinder_data_free(data);
        }
    }
}

void
test_gbinder_data_init_reader(
    TestGBinderData* data,
    GBinderReader* reader)
{
    memset(reader, 0, sizeof(*reader));
    if (data) {
        test_gbinder_reader_cast(reader)->item = data->items;
    }
}

void
test_gbinder_data_init_writer(
    TestGBinderData* data,
    GBinderWriter* writer)
{
    memset(writer, 0, sizeof(*writer));
    if (data) {
         test_gbinder_writer_cast(writer)->data = data;
    }
}

/*==========================================================================*
 * libgbinder API
 *==========================================================================*/

gboolean
gbinder_reader_read_uint32(
    GBinderReader* reader,
    guint32* value)
{
    TestGBinderReader* self = test_gbinder_reader_cast(reader);
    TestGBinderDataItem* item = self->item;

    if (item && item->type == DATA_TYPE_INT32) {
        if (value) {
            *value = item->data.i32;
        }
        self->item = item->next;
        return TRUE;
    }
    return FALSE;
}

gboolean
gbinder_reader_read_int32(
    GBinderReader* reader,
    gint32* value)
{
    return gbinder_reader_read_uint32(reader, (guint32*)value);
}

const void*
gbinder_reader_read_hidl_struct1(
    GBinderReader* reader,
    gsize size)
{
    TestGBinderReader* self = test_gbinder_reader_cast(reader);
    TestGBinderDataItem* item = self->item;

    if (item && item->type == DATA_TYPE_BUFFER &&
        item->data.blob.size == size) {
        self->item = item->next;
        return item->data.blob.buf;
    }
    return NULL;
}

GBinderRemoteObject*
gbinder_reader_read_object(
    GBinderReader* reader)
{
    TestGBinderReader* self = test_gbinder_reader_cast(reader);
    TestGBinderDataItem* item = self->item;

    if (item && item->type == DATA_TYPE_LOCAL_OBJ) {
        self->item = item->next;
        return test_gbinder_remote_object_new(item->data.obj);
    }
    return NULL;
}

void
gbinder_writer_append_int32(
    GBinderWriter* writer,
    guint32 value)
{
    TestGBinderWriter* self = test_gbinder_writer_cast(writer);
    TestGBinderDataItem* item = test_gbinder_data_item_new(DATA_TYPE_INT32);

    item->data.i32 = value;
    test_gbinder_data_append(self->data, item);
}

guint
gbinder_writer_append_buffer_object(
    GBinderWriter* writer,
    const void* buf,
    gsize size)
{
    TestGBinderWriter* self = test_gbinder_writer_cast(writer);
    TestGBinderDataItem* item = test_gbinder_data_item_new(DATA_TYPE_BUFFER);
    const guint index = test_gbinder_data_count_buffers(self->data);

    item->data.blob.buf = buf;
    item->data.blob.size = size;
    test_gbinder_data_append(self->data, item);
    return index;
}

void
gbinder_writer_append_local_object(
    GBinderWriter* writer,
    GBinderLocalObject* obj)
{
    TestGBinderWriter* self = test_gbinder_writer_cast(writer);
    TestGBinderDataItem* item = test_gbinder_data_item_new(DATA_TYPE_LOCAL_OBJ);

    item->data.obj = gbinder_local_object_ref(obj);
    item->destroy = test_gbinder_data_item_destroy_local_obj;
    test_gbinder_data_append(self->data, item);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
