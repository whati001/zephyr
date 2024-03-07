/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/device.h>

#define SLEEP_TIME_MS 5000

// const struct device *uart0_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));

	printk("Device ready\n");
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set(led.port, led.pin, (int)1);
	// gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);
	k_msleep(5000);
	gpio_pin_set(led.port, led.pin, (int)0);
	// gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);
	// printk("uart disable: %d", pm_device_action_run(uart0_dev, PM_DEVICE_ACTION_SUSPEND));

	k_msleep(1000);
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set(led.port, led.pin, (int)1);
	gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);

	while (true) {
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
