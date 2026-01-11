#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali_scheme.h>
#include <driver/gpio.h>

#include "sdkconfig.h"
#include "battery.h"

#define TAG "BATTERY"

/*
 * https://docs.espressif.com/projects/esp-idf/en/release-v6.0/ \
 * esp32s3/api-reference/peripherals/adc/index.html
 *
 * By design, Vref is set to 1100 mV.
 * The ADC can measure analog voltages from 0 V to Vref. \
 * To measure higher voltages, input signals can be attenuated \
 * before being passed to the ADC.
 *
 * With attenuation 12 dB, upper limit ought to be 4.4V. So it should
 * be all right to feed battery voltage directly to the pin, as long
 * as maximum attenuation is set. Am I right? Am I right?
 */

/*
 https://github.com/waveshareteam/ESP32-S3-Touch-LCD-3.49/ \
   blob/49582d4c8e7e945689e4078c4240144b3b681217/ \
   Examples/ESP-IDF/01_ADC_Test/components/adc_bsp/adc_bsp.c
 */

static adc_channel_t channel;
static adc_oneshot_unit_handle_t handle;
static adc_cali_handle_t cali;
static bool nobatt = false;

static void init_adc(int pin)
{
	adc_unit_t unit;

	ESP_ERROR_CHECK(adc_oneshot_io_to_channel(pin, &unit, &channel));
	ESP_LOGI(TAG, "GPIO %d gave us unit %d chan %d", pin, unit, channel);
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

void init_battery_adc(void)
{
	int pins[] = {CONFIG_HWE_BATTERY_ADC_1, CONFIG_HWE_BATTERY_ADC_2};
	int i, value = 0;

	for (i = 0; i < sizeof(pins) / sizeof(int); i++) {
		init_adc(pins[i]);
		ESP_ERROR_CHECK(gpio_set_pull_mode(pins[i],
					GPIO_PULLDOWN_ONLY));
		ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(
				handle, cali, channel, &value));
		ESP_ERROR_CHECK(gpio_set_pull_mode(pins[i], GPIO_FLOATING));
		if (value > 100) break;
		ESP_ERROR_CHECK(adc_oneshot_del_unit(handle));
	}
	if (i < sizeof(pins) / sizeof(int)) {
		ESP_LOGI(TAG, "Using pin %d: value with pulldown: %d",
				pins[i], value);
	} else {
		ESP_LOGI(TAG, "Could not find battery ADC");
		nobatt = true;
	}
}

int battery(void)
{
	if (nobatt) return 0;

	int value = 0;
	ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(
			handle, cali, channel, &value));
	ESP_LOGI(TAG, "Calibrated result: %d", value);
	return value;
}
