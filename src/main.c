/*
 * Copyright (c) Cold Bear Ltd
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include <zephyr/pm/policy.h>
#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zigbee/zigbee_app_utils.h>
#include <zigbee/zigbee_error_handler.h>
#include <zb_nrf_platform.h>
#include <zb_zcl_reporting.h>
#include <zb_nrf_platform.h>

#include "ha/zb_ha_temperature_sensor.h"

#include <math.h>

#include <dk_buttons_and_leds.h>
#include <ram_pwrdn.h>

#include "flash.h"
#include "battery.h"

#define BLUE_LED_NODE DT_ALIAS(led2)
#define DIVIDER_POWER_NODE DT_NODELABEL(divider_power)
#define ACTION_BUTTON_NODE DT_ALIAS(sw0)

// The default minimum reporting for temperature is 30 seconds.
//
#define RUN_READ_PROBES_INTERVAL_SEC 30

// The default minimum reporting for temperature is 1 hour.
//
#define RUN_READ_BATTERY_INTERVAL_SEC 5

#define ZCL_TEMPERATURE_MEASUREMENT_MEASURED_VALUE_MULTIPLIER 100

#define INIT_BASIC_POWER_SOURCE ZB_ZCL_BASIC_POWER_SOURCE_BATTERY

#define INIT_BASIC_MANUF_NAME "coldbear"
#define INIT_BASIC_MODEL_ID "FART Sensor"
#define INIT_BASIC_HW_VERSION 11
#define INIT_BASIC_SW_VERSION "v0.1"
#define INIT_BASIC_APP_VERSION 01
#define INIT_BASIC_STACK_VERSION 10

#define FART_ENDPOINT 1
#define PROBE_1_ENDPOINT 5
#define PROBE_2_ENDPOINT 7

#define BATTERY_ADC_CHANNEL 2

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

static const struct adc_dt_spec adc_channels[] = {DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

static const struct adc_dt_spec adc_channels[];

// TODO Move this to configuration, so they can be easily changed.
//
#define THERMISTORNOMINAL 10000
#define TEMPERATURENOMINAL 25
#define BCOEFFICIENT 3977
#define SERIESRESISTOR 10000

static const struct gpio_dt_spec blue_led = GPIO_DT_SPEC_GET(BLUE_LED_NODE, gpios);
static const struct gpio_dt_spec divider_power = GPIO_DT_SPEC_GET(DIVIDER_POWER_NODE, gpios);

LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

typedef struct
{
	zb_uint8_t battery_voltage;
	zb_uint8_t battery_size;
	zb_uint8_t battery_quantity;
	zb_uint8_t battery_rated_voltage;
	zb_uint8_t battery_alarm_mask;
	zb_uint8_t battery_voltage_min_threshold;
	zb_uint8_t battery_percentage_remaining;
	zb_uint8_t battery_voltage_threshold1;
	zb_uint8_t battery_voltage_threshold2;
	zb_uint8_t battery_voltage_threshold3;
	zb_uint8_t battery_percentage_min_threshold;
	zb_uint8_t battery_percentage_threshold1;
	zb_uint8_t battery_percentage_threshold2;
	zb_uint8_t battery_percentage_threshold3;
	zb_uint32_t battery_alarm_state;

} zb_zcl_power_config_battery_attrs_t;

struct zb_device_ctx
{
	zb_zcl_basic_attrs_ext_t basic_attr;
	zb_zcl_identify_attrs_t identify_attr;
	zb_zcl_temp_measurement_attrs_t probe_1_measure_attrs;
	zb_zcl_temp_measurement_attrs_t probe_2_measure_attrs;
	zb_zcl_power_config_battery_attrs_t power_config_attrs;
};

static struct zb_device_ctx dev_ctx;

/** A discharge curve specific to the power source. */
static const struct battery_level_point levels[] = {
	/* "Curve" here eyeballed from captured data for the [Adafruit
	 * 3.7v 2000 mAh](https://www.adafruit.com/product/2011) LIPO
	 * under full load that started with a charge of 3.96 V and
	 * dropped about linearly to 3.58 V over 15 hours.  It then
	 * dropped rapidly to 3.10 V over one hour, at which point it
	 * stopped transmitting.
	 *
	 * Based on eyeball comparisons we'll say that 15/16 of life
	 * goes between 3.95 and 3.55 V, and 1/16 goes between 3.55 V
	 * and 3.1 V.
	 */

	{10000, 3950},
	{625, 3550},
	{0, 3100},
};

