/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <lwip/sys.h>
#include <lwip/err.h>
#include <nvs_flash.h>
#include "wifi.h"

#if defined __has_include
#  if __has_include("../credentials.h")
#    include "../credentials.h"
#  endif
#endif

#include "sdkconfig.h"

#ifdef LOCAL_WIFI_SSID
#  define WIFI_SSID LOCAL_WIFI_SSID
#else
#  define WIFI_SSID CONFIG_WIFI_SSID
#endif

#ifdef LOCAL_WPA_PASSWORD
#  define WPA_PASSWORD LOCAL_WPA_PASSWORD
#else
#  define WPA_PASSWORD CONFIG_WPA_PASSWORD
#endif

static const char *TAG = "wifi";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
		int32_t event_id, void* event_data)
{
	assert(event_base == WIFI_EVENT);
	switch (event_id) {
	case WIFI_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case WIFI_EVENT_STA_CONNECTED:
		ESP_LOGI(TAG, "connect succeeded");
		break;
	case WIFI_EVENT_STA_DISCONNECTED:
		xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		ESP_LOGI(TAG, "connect failed");
		break;
	case WIFI_EVENT_HOME_CHANNEL_CHANGE:
		ESP_LOGI(TAG, "channel change");
		break;
	default:
		ESP_LOGI(TAG, "unexpected wifi event id %d", event_id);
		break;
	}
}

static void ip_event_handler(void* arg, esp_event_base_t event_base,
		int32_t event_id, void* event_data)
{
	assert(event_base == IP_EVENT);
	switch (event_id) {
	case IP_EVENT_STA_GOT_IP:
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		break;
	default:
		ESP_LOGI(TAG, "unexpected ip event id %d", event_id);
		break;
	}
}

void init_wifi(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES
		|| ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "Starting WiFi");
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();
	ESP_ERROR_CHECK(esp_wifi_init(
			&(wifi_init_config_t)WIFI_INIT_CONFIG_DEFAULT()));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
			ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
			IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,
		       	&(wifi_config_t){
				.sta = {
					.ssid = WIFI_SSID,
					.password = WPA_PASSWORD,
				},
			}));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_LOGI(TAG, "wifi_init_sta finished.");
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
			WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
			pdFALSE,
			pdFALSE,
			portMAX_DELAY);

	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap");
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to ap");
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}
}

void stop_wifi(void)
{
	ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT,
				IP_EVENT_STA_GOT_IP, &ip_event_handler));
	ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT,
				ESP_EVENT_ANY_ID, &wifi_event_handler));
	vEventGroupDelete(s_wifi_event_group);
	esp_wifi_stop();
	ESP_LOGI(TAG, "WiFi stopped");
}
