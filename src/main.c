/*
 * Copyright 2026 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdbool.h>
#include <stdio.h>

#include <zephyr/kernel.h>
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>

#include <anj/core.h>
#include <anj/defs.h>
#include <anj/dm/device_object.h>
#include <anj/dm/security_object.h>
#include <anj/dm/server_object.h>
#include <anj/log.h>

#define log(...) anj_log(anjay_lite_zephyr_demo, __VA_ARGS__)

// Installs Device Object and adds an instance of it.
// An instance of Device Object provides the data related to a device.
static int install_device_obj(anj_t *anj, anj_dm_device_obj_t *device_obj) {
    anj_dm_device_object_init_t device_obj_conf = {
        .firmware_version = "0.1"
    };
    return anj_dm_device_obj_install(anj, device_obj, &device_obj_conf);
}

// Installs Server Object and adds an instance of it.
// An instance of Server Object provides the data related to a LwM2M Server.
static int install_server_obj(anj_t *anj, anj_dm_server_obj_t *server_obj) {
    anj_dm_server_instance_init_t server_inst = {
        .ssid = 1,
        .lifetime = 50,
        .binding = "U",
        .bootstrap_on_registration_failure = &(bool) { false },
    };
    anj_dm_server_obj_init(server_obj);
    if (anj_dm_server_obj_add_instance(server_obj, &server_inst)
            || anj_dm_server_obj_install(anj, server_obj)) {
        return -1;
    }
    return 0;
}

// Installs Security Object and adds an instance of it.
// An instance of Security Object provides information needed to connect to
// LwM2M server.
static int install_security_obj(anj_t *anj,
                                anj_dm_security_obj_t *security_obj) {
    anj_dm_security_instance_init_t security_inst = {
        .ssid = 1,
        .server_uri = "coaps://eu.iot.avsystem.cloud:5684",
        .security_mode = ANJ_DM_SECURITY_PSK,
        .public_key_or_identity = {
            .source = ANJ_CRYPTO_DATA_SOURCE_BUFFER,
            .info.buffer.data = CONFIG_ANJAY_LITE_DEMO_ENDPOINT_NAME,
            .info.buffer.data_size = sizeof(CONFIG_ANJAY_LITE_DEMO_ENDPOINT_NAME) - 1
        },
        .secret_key = {
            .source = ANJ_CRYPTO_DATA_SOURCE_BUFFER,
            .info.buffer.data = CONFIG_ANJAY_LITE_DEMO_PSK,
            .info.buffer.data_size = sizeof(CONFIG_ANJAY_LITE_DEMO_PSK) - 1
        },
    };
    anj_dm_security_obj_init(security_obj);
    if (anj_dm_security_obj_add_instance(security_obj, &security_inst)
            || anj_dm_security_obj_install(anj, security_obj)) {
        return -1;
    }
    return 0;
}

anj_t anj;
anj_dm_device_obj_t device_obj;
anj_dm_server_obj_t server_obj;
anj_dm_security_obj_t security_obj;

int main(void) {
    int err = nrf_modem_lib_init();
    if (err) {
        log(L_ERROR, "Failed to initialize modem library: %d", err);
        return -1;
    }

    err = lte_lc_connect();
    if (err) {
        log(L_ERROR, "Failed to establish LTE link: %d", err);
        return -1;
    }

    anj_configuration_t config = {
        .endpoint_name = CONFIG_ANJAY_LITE_DEMO_ENDPOINT_NAME
    };
    if (anj_core_init(&anj, &config)) {
        log(L_ERROR, "Failed to initialize Anjay Lite");
        return -1;
    }

    if (install_device_obj(&anj, &device_obj)
            || install_security_obj(&anj, &security_obj)
            || install_server_obj(&anj, &server_obj)) {
        return -1;
    }

    while (true) {
        anj_core_step(&anj);
        k_sleep(K_MSEC(50));
    }
    return 0;
}