//------------
// Prototypes
//

void zboss_signal_handler(zb_bufid_t bufid);
static void zcl_device_cb(zb_bufid_t bufid);
static void configure_gpio(void);
static void start_identifying(zb_bufid_t bufid);
static void app_clusters_attr_init(void);
static void start_identifying(zb_bufid_t bufid);
static void identify_cb(zb_bufid_t bufid);
static void button_handler(uint32_t button_state, uint32_t has_changed);
static void slowly_toggle_identify_led(zb_bufid_t bufid);
static void read_battery();
static void read_probes();

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(
	identify_attr_list,
	&dev_ctx.identify_attr.identify_time);

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(
	ext_attr_list,
	&dev_ctx.basic_attr.zcl_version,
	&dev_ctx.basic_attr.app_version,
	&dev_ctx.basic_attr.stack_version,
	&dev_ctx.basic_attr.hw_version,
	&dev_ctx.basic_attr.mf_name,
	&dev_ctx.basic_attr.model_id,
	&dev_ctx.basic_attr.zcl_version,
	&dev_ctx.basic_attr.power_source,
	&dev_ctx.basic_attr.location_id,
	&dev_ctx.basic_attr.ph_env,
	&dev_ctx.basic_attr.sw_ver);

// This must be declared for compilate to succeed.
// https://devzone.nordicsemi.com/f/nordic-q-a/85315/zboss-declare-power-config-attribute-list-for-battery-bat_num
#define bat_num

ZB_ZCL_DECLARE_POWER_CONFIG_BATTERY_ATTRIB_LIST_EXT(power_config_attr_list,
													&dev_ctx.power_config_attrs.battery_voltage,
													&dev_ctx.power_config_attrs.battery_size,
													&dev_ctx.power_config_attrs.battery_quantity,
													&dev_ctx.power_config_attrs.battery_rated_voltage,
													&dev_ctx.power_config_attrs.battery_alarm_mask,
													&dev_ctx.power_config_attrs.battery_voltage_min_threshold,
													&dev_ctx.power_config_attrs.battery_percentage_remaining,
													&dev_ctx.power_config_attrs.battery_voltage_threshold1,
													&dev_ctx.power_config_attrs.battery_voltage_threshold2,
													&dev_ctx.power_config_attrs.battery_voltage_threshold3,
													&dev_ctx.power_config_attrs.battery_percentage_min_threshold,
													&dev_ctx.power_config_attrs.battery_percentage_threshold1,
													&dev_ctx.power_config_attrs.battery_percentage_threshold2,
													&dev_ctx.power_config_attrs.battery_percentage_threshold3,
													&dev_ctx.power_config_attrs.battery_alarm_state);

ZB_ZCL_DECLARE_FART_SENSOR_CLUSTER_LIST(fart_sensor_clusters, ext_attr_list, identify_attr_list, power_config_attr_list);
ZB_HA_DECLARE_FART_SENSOR_EP(fart_sensor_ep, FART_ENDPOINT, fart_sensor_clusters);

ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_measurement_attr_list_1,
											&dev_ctx.probe_1_measure_attrs.measure_value,
											&dev_ctx.probe_1_measure_attrs.min_measure_value,
											&dev_ctx.probe_1_measure_attrs.max_measure_value,
											&dev_ctx.probe_1_measure_attrs.tolerance);

ZB_HA_DECLARE_TEMPERATURE_SENSOR_CLUSTER_LIST(temperature_sensor_clusters_1, temp_measurement_attr_list_1);
ZB_HA_DECLARE_TEMPERATURE_SENSOR_EP_1(temperature_sensor_ep_one, PROBE_1_ENDPOINT, temperature_sensor_clusters_1);

ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_measurement_attr_list_2,
											&dev_ctx.probe_2_measure_attrs.measure_value,
											&dev_ctx.probe_2_measure_attrs.min_measure_value,
											&dev_ctx.probe_2_measure_attrs.max_measure_value,
											&dev_ctx.probe_2_measure_attrs.tolerance);

