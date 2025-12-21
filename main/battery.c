#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali_scheme.h>

#include "sdkconfig.h"
#include "battery.h"

#define TAG "BATTERY"

static adc_channel_t channel;
static adc_oneshot_unit_handle_t handle;
static adc_cali_handle_t cali;

void init_battery_adc(void)
{
	adc_unit_t unit;

	ESP_ERROR_CHECK(adc_oneshot_io_to_channel(CONFIG_HWE_BATTERY_ADC,
				&unit, &channel));
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&(adc_oneshot_unit_init_cfg_t){
				.unit_id = unit,
				.ulp_mode = ADC_ULP_MODE_DISABLE,
			},
			&handle));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(handle,
			channel,
			&(adc_oneshot_chan_cfg_t){
				.bitwidth = ADC_BITWIDTH_DEFAULT,
				.atten = ADC_ATTEN_DB_12,
			}));
	ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(
			&(adc_cali_curve_fitting_config_t){
				.unit_id = unit,
				.bitwidth = ADC_BITWIDTH_DEFAULT,
				.atten = ADC_ATTEN_DB_12,
			},
			&cali));
}

int battery(void)
{
	int value = 0;
	ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(
			handle, cali, channel, &value));
	ESP_LOGI(TAG, "Calibrated result: %d", value);
	return (value - 1500) / 5;
}
