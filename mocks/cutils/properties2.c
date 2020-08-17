/**
 * Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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

#include "properties2.h"
#include "ril_config.h"

int property_get(const char* key, char* value, const char* defaultValue)
{
    if(!key || !value) {
        return 0;
    }
    bool propertyFound = getConfigValue(key, value);
    if(!propertyFound) {
        if(defaultValue) {
            strlcpy(value, defaultValue, PROPERTY_VALUE_MAX);
        } else {
            return 0;
        }
    }
    value[PROPERTY_VALUE_MAX-1] = '\0';
    return strlen(value);
}

int8_t property_get_bool(const char* key, int8_t default_value)
{
    return 0;
}

int64_t property_get_int64(const char* key, int64_t default_value)
{
    return 0;
}

int32_t property_get_int32(const char* key, int32_t default_value)
{
    return 0;
}

int property_set(const char* key, const char* value)
{
    return 0;
}

int property_list(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie)
{
  return 0;
}