ZB_HA_DECLARE_TEMPERATURE_SENSOR_CLUSTER_LIST(temperature_sensor_clusters_2, temp_measurement_attr_list_2);
ZB_HA_DECLARE_TEMPERATURE_SENSOR_EP_2(temperature_sensor_ep_two, PROBE_2_ENDPOINT, temperature_sensor_clusters_2);

ZBOSS_DECLARE_DEVICE_CTX_3_EP(device_ctx, fart_sensor_ep, temperature_sensor_ep_one, temperature_sensor_ep_two);

// Taken from https://forum.seeedstudio.com/t/low-power-with-xiao-nrf52840-on-zephyr-rtos/270491/5
/* Prevent deep sleep (system off) from being entered on long timeouts
 * or `K_FOREVER` due to the default residency policy.
 *
 * This has to be done before anything tries to sleep, which means
 * before the threading system starts up between PRE_KERNEL_2 and
 * POST_KERNEL.  Do it at the start of PRE_KERNEL_2.
 */
static int disable_ds_1(const struct device *dev)
{
	ARG_UNUSED(dev);

	pm_policy_state_lock_get(PM_STATE_SOFT_OFF, PM_ALL_SUBSTATES);
	return 0;
}

SYS_INIT(disable_ds_1, PRE_KERNEL_2, 0);

//---------------------------------------------------------------------------------------------
// main
//

int main(void)
{
	LOG_INF("Starting Zigbee FART Sensor");

	int rc;

	// Taken from https://forum.seeedstudio.com/t/low-power-with-xiao-nrf52840-on-zephyr-rtos/270491/5
	// This turns off the QSPI flash.
	//
	da_flash_init();
	da_flash_command(0xB9);
	da_flash_uninit();

	configure_gpio();

	rc = battery_measure_enable();

	if (rc != 0)
	{
		LOG_ERR("Failed initialize battery measurement: %d", rc);
		return 0;
	}

	// WTF does "1" represent? It's the button's "state" value. The mask?
	//
	register_factory_reset_button(4);

	zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);

	zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(60000));

	zb_set_rx_on_when_idle(ZB_FALSE);

	ZB_ZCL_REGISTER_DEVICE_CB(zcl_device_cb);

	ZB_AF_REGISTER_DEVICE_CTX(&device_ctx);

	app_clusters_attr_init();

	ZB_AF_SET_IDENTIFY_NOTIFICATION_HANDLER(FART_ENDPOINT, identify_cb);

	zb_bdb_set_legacy_device_support(1);

	zigbee_configure_sleepy_behavior(true);

	power_down_unused_ram();

	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++)
	{
		if (!device_is_ready(adc_channels[i].dev))
		{
			LOG_ERR("ADC controller device not ready");
			return -1;
		}

		rc = adc_channel_setup_dt(&adc_channels[i]);

		if (rc < 0)
		{
			LOG_ERR("Could not setup channel #%d (%d)", i, rc);
			return -1;
		}

		LOG_INF("Setup channel #%d (%d)", i, rc);
	}

	// nrf_802154_tx_power_set(8);

	zigbee_enable();

	// Start the LED slowly blinking
	//
	// ZB_SCHEDULE_APP_CALLBACK(slowly_toggle_identify_led, ZB_ALARM_ANY_PARAM);

	// Zigbee will take it from here. Put this thread to sleep forever
	//
	while (1)
	{
		LOG_INF("Everything is setup. Main thread will now go to sleep");
		k_sleep(K_FOREVER);
	}
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
	LOG_INF("button_handler: %d", button_state);

	check_factory_reset_button(button_state, has_changed);

	user_input_indicate();

	// Maybe turn the light green
	// or indicate the active probe?? Red mean probe 1 is flow, green means probe 2 is flow?
}

