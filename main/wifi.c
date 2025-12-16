/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdlib.h>
#include <time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include <esp_netif_sntp.h>
#include <esp_netif_net_stack.h>
#include <lwip/sys.h>
#include <lwip/err.h>
#include <lwip/dhcp6.h>
#include <nvs_flash.h>
#include "wifi.h"
#include "display.h"
#include "httpc.h"

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

#define HAVE_IPV4 BIT0
#define HAVE_IPV6 BIT1
#define WIFI_FAIL BIT2

static int retries;
static esp_netif_t *wifi_netif;
static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
		int32_t event_id, void* event_data)
{
	uint16_t ap_num;
	assert(event_base == WIFI_EVENT);

	switch (event_id) {
	case WIFI_EVENT_STA_START:
		ESP_ERROR_CHECK(esp_wifi_scan_start(&(wifi_scan_config_t){
					.scan_type = WIFI_SCAN_TYPE_ACTIVE,
				}, false));
		break;
	case WIFI_EVENT_STA_STOP:
		ESP_LOGI(TAG, "Stopped");
		break;
	case WIFI_EVENT_SCAN_DONE:
		ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
		ESP_LOGI(TAG, "Scan returned %hu records", ap_num);
		wifi_ap_record_t *ap_records =
			malloc(sizeof(wifi_ap_record_t) * ap_num);
		if (!ap_records) {
			ESP_LOGE(TAG, "Failed to allocate record results");
			break;
		}
		ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(
					&ap_num, ap_records));
		for (int i = 0; i < ap_num; i++) {
			ESP_LOGI(TAG, "AP %i: ssid %.33s, ch %i",
					i, ap_records[i].ssid,
					ap_records[i].primary);
		}
		free(ap_records);
		ESP_ERROR_CHECK(esp_wifi_connect());  // select SSID here
		break;
	case WIFI_EVENT_STA_CONNECTED:
		ESP_LOGI(TAG, "Connected");
		// Have to do this by hand for SLAAC to work!?
		ESP_ERROR_CHECK(esp_netif_create_ip6_linklocal(wifi_netif));
		ESP_ERROR_CHECK(dhcp6_enable_stateless(
				esp_netif_get_netif_impl(wifi_netif)));
		break;
	case WIFI_EVENT_STA_DISCONNECTED:
		ESP_LOGI(TAG, "Disconnected");
		if (retries-- > 0) {
			ESP_LOGI(TAG, "retries left: %d", retries);
			esp_wifi_connect();
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL);
		}
		break;
	case WIFI_EVENT_HOME_CHANNEL_CHANGE:
		ESP_LOGI(TAG, "Channel change");
		break;
	default:
		ESP_LOGI(TAG, "Unexpected wifi event id %d", event_id);
		break;
	}
}

static void ip_event_handler(void* arg, esp_event_base_t event_base,
		int32_t event_id, void* event_data)
{
	assert(event_base == IP_EVENT);
	switch (event_id) {
	case IP_EVENT_STA_GOT_IP:
		ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ipv4:" IPSTR,
				IP2STR(&event->ip_info.ip));
		xEventGroupSetBits(s_wifi_event_group, HAVE_IPV4);
		break;
	case IP_EVENT_GOT_IP6:
		ip_event_got_ip6_t *event6 = (ip_event_got_ip6_t*) event_data;
		ESP_LOGI(TAG, "got ipv6:" IPV6STR,
				IPV62STR(event6->ip6_info.ip));
		if (esp_netif_ip6_get_addr_type(&event6->ip6_info.ip)
				== ESP_IP6_ADDR_IS_GLOBAL) {
			ESP_LOGI(TAG, "It is global, can use!");
			xEventGroupSetBits(s_wifi_event_group, HAVE_IPV6);
		}
		break;
	default:
		ESP_LOGI(TAG, "unexpected ip event id %d", event_id);
		break;
	}
}

void init_wifi(void *drawhdl)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES
		|| ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "Starting WiFi");
	retries = 2;
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	wifi_netif = esp_netif_create_default_wifi_sta();
	ESP_ERROR_CHECK(esp_wifi_init(
			&(wifi_init_config_t)WIFI_INIT_CONFIG_DEFAULT()));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
			ESP_EVENT_ANY_ID, &wifi_event_handler, drawhdl));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
			IP_EVENT_STA_GOT_IP, &ip_event_handler, drawhdl));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
			IP_EVENT_GOT_IP6, &ip_event_handler, drawhdl));
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
			HAVE_IPV4 | HAVE_IPV6 | WIFI_FAIL,
			pdFALSE,
			pdFALSE,
			5000 / portTICK_PERIOD_MS);  // 1s

	if (bits & HAVE_IPV4) {
		ESP_LOGI(TAG, "connected with IPv4");
	} else if (bits & HAVE_IPV6) {
		ESP_LOGI(TAG, "connected with IPv6");
	} else if (bits & WIFI_FAIL) {
		ESP_LOGI(TAG, "Failed to connect to ap");
	} else {
		ESP_LOGE(TAG, "Timed out");
	}
	if (bits & (HAVE_IPV4 | HAVE_IPV6)) {
		ESP_ERROR_CHECK(esp_netif_sntp_init(
			&(esp_sntp_config_t)ESP_NETIF_SNTP_DEFAULT_CONFIG(
				"pool.ntp.org"
			)));
		httpc(drawhdl);
		ESP_ERROR_CHECK(esp_netif_sntp_sync_wait(
					pdMS_TO_TICKS(5000)));
		time_t now;
		struct tm timeinfo;
		char strftime_buf[64];
		setenv("TZ", CONFIG_TZSPEC, true);
		tzset();
		time(&now);
		localtime_r(&now, &timeinfo);
		strftime(strftime_buf, sizeof(strftime_buf),
				"%c %z", &timeinfo);
		ESP_LOGI(TAG, "Current time: %s", strftime_buf);
		draw_status(drawhdl, strftime_buf);
	} else {
		draw_main(drawhdl, "Failed to connect to the Internet");
	}
	retries = 0;
	ESP_ERROR_CHECK(esp_wifi_disconnect());
	ESP_ERROR_CHECK(esp_wifi_stop());
}

void stop_wifi(void)
{
	ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT,
				IP_EVENT_GOT_IP6, &ip_event_handler));
	ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT,
				IP_EVENT_STA_GOT_IP, &ip_event_handler));
	ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT,
				ESP_EVENT_ANY_ID, &wifi_event_handler));
	ESP_ERROR_CHECK(esp_wifi_stop());
	vEventGroupDelete(s_wifi_event_group);
	ESP_LOGI(TAG, "WiFi stopped");
}
