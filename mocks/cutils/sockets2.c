/**
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sockets2.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>
#include <syslog.h>
#include <errno.h>
#include <libqrtr.h>

#ifndef SOCKET_PATH_PREFIX
#define SOCKET_PATH_PREFIX "/run/"
#endif
#define DONT_CARE_PORT_NUM 0

static int make_sockaddr_un(const char *name, int type, struct sockaddr_un* addr) {
    int namelen = strlen(name) + strlen(SOCKET_PATH_PREFIX);

    if (namelen > sizeof(*addr)
                  - offsetof(struct sockaddr_un, sun_path) - 1) {
        return -1;
    }
    g_strlcpy(addr->sun_path, SOCKET_PATH_PREFIX, sizeof(addr->sun_path) + 1);
    g_strlcat(addr->sun_path, name, sizeof(addr->sun_path) + 1);
    addr->sun_family = AF_LOCAL;

    return 0;
}


int ril_socket_local_server(const char *name, int type) {
    int fdListen = socket(AF_LOCAL, type, 0);
    if (fdListen < 0) {
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));

    if (make_sockaddr_un(name, type, &addr) < 0) {
        return -1;
    }

    unlink(addr.sun_path);
    if (bind(fdListen, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fdListen);
        return -1;
    }

    return fdListen;
}

int ril_socket_local_client(const char *name, int type) {
    int s;

    s = socket(AF_LOCAL, type, 0);
    if(s < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));

    if (make_sockaddr_un(name, type, &addr) < 0) {
        return -1;
    }

    if ( 0 > connect(s, (struct sockaddr*)&addr, sizeof(addr))) {
        close(s);
        return -1;
    }

    return s;
}

/*
 * Create a qrtr socket server with the specific service and instance ID.
 *
 * @param[in] service - service ID to set to the qrtr socket server
 * @param[in] instance - instance ID to set to the qrtr the qsocket server
 */
int get_qsocket_server(const int service, const int instance) {
    int fd = qrtr_open(DONT_CARE_PORT_NUM);
    if (fd < 0) {
        syslog(LOG_ERR, "%s:[%d] qrtr_open failed %d for service = %d, instance = %d",
            __func__, __LINE__, fd, service, instance);
        return SOCKET_GENERIC_FAILURE;
    }
    int rc = qrtr_publish(fd, service, 0, instance);
    if (rc < 0) {
        syslog(LOG_ERR, "%s:[%d] publish failed: %s, rc = %d", __func__, __LINE__,
            strerror(errno), rc);
        qrtr_close(fd);
        return PUBLISH_FAILURE;
    }

    return fd;
}

/*
 * Create a socket client and lookup a qrtr socket server with the specific
 * service and instance ID.
 *
 * @param[in]  service - service ID to query the qrtr socket server
 * @param[in]  instance - instance ID to query the qrtr socket server
 */
int get_qsocket_client(const int service, const int instance) {
    int fd = qrtr_open(DONT_CARE_PORT_NUM);
    if (fd < 0) {
        syslog(LOG_ERR, "%s:[%d] qrtr_open failed %d for service = %d, instance = %d",
            __func__, __LINE__, fd, service, instance);
        return SOCKET_GENERIC_FAILURE;
    }

    int rc = qrtr_new_lookup(fd, service, 0, instance);
    if (rc < 0) {
        syslog(LOG_ERR, "%s:[%d] lookup failed: %s, rc = %d", __func__, __LINE__,
            strerror(errno), rc);
        qrtr_close(fd);
        return LOOKUP_FAILURE;
    }

    return fd;
}
