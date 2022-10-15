/**
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

/******************************************************************************
  @file    read_config.c
  @brief   Read settings from configuration file
******************************************************************************/

#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <syslog.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ril_config.h"

#define DEFAULT_CONFIG_FILE "tel.conf"
#define MAX_CONFIG 512
#define LINE_LENGTH 256
#define PROP_KEY_MAX   32
#define PROP_VALUE_MAX  92
#define FILE_PATH_MAX 100

/* -----------------------------------------------------------------------------
   ConfigSettings
   DESCRIPTION:
   Contains key and value pairs for configuration settings
-------------------------------------------------------------------------------*/

typedef struct {
   char key[PROP_KEY_MAX];
   char value[PROP_VALUE_MAX];
} ConfigSettings;

ConfigSettings gSettings[MAX_CONFIG];

bool confFileRead = false;

void getCurrentProcessPath(char *procPath) {
   ssize_t count;
   if ((count = readlink("/proc/self/exe", procPath, FILE_PATH_MAX-1)) > 0) {
       procPath[count] = '\0';
   }
   syslog(LOG_DEBUG, "%s:[%d] complete process path: %s", __func__, __LINE__,
          procPath);
}

void getCurrentProcessName(char *procName) {
   char path[FILE_PATH_MAX];
   getCurrentProcessPath(path);
   strlcpy(procName, basename(path), PROP_VALUE_MAX-1);
   procName[PROP_VALUE_MAX-1] = '\0';
}

void getProcessPath(char *procPath) {
   char path[FILE_PATH_MAX];
   getCurrentProcessPath(path);
   strlcpy(procPath, dirname(path), FILE_PATH_MAX-1);
   procPath[FILE_PATH_MAX-1] = '\0';
}

/*
 * Order of search for the config file:
 * 1. Search for <processName>.conf in etc folder.
 * 2. Search for <processName>.conf in the folder that contains the process.
 * 3. Search for tel.conf in etc folder.
 * 4. Search for tel.conf in the folder that contains the process.
 */
void getConfigFilePath(char *configFile) {

    char procPath[FILE_PATH_MAX];
    char procName[PROP_VALUE_MAX];

    getProcessPath(procPath);
    syslog(LOG_DEBUG, "%s:[%d] process path: %s", __func__, __LINE__, procPath);

    getCurrentProcessName(procName);

    syslog(LOG_DEBUG, "%s:[%d] process name: %s", __func__, __LINE__, procName);

    // get current config file based on process name in etc folder
    memset(configFile, 0, FILE_PATH_MAX);
    strlcat(configFile, "/etc/", FILE_PATH_MAX);
    strlcat(configFile, procName, FILE_PATH_MAX);
    strlcat(configFile, ".conf", FILE_PATH_MAX);
    syslog(LOG_DEBUG, "%s:[%d] config file path: %s", __func__, __LINE__, configFile);

    if (access(configFile, F_OK) != -1) {
       syslog(LOG_DEBUG, "%s:[%d] able to access config file path: %s", __func__, __LINE__,
              configFile);
       return;
    } else {
       syslog(LOG_DEBUG, "%s:[%d] %s is not in etc folder", __func__, __LINE__, configFile);
    }

    // get current config file where process is running
    memset(configFile, 0, FILE_PATH_MAX);
    strlcat(configFile, procPath, FILE_PATH_MAX);
    strlcat(configFile, "/", FILE_PATH_MAX);
    strlcat(configFile, procName, FILE_PATH_MAX);
    strlcat(configFile, ".conf", FILE_PATH_MAX);
    syslog(LOG_DEBUG, "%s:[%d] config file path: %s", __func__, __LINE__,
           configFile);
    if (access(configFile, F_OK) != -1) {
       syslog(LOG_DEBUG, "%s:[%d] able to access config file path: %s", __func__, __LINE__,
              configFile);
       return;
    } else {
       syslog(LOG_DEBUG, "%s:[%d] %s is not in the folder that contains the process.",
              __func__, __LINE__, configFile);
    }

    // get default config file (tel.conf) from /etc
    memset(configFile, 0, FILE_PATH_MAX);
    strlcat(configFile, "/etc/", FILE_PATH_MAX);
    strlcat(configFile, DEFAULT_CONFIG_FILE, FILE_PATH_MAX);
    syslog(LOG_DEBUG, "%s:[%d] config file path: %s", __func__, __LINE__,
          configFile);
    if (access(configFile, F_OK) != -1) {
      syslog(LOG_DEBUG, "%s:[%d] able to access config file path: %s", __func__, __LINE__,
             configFile);
      return;
    } else {
      syslog(LOG_DEBUG, "%s:[%d] default %s is not in etc folder", __func__, __LINE__, configFile);
    }

    // get default config file where process is running
    memset(configFile, 0, FILE_PATH_MAX);
    strlcat(configFile, procPath, FILE_PATH_MAX);
    strlcat(configFile, "/", FILE_PATH_MAX);
    strlcat(configFile, DEFAULT_CONFIG_FILE, FILE_PATH_MAX);
    syslog(LOG_DEBUG, "%s:[%d] config file path: %s", __func__, __LINE__,
          configFile);
    if (access(configFile, F_OK) != -1) {
      syslog(LOG_DEBUG, "%s:[%d] able to access config file path:  %s", __func__, __LINE__,
             configFile);
      return;
    } else {
      syslog(LOG_DEBUG, "%s:[%d] default %s is not in the folder that contains the process.",
             __func__, __LINE__, configFile);
    }
    return;
}