static void app_clusters_attr_init(void)
{
	dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
	dev_ctx.basic_attr.power_source = INIT_BASIC_POWER_SOURCE;
	dev_ctx.basic_attr.stack_version = INIT_BASIC_STACK_VERSION;
	dev_ctx.basic_attr.hw_version = INIT_BASIC_HW_VERSION;
	dev_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;
	dev_ctx.basic_attr.ph_env = ZB_ZCL_BASIC_ENV_UNSPECIFIED;

	dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;

	ZB_ZCL_SET_STRING_VAL(
		dev_ctx.basic_attr.mf_name,
		INIT_BASIC_MANUF_NAME,
		ZB_ZCL_STRING_CONST_SIZE(INIT_BASIC_MANUF_NAME));

	ZB_ZCL_SET_STRING_VAL(
		dev_ctx.basic_attr.model_id,
		INIT_BASIC_MODEL_ID,
		ZB_ZCL_STRING_CONST_SIZE(INIT_BASIC_MODEL_ID));

	// Set the measurements to zero.
	//
	dev_ctx.probe_1_measure_attrs.measure_value = 0;
	dev_ctx.probe_2_measure_attrs.measure_value = 0;

	ZB_ZCL_SET_ATTRIBUTE(PROBE_1_ENDPOINT,
						 ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
						 ZB_ZCL_CLUSTER_SERVER_ROLE,
						 ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
						 (zb_uint8_t *)&dev_ctx.probe_1_measure_attrs.measure_value,
						 ZB_FALSE);

	ZB_ZCL_SET_ATTRIBUTE(PROBE_2_ENDPOINT,
						 ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
						 ZB_ZCL_CLUSTER_SERVER_ROLE,
						 ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
						 (zb_uint8_t *)&dev_ctx.probe_2_measure_attrs.measure_value,
						 ZB_FALSE);

	// Set battery measurements
	//
	dev_ctx.power_config_attrs.battery_voltage = ZB_ZCL_POWER_CONFIG_BATTERY_VOLTAGE_INVALID;
	dev_ctx.power_config_attrs.battery_percentage_remaining = ZB_ZCL_POWER_CONFIG_BATTERY_REMAINING_UNKNOWN;
	dev_ctx.power_config_attrs.battery_quantity = 1;
	dev_ctx.power_config_attrs.battery_size = ZB_ZCL_POWER_CONFIG_BATTERY_SIZE_DEFAULT_VALUE;
}

static void configure_gpio(void)
{
	int err;

	err = dk_buttons_init(button_handler);
	if (err)
	{
		LOG_ERR("Cannot init buttons (err: %d)", err);
		return;
	}

	if (!gpio_is_ready_dt(&blue_led))
	{
		LOG_ERR("Blue LED not ready");
		return;
	}

	err = gpio_pin_configure_dt(&blue_led, GPIO_OUTPUT_ACTIVE);
	if (err != 0)
	{
		LOG_ERR("Cannot configure Blue LED (err: %d)", err);
		return;
	}

	if (!gpio_is_ready_dt(&divider_power))
	{
		LOG_ERR("Cannot configure Divider Power switch (err: %d)", err);
		return;
	}

	err = gpio_pin_configure_dt(&divider_power, GPIO_OUTPUT_INACTIVE);
	if (err != 0)
	{
		LOG_ERR("Configuring Divider Power pin failed (err: %d)", err);
		return;
	}
}

/* Rapidly blink an LED */
static void toggle_identify_led(zb_bufid_t bufid)
{
	gpio_pin_toggle_dt(&blue_led);
	ZB_SCHEDULE_APP_ALARM(toggle_identify_led, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(100));
}

/* Slowly blink an LED */
static void slowly_toggle_identify_led(zb_bufid_t bufid)
{
	gpio_pin_toggle_dt(&blue_led);
	ZB_SCHEDULE_APP_ALARM(slowly_toggle_identify_led, bufid, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000));
}

/* Invoked by ZBOSS. This will be called again with no buffer to turn off the identify mode */
static void identify_cb(zb_bufid_t bufid)
{
	zb_ret_t zb_err_code;

	if (bufid)
	{
		ZB_SCHEDULE_APP_CALLBACK(toggle_identify_led, bufid);
	}
	else
	{
		zb_err_code = ZB_SCHEDULE_APP_ALARM_CANCEL(toggle_identify_led, ZB_ALARM_ANY_PARAM);
		ZVUNUSED(zb_err_code);

		// Ensure the LED is turned off when idenify is done.
		//
		gpio_pin_set_dt(&blue_led, 0);
	}
}

