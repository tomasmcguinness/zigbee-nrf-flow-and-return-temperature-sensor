/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

#include "battery.h"

LOG_MODULE_REGISTER(BATTERY, CONFIG_ADC_LOG_LEVEL);

// #define VBATT DT_PATH(vbatt)
// #define ZEPHYR_USER DT_PATH(zephyr_user)

// #define BATTERY_ADC_GAIN ADC_GAIN_1_6

// struct io_channel_config
// {
// 	uint8_t channel;
// };

// struct divider_config
// {
// 	struct io_channel_config io_channel;
// 	struct gpio_dt_spec power_gpios;
// 	uint32_t output_ohm;
// 	uint32_t full_ohm;
// };

// static const struct divider_config divider_config = {
// 	.io_channel = {
// 		DT_IO_CHANNELS_INPUT(VBATT),
// 	},
// 	.power_gpios = GPIO_DT_SPEC_GET_OR(VBATT, power_gpios, {}),
// 	.output_ohm = DT_PROP(VBATT, output_ohms),
// 	.full_ohm = DT_PROP(VBATT, full_ohms),
// };

// struct divider_data
// {
// 	const struct device *adc;
// 	struct adc_channel_cfg adc_cfg;
// 	struct adc_sequence adc_seq;
// 	int16_t raw;
// };

// static struct divider_data divider_data = {
// 	.adc = DEVICE_DT_GET(DT_IO_CHANNELS_CTLR(VBATT)),
// };

// static int divider_setup(void)
// {
// 	const struct divider_config *cfg = &divider_config;
// 	const struct io_channel_config *iocp = &cfg->io_channel;
// 	const struct gpio_dt_spec *gcp = &cfg->power_gpios;
// 	struct divider_data *ddp = &divider_data;
// 	struct adc_sequence *asp = &ddp->adc_seq;
// 	struct adc_channel_cfg *accp = &ddp->adc_cfg;
	
// 	int rc;

// 	if (!device_is_ready(ddp->adc))
// 	{
// 		LOG_ERR("ADC device is not ready %s", ddp->adc->name);
// 		return -ENOENT;
// 	}

// 	LOG_INF("ADC device is ready %s", ddp->adc->name);

// 	if (gcp->port)
// 	{
// 		if (!device_is_ready(gcp->port))
// 		{
// 			LOG_ERR("%s: device not ready", gcp->port->name);
// 			return -ENOENT;
// 		}

// 		rc = gpio_pin_configure_dt(gcp, GPIO_OUTPUT_INACTIVE);

// 		if (rc != 0)
// 		{
// 			LOG_ERR("Failed to control feed %s.%u: %d",	gcp->port->name, gcp->pin, rc);
// 			return rc;
// 		}

// 		LOG_INF("GPIO sink is ready %s", ddp->adc->name);
// 	}

// 	// TODO We need to get an instance of the adc_spec_dt here to setup.
// 	// The channel. 
// 	// The battery sample code sets the channel up manually!
// 	//
// 	//rc = adc_channel_setup_dt(&iocp->channel);

// 	*asp = (struct adc_sequence){
// 		.channels = BIT(2),
// 		.buffer = &ddp->raw,
// 		.buffer_size = sizeof(ddp->raw),
// 		.resolution = 8,
// 		.oversampling = 4,
// 		.calibrate = true
// 	};

// 	*accp = (struct adc_channel_cfg){
// 		.gain = BATTERY_ADC_GAIN,
// 		.reference = ADC_REF_INTERNAL,
// 		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40),
// 		.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput2 + iocp->channel
// 	};

// 	rc = adc_channel_setup(ddp->adc, accp);
// 	rc = adc_sequence_init_dt(&ddp->adc, asp);
	
// 	LOG_INF("Setup ADC%u got %d", iocp->channel, rc);

// 	return rc;
// }

// // static bool battery_ok;

// // static int battery_setup(void)
// // {
// // 	int rc = divider_setup();

// // 	battery_ok = (rc == 0);

// // 	LOG_INF("Battery setup: %d %d", rc, battery_ok);

// // 	return rc;
// // }

// // SYS_INIT(battery_setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);


static const struct gpio_dt_spec read_batt = GPIO_DT_SPEC_GET(DT_ALIAS(readbatt), gpios);

int battery_measure_enable()
{
	gpio_pin_configure_dt(&read_batt, GPIO_OUTPUT_ACTIVE);
    gpio_pin_set_dt(&read_batt, 1);

	return 0;
}

// 	// int rc = -ENOENT;

// 	// if (battery_ok)
// 	// {
// 	// 	const struct gpio_dt_spec *gcp = &divider_config.power_gpios;

// 	// 	rc = 0;

// 	// 	if (gcp->port)
// 	// 	{
// 	// 		// The GPIO sink, which sets it to ground.
// 	// 		//
// 	// 		rc = gpio_pin_set_dt(gcp, 0);
// 	// 	}
// 	// }

// 	// return rc;
// }

int battery_sample(void)
{
	int rc = -ENOENT;

	// if (battery_ok)
	// {
	// 	LOG_INF("Divider is configured. Taking a voltage reading...");

	// 	struct divider_data *ddp = &divider_data;
	// 	const struct divider_config *dcp = &divider_config;
	// 	struct adc_sequence *sp = &ddp->adc_seq;

	// 	rc = adc_read(ddp->adc, sp);
		 
	// 	sp->calibrate = false;

	// 	if (rc == 0)
	// 	{
	// 		int32_t val = ddp->raw;

	// 		adc_raw_to_millivolts(adc_ref_internal(ddp->adc),
	// 							  ddp->adc_cfg.gain,
	// 							  sp->resolution,
	// 							  &val);

	// 		if (dcp->output_ohm != 0)
	// 		{
	// 			rc = val * (uint64_t)dcp->full_ohm / dcp->output_ohm;
	// 			LOG_INF("raw %u ~ %u mV => %d mV", ddp->raw, val, rc);
	// 		}
	// 		else
	// 		{
	// 			rc = val;
	// 			LOG_INF("raw %u ~ %u mV", ddp->raw, val);
	// 		}
	// 	}
	// }

	return rc;
}

unsigned int battery_level_pptt(unsigned int batt_mV, const struct battery_level_point *curve)
{
	const struct battery_level_point *pb = curve;

	if (batt_mV >= pb->lvl_mV)
	{
		/* Measured voltage above highest point, cap at maximum. */
		return pb->lvl_pptt;
	}
	/* Go down to the last point at or below the measured voltage. */
	while ((pb->lvl_pptt > 0) && (batt_mV < pb->lvl_mV))
	{
		++pb;
	}
	if (batt_mV < pb->lvl_mV)
	{
		/* Below lowest point, cap at minimum */
		return pb->lvl_pptt;
	}

	/* Linear interpolation between below and above points. */
	const struct battery_level_point *pa = pb - 1;

	return pb->lvl_pptt + ((pa->lvl_pptt - pb->lvl_pptt) * (batt_mV - pb->lvl_mV) / (pa->lvl_mV - pb->lvl_mV));
}