void trim(char *token) {

    int index = 0;

    // Trim leading white spaces
    while(token[index] == ' ' || token[index] == '\t' || token[index] == '\n') {
        index++;
    }

    // Shift all trailing characters to its left
    int i = 0;
    while(token[i + index] != '\0') {
        token[i] = token[i + index];
        i++;
    }
    token[i] = '\0';

    // Trim trailing white spaces
    i = 0;
    index = -1;
    while(token[i] != '\0') {
        if(token[i] != ' ' && token[i] != '\t' && token[i] != '\n') {
            index = i;
        }
        i++;
    }
    token[index + 1] = '\0';
}

/*
 * Utility function to read config file with key value pairs
 * Prepares an array of structure which has key and value data
 * Discards blank lines and lines starting with #
 */
void readConfigFile(char *configurationFile) {
   char property[LINE_LENGTH];
   char *savePtr = NULL;
   int index = 0;

   FILE *ifp = NULL;
   char mode[] = "r";

   ifp = fopen(configurationFile, mode);

   if (ifp == NULL) {
      syslog(LOG_ERR, "%s:[%d] Unable to open config file: %s", __func__, __LINE__,
             configurationFile);
      return;
   }

   while (fgets(property, LINE_LENGTH, ifp) != NULL) {
      char *token = NULL;
      if (property[0] == '#' || strlen(property) == 1) {
         continue;
      } else {
         token = strtok_r(property, "=", &savePtr);
         if (token != NULL) {
            trim(token);
            strlcpy(gSettings[index].key, token, PROP_KEY_MAX - 1);
            gSettings[index].key[PROP_KEY_MAX - 1] = '\0';
         }
         token = strtok_r(NULL, "=", &savePtr);
         if (token != NULL) {
            trim(token);
            strlcpy(gSettings[index].value, token, PROP_VALUE_MAX - 1);
            gSettings[index].value[PROP_VALUE_MAX - 1] = '\0';
         }
         index++;
      }
   }
}

void init() {
   //Check if key already exists, if not read the config file and populate ConfigSettings
   if (strlen(gSettings[0].key) == 0) {
      char configPath[FILE_PATH_MAX];
      getConfigFilePath(configPath);
      readConfigFile(configPath);
      confFileRead = true;
   }

}

void getConfigValue(const char *configKey, char *configVal) {

   if(!confFileRead) {
      init();
   }

   for (int index = 0; strlen(gSettings[index].key) != 0; index++) {
      if (strncmp (gSettings[index].key , configKey, sizeof(gSettings[index].key)) == 0) {
         strlcpy(configVal, gSettings[index].value, PROP_VALUE_MAX-1);
         configVal[PROP_VALUE_MAX-1] = '\0';
         syslog(LOG_DEBUG, "%s:[%d] key: %s configVal: %s", __func__, __LINE__,
            gSettings[index].key, configVal);
         break;
      }
   }
}
