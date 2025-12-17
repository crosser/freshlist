#include <esp_log.h>
#include <esp_http_client.h>
// #include <esp_tls.h>
#include "httpc.h"
#include "display.h"

#if defined __has_include
#  if __has_include("../url.h")
#    include "../url.h"
#  endif
#endif

#include "sdkconfig.h"

#ifdef LOCAL_URL
#  define URL LOCAL_URL
#else
#  define URL CONFIG_URL
#endif

#define TAG "httpc"

#define BUFFER_SIZE 1024

typedef struct {
	char *buffer;
	size_t capacity;
	size_t current;
} data_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
	data_t *data = (data_t *)evt->user_data;

	switch(evt->event_id) {
	case HTTP_EVENT_ON_CONNECTED:
		ESP_LOGI(TAG, "Event HTTP_EVENT_ON_CONNECTED");
		break;
	case HTTP_EVENT_HEADERS_SENT:
		ESP_LOGI(TAG, "Event HTTP_EVENT_HEADERS_SENT");
		break;
	case HTTP_EVENT_ON_HEADER:
		ESP_LOGI(TAG, "Event HTTP_EVENT_ON_HEADER");
		break;
	case HTTP_EVENT_ON_HEADERS_COMPLETE:
		ESP_LOGI(TAG, "Event HTTP_EVENT_ON_HEADERS_COMPLETE");
		break;
	case HTTP_EVENT_ON_STATUS_CODE:
		ESP_LOGI(TAG, "Event HTTP_EVENT_ON_STATUS_CODE");
		break;
	case HTTP_EVENT_ON_DATA:
		ESP_LOGI(TAG, "Data len=%d", evt->data_len);
		if (data->current + evt->data_len < data->capacity) {
			memcpy(&(data->buffer[data->current]), evt->data,
					evt->data_len);
			data->current += evt->data_len;
		} else {
			ESP_LOGE(TAG, "Too much data: %zu + %zu > %zu",
					data->current, evt->data_len,
					data->capacity);
		}
		ESP_LOGI(TAG, "Data \"%.*s\"", evt->data_len, evt->data);
		break;
	case HTTP_EVENT_ON_FINISH:
		ESP_LOGI(TAG, "Event HTTP_EVENT_ON_FINISH");
		break;
	case HTTP_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "Event HTTP_EVENT_DISCONNECTED");
		break;
	case HTTP_EVENT_REDIRECT:
		ESP_LOGI(TAG, "Event HTTP_EVENT_REDIRECT");
		break;
	default:
		ESP_LOGI(TAG, "Event %d", evt->event_id);
		break;
	}
	return ESP_OK;
}

void httpc(void *drawhdl)
{
	char buf[BUFFER_SIZE] = {0};
	data_t data = {
		.buffer = buf,
		.capacity = BUFFER_SIZE - 1,
		.current = 0,
	};

	ESP_LOGI(TAG, "connecting to %s", URL);
	draw_main(drawhdl, "HTTP client called");
	esp_http_client_handle_t client = esp_http_client_init(
		&(esp_http_client_config_t){
			.url = URL,
			.event_handler = http_event_handler,
			.user_data = &data,
		}
	);

	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGD(TAG, "got \"%s\"", buf);
		draw_main(drawhdl, buf);
	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s",
				esp_err_to_name(err));
		draw_main(drawhdl, (char *)esp_err_to_name(err));
	}

	esp_http_client_cleanup(client);
}
