/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sys/poweroff.h>
#include <stm32_ll_pwr.h>

#define STACKSIZE     1024
#define PRIORITY      7
#define SLEEP_TIME_MS 1000

/* Semaphore used to control button pressed value */

static int led_is_on;
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
	uint32_t cause;

	hwinfo_get_reset_cause(&cause);
	hwinfo_clear_reset_cause();

	if (cause == RESET_LOW_POWER_WAKE) {
		hwinfo_clear_reset_cause();
		printk("\nReset cause: Standby mode\n\n");
	}

	if (cause == (RESET_PIN | RESET_BROWNOUT)) {
		printk("\nReset cause: Shutdown mode or power up\n\n");
	}

	if (cause == RESET_PIN) {
		printk("\nReset cause: Reset pin\n\n");
	}

	uint8_t idx = 0;
	led_is_on = true;
	while (idx++ < 5) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set(led.port, led.pin, (int)led_is_on);
		k_msleep(SLEEP_TIME_MS);
		led_is_on = !led_is_on;
	}

	sys_poweroff();

	idx = 0;
	led_is_on = true;
	while (idx++ < 5) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set(led.port, led.pin, (int)led_is_on);
		k_msleep(SLEEP_TIME_MS);
		led_is_on = !led_is_on;
	}

	return 0;
}
