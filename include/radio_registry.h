/*
 * Copyright (C) 2018-2019 Jolla Ltd.
 * Copyright (C) 2018-2019 Slava Monich <slava.monich@jolla.com>
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

#ifndef RADIO_REGISTRY_H
#define RADIO_REGISTRY_H

#include <radio_types.h>

G_BEGIN_DECLS

typedef
void
(*RadioRegistryInstanceFunc)(
    RadioRegistry* registry,
    RadioInstance* radio,
    gpointer user_data);

typedef
void
(*RadioRegistryStrFunc)(
    RadioRegistry* registry,
    const char* str,
    gpointer user_data);

RadioRegistry*
radio_registry_new(
    void);

RadioRegistry*
radio_registry_ref(
    RadioRegistry* reg);

void
radio_registry_unref(
    RadioRegistry* reg);

gulong
radio_registry_add_instance_added_handler(
    RadioRegistry* reg,
    const char* key, /* NULL for any */
    RadioRegistryInstanceFunc func,
    gpointer user_data);

gulong
radio_registry_add_instance_removed_handler(
    RadioRegistry* reg,
    const char* key, /* NULL for any */
    RadioRegistryStrFunc func,
    gpointer user_data);

void
radio_registry_remove_handler(
    RadioRegistry* reg,
    gulong id);

void
radio_registry_remove_handlers(
    RadioRegistry* reg,
    gulong* ids,
    int count);

#define radio_registry_remove_all_handlers(reg,ids) \
    radio_registry_remove_handlers(reg, ids, G_N_ELEMENTS(ids))

G_END_DECLS

#endif /* RADIO_REGISTRY_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */

