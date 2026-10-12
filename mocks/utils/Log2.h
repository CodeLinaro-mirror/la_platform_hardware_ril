/**
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following
 *   disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * * Neither the name of The Linux Foundation nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SOURCES_LOG_H
#define SOURCES_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

#define RIL_LOG_ERROR         1
#define RIL_LOG_WARNING       2
#define RIL_LOG_DEBUG         3
#define RIL_LOG_VERBOSE       4
#define RIL_LOG_INFO          5

#ifdef TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED
#include <clog.h>
#include <clog_ids.h>
extern clog_context_t clog_ctx;

#define RLOGE(fmt, args...) \
    do { writeToClog(clog_ctx, __FILE__, __LINE__, RIL_LOG_ERROR, \
            "[DJ:E][%s:%d] " fmt, __func__, __LINE__, ## args); } while (0)
#define RLOGI(fmt, args...) \
    do { writeToClog(clog_ctx, __FILE__, __LINE__, RIL_LOG_INFO, \
            "[DJ:I][%s:%d] " fmt, __func__, __LINE__, ## args); } while (0)
#define RLOGW(fmt, args...) \
    do { writeToClog(clog_ctx, __FILE__, __LINE__, RIL_LOG_WARNING, \
            "[DJ:W][%s:%d] " fmt, __func__, __LINE__, ## args); } while (0)
#define RLOGD(fmt, args...) \
    do { writeToClog(clog_ctx, __FILE__, __LINE__, RIL_LOG_DEBUG, \
            "[DJ:D][%s:%d] " fmt, __func__, __LINE__, ## args); } while (0)
#define RLOGW_IF(cond, fmt, args...) \
    do { if (cond) writeToClog(clog_ctx, __FILE__, __LINE__, RIL_LOG_WARNING, \
            "RLOGW_IF: " fmt, ## args); } while (0)
#define RLOGI_IF(cond, fmt, args...) \
    do { if (cond) writeToClog(clog_ctx, __FILE__, __LINE__, RIL_LOG_INFO, \
            "RLOGI_IF: " fmt, ## args); } while (0)
#define RLOGV_IF(cond, fmt, args...) \
    do { if (cond) writeToClog(clog_ctx, __FILE__, __LINE__, RIL_LOG_VERBOSE, \
            "RLOGV_IF: " fmt, ## args); } while (0)
#define ALOGE(fmt, args...) \
    do { writeToClog(clog_ctx, __FILE__, __LINE__, RIL_LOG_ERROR, \
            "ALOGE: " fmt, ## args); } while (0)
#define RLOGV(fmt, args...) \
    do { writeToClog(clog_ctx, __FILE__, __LINE__, RIL_LOG_VERBOSE, \
            "RLOGV: " fmt, ## args); } while (0)

static inline clog_level_t toClogLevel(int ril_level)
{
    switch (ril_level) {
    case RIL_LOG_ERROR:
        return CLOG_LEVEL_ERROR;
    case RIL_LOG_WARNING:
        return CLOG_LEVEL_WARN;
    case RIL_LOG_INFO:
        return CLOG_LEVEL_INFO;
    case RIL_LOG_DEBUG:
        return CLOG_LEVEL_DEBUG;
    case RIL_LOG_VERBOSE:
        return CLOG_LEVEL_VERBOSE;
    default:
        return CLOG_LEVEL_ERROR;
    }
}

static inline void writeToClog(clog_context_t ctx, const char *file, int line,
        int ril_level, const char *fmt, ...)
{
    char message[4096];
    va_list args;
    int formatted_length;
    clog_return_t clog_rc;

    message[0] = '\0';
    if (ctx == NULL || file == NULL || fmt == NULL) {
        syslog(LOG_ERR, "RILD CLOG log skipped: invalid context or format");
        return;
    }

    va_start(args, fmt);
    formatted_length = vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    if (formatted_length < 0) {
        syslog(LOG_ERR, "RILD CLOG log formatting failed: file=%s line=%d", file, line);
        return;
    }
    if ((size_t)formatted_length >= sizeof(message)) {
        syslog(LOG_WARNING, "RILD CLOG log truncated: file=%s line=%d", file, line);
    }

    clog_rc = clog_log(ctx, file, line, toClogLevel(ril_level), "%s", message);
    if (clog_rc != CLOG_RETURN_OK) {
        syslog(LOG_ERR, "RILD CLOG log failed: rc=%d file=%s line=%d",
                clog_rc, file, line);
    }
}

#else
//Legacy logging

#define RLOGE(fmt, args...) \
    do_syslog_new(LOG_ERR, RIL_LOG_ERROR, "[DJ:E][%s:%d] " fmt, __func__, __LINE__, ## args)
#define RLOGI(fmt, args...) \
    do_syslog_new(LOG_ERR, RIL_LOG_INFO, "[DJ:I][%s:%d] " fmt, __func__, __LINE__, ## args)
#define RLOGW(fmt, args...) \
    do_syslog_new(LOG_ERR, RIL_LOG_WARNING, "[DJ:W][%s:%d] " fmt, __func__, __LINE__, ## args)
#define RLOGD(fmt, args...) \
    do_syslog_new(LOG_ERR, RIL_LOG_DEBUG, "[DJ:D][%s:%d] " fmt, __func__, __LINE__, ## args)
#define RLOGW_IF(...) do {do_syslog_new(LOG_ERR, RIL_LOG_WARNING, "RLOGW_IF: %s %s", __func__, __VA_ARGS__ );} while(0)
#define RLOGI_IF(...) do {do_syslog_new(LOG_ERR, RIL_LOG_INFO, "RLOGI_IF: %s %s", __func__, __VA_ARGS__ );} while(0)
#define RLOGV_IF(...) do {do_syslog_new(LOG_ERR, RIL_LOG_VERBOSE, "RLOGV_IF: %s %s", __func__, __VA_ARGS__ );} while(0)
#define ALOGE(...) do {do_syslog_new(LOG_ERR, RIL_LOG_ERROR, "ALOGE: %s %s", __func__, __VA_ARGS__ );} while(0)
#define RLOGV(...) do {do_syslog_new(LOG_ERR, RIL_LOG_VERBOSE, "RLOGV: %s %s", __func__, __VA_ARGS__ );} while(0)

static inline void do_syslog_new(int level, int ril_level, const char *fmt, ...)
{
    // LOG only ERROR by default into sys log
    if ( ril_level == RIL_LOG_ERROR) {
        va_list args;

        va_start(args, fmt);
        vsyslog(level, fmt, args);
        va_end(args);
    }
}
#endif //TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED

#endif //SOURCES_LOG_H
