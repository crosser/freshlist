#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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

#define BUFFER_SIZE 256

typedef struct {
	int index;
	char *buffer;
	size_t capacity;
	size_t current;
} data_ctx_t;

static void (*finish)(void);

static void process_line(int n, char *l)
{
	char *r, *w, *f = l;
	bool in, quote = false;
	int i = 0;
	char *e[2] = {};

	ESP_LOGI(TAG, "Line %d: %s", n, l);
	for (r=l, w=l, in=false; *r; r++) {
		switch (*r) {
		case '"':
			if (quote) *(w++) = *r;
			quote = in;
			in = !in;
			break;
		case ',':
			quote = false;
			if (!in) {
				*(w++) = '\0';
				if (i < 2) e[i++] = f;
				else ESP_LOGE(TAG, "csv %d: %s", i, f);
				f = w;
			}
			break;
		default:
			quote = false;
			*(w++) = *r;
			break;
		}
	}
	*w = '\0';
	if (i < 2) e[i++] = f;
	else ESP_LOGE(TAG, "csv %d: %s", i, f);
	ESP_LOGI(TAG, "%d: pfx=%s, msg=%s", n, e[0], e[1]);
}

static inline bool eol(char c)
{
	return (c == '\n' || c == '\r');
}

static void process_data(data_ctx_t *data_ctx, size_t len, char *chunk)
{
	char *b = chunk, *e = chunk + len;
	char *ln;
	size_t sz;
	bool got_nl;

	do {
		for (e = b; e < chunk + len && !eol(*e); e++) /* nothing */ ;
		// now e points to the character beyond the end of the line
		got_nl = (e < chunk + len);  // Not ran into the end yet
		sz = e - b;
		while (e < chunk + len && eol(*e)) *(e++) = '\0';
		ESP_LOGI(TAG,
			"Slice b=%p, e=%p, end=%p, got_nl=%d, size=%d: %s",
			b, e, chunk + len, got_nl, sz, b?b:"NULL");
		// Now b points to a null-terminated line. Do we gave leftover?
		if (data_ctx->current) {
			ln = data_ctx->buffer;
			if (len) {  // Got anything at all or is is fin?
				if (data_ctx->current + sz
						> data_ctx->capacity) {
					ESP_LOGE(TAG, "Too much data %d", sz);
					memcpy(ln, b, data_ctx->capacity
							- data_ctx->current);
					data_ctx->current = data_ctx->capacity;
				} else {
					memcpy(data_ctx->buffer
						+ data_ctx->current + sz,
						b, sz);
					data_ctx->current += sz;
				}
			}
			// We have reserved an extra byte there
			ln[data_ctx->current] = '\0';
		} else {
			ln = b;
		}
		// Now we have a nul-terminated line `ln`. If it was newline
		// terminated, _or_ came with len == 0 (EVENT_FINISHED),
		// pass it to the function. Otherwise save in the data_ctx.
		if (got_nl || !len) {
			if (ln) {  // FIN could happen with empty save-data
				process_line(data_ctx->index++, ln);
			}
			data_ctx->current = 0;
		} else {  // Don't call process_line, but save it
			if (sz > data_ctx->capacity) {
				ESP_LOGE(TAG, "line longer than buffer %d",
						sz);
				memcpy(data_ctx->buffer, b,
						data_ctx->capacity);
				data_ctx->current = data_ctx->capacity;
			} else {
				memcpy(data_ctx->buffer, b, sz);
				data_ctx->current = sz;
			}
		}
		b = e;
	} while (b < chunk + len);
}

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
	data_ctx_t *data_ctx = (data_ctx_t *)evt->user_data;

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
		ESP_LOGI(TAG, "Event HTTP_EVENT_ON_DATA len=%d",
				evt->data_len);
		ESP_LOGD(TAG, "Event HTTP_EVENT_ON_DATA data=%.*s",
				evt->data_len, evt->data);
		process_data(data_ctx, evt->data_len, evt->data);
		break;
	case HTTP_EVENT_ON_FINISH:
		ESP_LOGI(TAG, "Event HTTP_EVENT_ON_FINISH");
		process_data(data_ctx, 0, NULL);
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

static void httpc_run(void *drawhdl)
{
	char buf[BUFFER_SIZE] = {0};
	data_ctx_t data_ctx = {
		.index = 0,
		.buffer = buf,
		.capacity = BUFFER_SIZE - 1,
		.current = 0,
	};

	ESP_LOGI(TAG, "connecting to %s", URL);
	esp_http_client_handle_t client = esp_http_client_init(
		&(esp_http_client_config_t){
			.url = URL,
			.event_handler = http_event_handler,
			.user_data = &data_ctx,
		}
	);

	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s",
				esp_err_to_name(err));
		draw_main(drawhdl, 0, "", (char *)esp_err_to_name(err));
	}

	esp_http_client_cleanup(client);
	ESP_LOGI(TAG, "Httpc task did the deed and is about to finish");
	finish();
	vTaskDelete(NULL);
}

void httpc(void *drawhdl, void (*finp)(void))
{
	finish = finp;
	TaskHandle_t http_task;
	xTaskCreate(httpc_run, "httpc_task", 4096*2, drawhdl, 0, &http_task);
	ESP_LOGI(TAG, "Httpc task launched");
}
