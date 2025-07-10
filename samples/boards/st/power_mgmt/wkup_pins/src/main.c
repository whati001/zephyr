/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/dt-bindings/gpio/stm32-gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wkup_pins, LOG_LEVEL_INF);

#define WKUP_SRC_NODE DT_ALIAS(wkup_src)
#if !DT_NODE_HAS_STATUS_OKAY(WKUP_SRC_NODE)
#error "Unsupported board: wkup_src devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(WKUP_SRC_NODE, gpios);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static struct gpio_callback gpio_cb;
static void wkup_pin_isr(const struct device *unused1, struct gpio_callback *unused2,
			 uint32_t unused3)
{
	LOG_INF("WAKE-UP button pressed");
}

int main(void)
{
	LOG_INF("Wake-up button is connected to %s pin %d", button.port->name, button.pin);

	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set(led.port, led.pin, 1);

	/* Setup button GPIO pin as a source for exiting Poweroff */
	gpio_pin_configure_dt(&button, STM32_GPIO_WKUP);

	// enable interrupt on button press
	gpio_init_callback(&gpio_cb, wkup_pin_isr, BIT(button.pin));
	int err = gpio_add_callback(button.port, &gpio_cb);
	if (err) {
		return err;
	}

	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

	LOG_INF("Powering off");
	LOG_INF("Press the user button to power the system on");

	// sys_poweroff();
	while (1) {
		k_sleep(K_SECONDS(4));
		LOG_INF("System is powered off, waiting for wake-up button press...");
	}
	/* Will remain powered off until wake-up or reset button is pressed */

	return 0;
}
