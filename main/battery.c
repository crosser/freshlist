#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali_scheme.h>

#include "sdkconfig.h"
#include "battery.h"

#define TAG "BATTERY"

/*
 https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.49/ \
   blob/49582d4c8e7e945689e4078c4240144b3b681217/ \
   Examples/ESP-IDF/01_ADC_Test/components/adc_bsp/adc_bsp.c
 */

static adc_channel_t channel;
static adc_oneshot_unit_handle_t handle;
static adc_cali_handle_t cali;

void init_battery_adc(void)
{
	adc_unit_t unit;

	ESP_ERROR_CHECK(adc_oneshot_io_to_channel(CONFIG_HWE_BATTERY_ADC,
				&unit, &channel));
	ESP_LOGI(TAG, "GPIO %d gave us unit %d (exp. %d) chan %d (exp %d)",
			CONFIG_HWE_BATTERY_ADC, unit, ADC_UNIT_1,
			channel, ADC_CHANNEL_3);
	ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(
			&(adc_cali_curve_fitting_config_t){
				.unit_id = unit,
				.bitwidth = ADC_BITWIDTH_DEFAULT,
				.atten = ADC_ATTEN_DB_12,
			},
			&cali));
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&(adc_oneshot_unit_init_cfg_t){
				.unit_id = unit,
				.ulp_mode = ADC_ULP_MODE_DISABLE,  // redundant
			},
			&handle));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(handle,
			channel,
			&(adc_oneshot_chan_cfg_t){
				.bitwidth = ADC_BITWIDTH_DEFAULT,
				.atten = ADC_ATTEN_DB_12,
			}));
}

int battery(void)
{
	int value = 0;
	ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(
			handle, cali, channel, &value));
	ESP_LOGI(TAG, "Calibrated result: %d", value);
	// Doc promises the value in millivolt. Assuming that we use
	// a typical Lithium Ion battery, working range is between
	// 3.0 and 4.0 V. ADC is measuring 1/2 of the battery voltage.
	// Consequently, the range between 1500 mV and 2000 mV can be
	// considered 0% to 100% of the charge.
	if (value < 1500) return 0;
	return (value - 1500) / 5;
	// result = 0.001 * value * 3; in the example. Is the divisor
	// 2 or 3? ... Hmmm.
}