static void start_identifying(zb_bufid_t bufid)
{
	LOG_INF("start_identifying() has been called!");

	ZVUNUSED(bufid);

	if (ZB_JOINED())
	{
		/* Check if endpoint is in identifying mode,
		 * if not put desired endpoint in identifying mode.
		 */
		if (dev_ctx.identify_attr.identify_time == ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE)
		{
			zb_ret_t zb_err_code = zb_bdb_finding_binding_target(PROBE_1_ENDPOINT);

			if (zb_err_code == RET_OK)
			{
				LOG_INF("Enter identify mode");
			}
			else if (zb_err_code == RET_INVALID_STATE)
			{
				LOG_WRN("RET_INVALID_STATE - Cannot enter identify mode");
			}
			else
			{
				ZB_ERROR_CHECK(zb_err_code);
			}
		}
		else
		{
			LOG_INF("Cancel identify mode");
			zb_bdb_finding_binding_target_cancel();
		}
	}
	else
	{
		LOG_WRN("Device not in a network - cannot enter identify mode");
	}
}

void zboss_signal_handler(zb_uint8_t param)
{
	zb_zdo_app_signal_hdr_t *sig_hndler = NULL;
	zb_zdo_app_signal_type_t sig = zb_get_app_signal(param, &sig_hndler);
	zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(param);

	LOG_INF("ZB signal (%u, status: %d)", sig, status);

	zigbee_default_signal_handler(param);

	switch (sig)
	{
	case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
	case ZB_BDB_SIGNAL_DEVICE_REBOOT:
	{
		LOG_INF("Device starting or rebooting");

		// All connected!
		//
		// ZB_SCHEDULE_APP_ALARM_CANCEL(slowly_toggle_identify_led, ZB_ALARM_ANY_PARAM);

		// Ensure the LED is turned off
		//
		gpio_pin_set_dt(&blue_led, 0);
	}
	break;
	case ZB_DEVICE_LEFT:
	{
		LOG_INF("Device has left the network");

		// We have been cut loose. Blink the light
		//
		// ZB_SCHEDULE_APP_CALLBACK(slowly_toggle_identify_led, ZB_ALARM_ANY_PARAM);
	}
	break;
	case ZB_COMMON_SIGNAL_CAN_SLEEP:
	{
		zb_zdo_signal_can_sleep_params_t *can_sleep_params = ZB_ZDO_SIGNAL_GET_PARAMS(sig_hndler, zb_zdo_signal_can_sleep_params_t);
		LOG_INF("Device can sleep for %d ms", can_sleep_params->sleep_tmo);
		zb_sleep_now();
	}
	break;
	}

	zb_buf_free(param);
}

static void zcl_device_cb(zb_bufid_t bufid)
{
	zb_zcl_device_callback_param_t *device_cb_param = ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t);

	LOG_INF("%s id %hd", __func__, device_cb_param->device_cb_id);

	device_cb_param->status = RET_OK;
}

int16_t buf;

struct adc_sequence sequence = {
	.buffer = &buf,
	.buffer_size = sizeof(buf),
	.calibrate = true,
};

// The ADC reads to a max of 3v and is configured with 4096 levels.
//
float mv_per_lsb = 3000.0F / 4096.0F; // GAIN 0.6 / (1/5)
float batt_mv_per_lsb = 3600.0F / 4096.0F; // GAIN 0.6 / (1/6)

static const float               calib_factor = 1510.0 / 510.0;

static uint16_t read_temperature(int i)
{
	int err = adc_sequence_init_dt(&adc_channels[i], &sequence);

	if (err < 0)
	{
		LOG_ERR("Could initialise ADC%d (%d)", i, err);
		return -1;
	}

	err = adc_read(adc_channels[i].dev, &sequence);

	if (err < 0)
	{
		LOG_ERR("Could not read ADC%d (%d)", i, err);
		return -1;
	}

	int32_t adc_reading = buf;

	int32_t val_mv = (float)adc_reading * mv_per_lsb;

	float reading = (val_mv * SERIESRESISTOR) / (CONFIG_REFERENCE_VOLTAGE - val_mv);

	double steinhart;
	steinhart = reading / THERMISTORNOMINAL;		  // (R/Ro)
	steinhart = log(steinhart);						  // ln(R/Ro)
	steinhart /= BCOEFFICIENT;						  // 1/B * ln(R/Ro)
	steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
	steinhart = 1.0 / steinhart;					  // Invert
	steinhart -= 273.15;							  // Convert to Celcius

	uint16_t value = (uint16_t)(steinhart);

	LOG_INF("ADC CHANNEL %d", i);
	LOG_INF("A: %d", adc_reading);
	LOG_INF("V: %d", val_mv);
	LOG_INF("R: %d", (int)reading);
	LOG_INF("T: %d", value);

	return value;
}

