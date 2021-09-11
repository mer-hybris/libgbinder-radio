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

#include "test_common.h"

#include "radio_instance.h"
#include "radio_registry_p.h"

#include <glib-object.h>

static TestOpt test_opt;

/*==========================================================================*
 * null
 *==========================================================================*/

static
void
test_null(
    void)
{
    radio_registry_instance_added(NULL);
    radio_registry_instance_removed(NULL);
    radio_registry_remove_handler(NULL, 0);
    radio_registry_remove_handlers(NULL, NULL, 0);
    radio_registry_unref(NULL);
    g_assert(!radio_registry_ref(NULL));
    g_assert(!radio_registry_add_instance_added_handler(NULL,NULL,NULL,NULL));
    g_assert(!radio_registry_add_instance_removed_handler(NULL,NULL,NULL,NULL));
}

/*==========================================================================*
 * basic
 *==========================================================================*/

static const char* test_basic_key = "foo";
static const char* test_basic_bad_key = "bar";

static
void
test_basic_add_cb(
    RadioRegistry* registry,
    RadioInstance* radio,
    gpointer user_data)
{
    (*((int*)user_data))++;
}

static
void
test_basic_remove_cb(
    RadioRegistry* registry,
    const char* str,
    gpointer user_data)
{
    g_assert_cmpstr(str, == ,test_basic_key);
    (*((int*)user_data))++;
}

static
void
test_basic(
    void)
{
    RadioRegistry* reg = radio_registry_new();
    int add_count = 0, remove_count = 0;
    gulong id[6];
    GObject* instance;

    g_assert(reg);
    g_assert(reg == radio_registry_new()); /* New ref to the same instance */
    radio_registry_unref(reg);

    g_assert(!radio_registry_add_instance_added_handler(reg,NULL,NULL,NULL));
    g_assert(!radio_registry_add_instance_removed_handler(reg,NULL,NULL,NULL));

    /* Add/remove handlers */
    id[0] = radio_registry_add_instance_added_handler(reg, "",
        test_basic_add_cb, &add_count);
    radio_registry_remove_handler(reg, id[0]);
    id[0] = radio_registry_add_instance_added_handler(reg, NULL,
        test_basic_add_cb, &add_count);
    id[1] = radio_registry_add_instance_added_handler(reg, test_basic_bad_key,
        test_basic_add_cb, &add_count);  /* won't get called */

    id[2] = radio_registry_add_instance_removed_handler(reg, NULL,
        test_basic_remove_cb, &remove_count);
    id[3] = radio_registry_add_instance_removed_handler(reg, "",
        test_basic_remove_cb, &remove_count);
    id[4] = radio_registry_add_instance_removed_handler(reg, test_basic_key,
        test_basic_remove_cb, &remove_count);
    id[5] = radio_registry_add_instance_removed_handler(reg, test_basic_bad_key,
        test_basic_remove_cb, &remove_count); /* won't get called */

    /* Well, this wouldn't be a real functional instance but we don't care */
    instance = g_object_new(RADIO_TYPE_INSTANCE, NULL);
    radio_registry_instance_added(RADIO_INSTANCE(instance));
    g_assert_cmpint(add_count, == ,1); /* 1 out of 2 is called */
    g_assert_cmpint(remove_count, == ,0);

    radio_registry_instance_removed(test_basic_key);
    g_assert_cmpint(add_count, == ,1);
    g_assert_cmpint(remove_count, == ,3); /* 3 our of 4 are called */
    g_object_unref(instance);

    /* remove_all zeros the ids */
    radio_registry_remove_all_handlers(reg, id);
    g_assert(!id[0]);
    g_assert(!id[4]);

    radio_registry_remove_handler(reg, 0); /* No effect */
    radio_registry_unref(reg);
}

/*==========================================================================*
 * Common
 *==========================================================================*/

#define TEST_PREFIX "/registry/"
#define TEST_(t) TEST_PREFIX t

int main(int argc, char* argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("null"), test_null);
    g_test_add_func(TEST_("basic"), test_basic);
    test_init(&test_opt, argc, argv);
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
