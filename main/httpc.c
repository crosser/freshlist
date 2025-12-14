#include <esp_log.h>
#include <esp_http_client.h>
// #include <esp_tls.h>
#include "httpc.h"
#include "display.h"

#define TAG "httpc"

void httpc(void *drawhdl)
{
	char buf[1024] = {0};

	draw(drawhdl, "HTTP client called");
	esp_http_client_handle_t client = esp_http_client_init(
		&(esp_http_client_config_t){
			.url = "https://par.average.org/latest.txt",
			.user_data = buf,
		}
	);


	// (esp_tls_cfg_t){.skip_common_name = true,}

	esp_err_t err = esp_http_client_perform(client);
	if (err == ESP_OK) {
		ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %"PRId64,
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
		ESP_LOGI(TAG, "got \"%s\"", buf);
	} else {
		ESP_LOGE(TAG, "HTTP GET request failed: %s",
				esp_err_to_name(err));
	}

	esp_http_client_cleanup(client);
}
