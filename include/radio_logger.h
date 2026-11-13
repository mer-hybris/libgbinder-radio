/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Since 1.7.0
 */

#ifndef RADIO_LOGGER_H
#define RADIO_LOGGER_H

#include <radio_types.h>

G_BEGIN_DECLS

RadioLogger*
radio_logger_new_trace(
    RadioInstance* radio,
    GLogModule* module,
    int level,
    const char* prefix);

RadioLogger*
radio_logger_new_dump(
    RadioInstance* radio,
    GLogModule* module,
    int level,
    const char* prefix);

RadioLogger*
radio_logger_new_config_trace(
    RadioConfig* config,
    GLogModule* module,
    int level,
    const char* prefix);

RadioLogger*
radio_logger_new_config_dump(
    RadioConfig* config,
    GLogModule* module,
    int level,
    const char* prefix);

void
radio_logger_free(
    RadioLogger* logger);

G_END_DECLS

#endif /* RADIO_LOGGER_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
