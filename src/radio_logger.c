/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Since 1.7.0
 */

#include "radio_logger.h"

#include "radio_base.h"
#include "radio_instance.h"
#include "radio_util.h"

#include <gbinder.h>

#include <gutil_log.h>

enum radio_logger_events {
    EVENT_REQ,
    EVENT_RESP,
    EVENT_IND,
    EVENT_ACK,
    EVENT_COUNT
};

struct radio_logger {
    RadioInstance* radio;
    GLogModule* module;
    int level;
    gulong event_id[EVENT_COUNT];
    char* prefix;
};

static
void
radio_logger_dump_reader(
    RadioLogger* logger,
    const GBinderReader* reader)
{
    gsize size;
    const guint8* data = gbinder_reader_get_data(reader, &size);

    gutil_log_dump(logger->module, logger->level, "  ", data, size);
}

static
void
radio_logger_trace_req(
    RadioInstance* radio,
    RADIO_REQ code,
    GBinderLocalRequest* args,
    gpointer user_data)
{
    RadioLogger* logger = user_data;
    const gsize header_size = radio_instance_rpc_header_size(radio, code);
    const char* name = radio_req_name2(radio, code);
    GBinderWriter writer;
    const guint8* data;
    guint32 serial;
    gsize size;

    /* Use writer API to fetch the raw data and extract the serial */
    gbinder_local_request_init_writer(args, &writer);
    data = gbinder_writer_get_data(&writer, &size);
    serial = (size >= header_size + 4) ? *(guint32*)(data + header_size) : 0;

    if (serial) {
        gutil_log(logger->module, logger->level, "%s< [%08x] %u %s",
            logger->prefix, serial, code, name ? name : "");
    } else {
        gutil_log(logger->module, logger->level, "%s< %u %s",
            logger->prefix, code, name ? name : "");
    }
}

static
void
radio_logger_trace_resp(
    RadioInstance* radio,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* args,
    gpointer user_data)
{
    RadioLogger* logger = user_data;
    const char* name = radio_resp_name2(radio, code);
    const char* error = (info->error == RADIO_ERROR_NONE) ? NULL :
        radio_error_string(info->error);
    const char* arg1 = name ? name : error;
    const char* arg2 = name ? error : NULL;

    if (arg2) {
        gutil_log(logger->module, logger->level, "%s> [%08x] %u %s %s",
            logger->prefix, info->serial, code, arg1, arg2);
    } else if (arg1) {
        gutil_log(logger->module, logger->level, "%s> [%08x] %u %s",
            logger->prefix, info->serial, code, arg1);
    } else {
        gutil_log(logger->module, logger->level, "%s> [%08x] %u",
            logger->prefix, info->serial, code);
    }
}

static
void
radio_logger_trace_ind(
    RadioInstance* radio,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* args,
    gpointer user_data)
{
    RadioLogger* logger = user_data;
    const char* name = radio_ind_name2(radio, code);

    gutil_log(logger->module, logger->level, "%s> %u %s",
        logger->prefix, code, name ? name : "");
}

static
void
radio_logger_trace_ack(
    RadioInstance* radio,
    guint32 serial,
    gpointer user_data)
{
    RadioLogger* logger = user_data;

    gutil_log(logger->module, logger->level, "%s> [%08x] ack",
        logger->prefix, serial);
}

static
void
radio_logger_dump_req(
    RadioInstance* radio,
    RADIO_REQ code,
    GBinderLocalRequest* args,
    gpointer user_data)
{
    RadioLogger* logger = user_data;
    GBinderWriter writer;
    const guint8* data;
    gsize size;

    /* Use writer API to fetch the raw data */
    gbinder_local_request_init_writer(args, &writer);
    data = gbinder_writer_get_data(&writer, &size);
    gutil_log_dump(logger->module, logger->level, "  ", data, size);
}

static
void
radio_logger_dump_resp(
    RadioInstance* radio,
    RADIO_RESP code,
    const RadioResponseInfo* info,
    const GBinderReader* args,
    gpointer user_data)
{
    radio_logger_dump_reader(user_data, args);
}

static
void
radio_logger_dump_ind(
    RadioInstance* radio,
    RADIO_IND code,
    RADIO_IND_TYPE type,
    const GBinderReader* args,
    gpointer user_data)
{
    radio_logger_dump_reader(user_data, args);
}

static
RadioLogger*
radio_logger_new(
    RadioInstance* radio,
    GLogModule* module,
    int level,
    const char* prefix,
    RADIO_INSTANCE_PRIORITY pri,
    RadioRequestObserverFunc req_cb,
    RadioResponseObserverFunc resp_cb,
    RadioIndicationObserverFunc ind_cb,
    RadioAckFunc ack_cb)
{
    if (radio) {
        RadioLogger* logger = g_new0(RadioLogger, 1);

        if (prefix && prefix[0]) {
            /* Ensure trailing space as a delimiter */
            if (g_str_has_suffix(prefix, " ")) {
                logger->prefix = g_strdup(prefix);
            } else {
                logger->prefix = g_strconcat(prefix, " ", NULL);
            }
        } else {
            logger->prefix =  g_strdup("");
        }
        logger->module = module;
        logger->level = level;
        logger->radio = radio_instance_ref(radio);
        logger->event_id[EVENT_REQ] =
            radio_instance_add_request_observer_with_priority(radio, pri,
                RADIO_REQ_ANY, req_cb, logger);
        logger->event_id[EVENT_RESP] =
            radio_instance_add_response_observer_with_priority(radio, pri,
                RADIO_RESP_ANY, resp_cb, logger);
        logger->event_id[EVENT_IND] =
            radio_instance_add_indication_observer_with_priority(radio, pri,
                RADIO_IND_ANY, ind_cb, logger);
        logger->event_id[EVENT_ACK] = radio_instance_add_ack_handler(radio,
            ack_cb, logger);
        return logger;
    } else {
        return NULL;
    }
}

/*==========================================================================*
 * API
 *==========================================================================*/

RadioLogger*
radio_logger_new_trace(
    RadioInstance* radio,
    GLogModule* module,
    int level,
    const char* prefix)
{
    return radio_logger_new(radio, module, level, prefix,
        RADIO_INSTANCE_PRIORITY_HIGHEST, radio_logger_trace_req,
        radio_logger_trace_resp, radio_logger_trace_ind,
        radio_logger_trace_ack);
}

RadioLogger*
radio_logger_new_dump(
    RadioInstance* radio,
    GLogModule* module,
    int level,
    const char* prefix)
{
    return radio_logger_new(radio, module, level, prefix,
        RADIO_INSTANCE_PRIORITY_HIGHEST - 1, radio_logger_dump_req,
        radio_logger_dump_resp, radio_logger_dump_ind,
        NULL);
}

RadioLogger*
radio_logger_new_config_trace(
    RadioConfig* config,
    GLogModule* module,
    int level,
    const char* prefix)
{
    return config ? radio_logger_new_trace(RADIO_BASE(config)->instance,
        module, level, prefix) : NULL;
}

RadioLogger*
radio_logger_new_config_dump(
    RadioConfig* config,
    GLogModule* module,
    int level,
    const char* prefix)
{
    return config ? radio_logger_new_dump(RADIO_BASE(config)->instance,
        module, level, prefix) : NULL;
}

void
radio_logger_free(
    RadioLogger* logger)
{
    if (logger) {
        radio_instance_remove_all_handlers(logger->radio, logger->event_id);
        radio_instance_unref(logger->radio);
        g_free(logger->prefix);
        g_free(logger);
    }
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