static void read_battery()
{
	// Channel 2 is the battery ADC.
	//
	int err = adc_sequence_init_dt(&adc_channels[BATTERY_ADC_CHANNEL], &sequence);

	if (err < 0)
	{
		LOG_ERR("Could initialise ADC%d (%d)", BATTERY_ADC_CHANNEL, err);
		return -1;
	}

	err = adc_read(adc_channels[BATTERY_ADC_CHANNEL].dev, &sequence);

	if (err < 0)
	{
		LOG_ERR("Could not read ADC%d (%d)", BATTERY_ADC_CHANNEL, err);
		return -1;
	}

	int32_t adc_reading = buf;

	int32_t batt_mV = (float)adc_reading * batt_mv_per_lsb;

	batt_mV = batt_mV * calib_factor;

	zb_uint8_t batt_pptt = battery_level_pptt(batt_mV, levels);

	LOG_DBG("Battery: %d; %d mV; %u pptt", adc_reading, batt_mV, batt_pptt);

	zb_zcl_status_t status = zb_zcl_set_attr_val(FART_ENDPOINT,
												  ZB_ZCL_CLUSTER_ID_POWER_CONFIG,
												  ZB_ZCL_CLUSTER_SERVER_ROLE,
												  ZB_ZCL_ATTR_POWER_CONFIG_BATTERY_PERCENTAGE_REMAINING_ID,
												  &batt_pptt,
												  ZB_FALSE);

	if (status != ZB_ZCL_STATUS_SUCCESS)
	{
		LOG_ERR("Failed to write to Battery Percentage Remaining Attribute");
	}
}

static void read_battery_loop(zb_uint8_t arg)
{
	LOG_DBG("Reading battery & updating power cluster...");

	while (1)
	{
		read_battery();

		k_sleep(K_MSEC(RUN_READ_BATTERY_INTERVAL_SEC * 1000));
	}
}

static void read_probes()
{
	// Switch on the power pin.
	//
	gpio_pin_set_dt(&divider_power, 1);

	// Let the voltage stabalise??.
	//
	k_sleep(K_MSEC(100));

	uint16_t temperature_1 = read_temperature(0);
	uint16_t temperature_2 = read_temperature(1);

	// Switch off the power pin.
	//
	gpio_pin_set_dt(&divider_power, 0);

	int16_t temperature_attribute_1 = (int16_t)(temperature_1 * ZCL_TEMPERATURE_MEASUREMENT_MEASURED_VALUE_MULTIPLIER);

	zb_zcl_status_t status1 = zb_zcl_set_attr_val(PROBE_1_ENDPOINT,
												  ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
												  ZB_ZCL_CLUSTER_SERVER_ROLE,
												  ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
												  (zb_uint8_t *)&temperature_attribute_1,
												  ZB_FALSE);

	if (status1 != ZB_ZCL_STATUS_SUCCESS)
	{
		LOG_ERR("Failed to write to Endpoint 1 Attribute");
	}

	int16_t temperature_attribute_2 = (int16_t)(temperature_2 * ZCL_TEMPERATURE_MEASUREMENT_MEASURED_VALUE_MULTIPLIER);

	zb_zcl_status_t status2 = zb_zcl_set_attr_val(PROBE_2_ENDPOINT,
												  ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
												  ZB_ZCL_CLUSTER_SERVER_ROLE,
												  ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
												  (zb_uint8_t *)&temperature_attribute_2,
												  ZB_FALSE);

	if (status2 != ZB_ZCL_STATUS_SUCCESS)
	{
		LOG_ERR("Failed to write to Endpoint 2 Attribute");
	}
}

void read_probes_loop()
{
	while (1)
	{
		if (ZB_JOINED())
		{
			read_probes();
		}

		k_sleep(K_MSEC(RUN_READ_PROBES_INTERVAL_SEC * 1000));
	}
}

#define STACKSIZE 1024
#define PRIORITY 7

// Start the probes thread. This means the readings won't be taken straight away
//
//
K_THREAD_DEFINE(read_probes_id, STACKSIZE, read_probes_loop, NULL, NULL, NULL, PRIORITY, 0, 0);

// TODO Start the battery monitor thread
//
K_THREAD_DEFINE(read_battery_id, STACKSIZE, read_battery_loop, NULL, NULL, NULL, PRIORITY, 0, 0);