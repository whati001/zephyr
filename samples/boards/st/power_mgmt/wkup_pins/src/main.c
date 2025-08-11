/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stm32u5xx_ll_pwr.h"
#include "zephyr/dt-bindings/gpio/gpio.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/dt-bindings/gpio/stm32-gpio.h>

#define WAIT_TIME_US 4000000

#define MAIN_LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wkup_pins);

#define WKUP_SRC_NODE DT_ALIAS(wkup_src)
#if !DT_NODE_HAS_STATUS_OKAY(WKUP_SRC_NODE)
#error "Unsupported board: wkup_src devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(WKUP_SRC_NODE, gpios);

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

// Struct for holding GPIO-related callback functions
static struct gpio_callback btn_cb_data;

// GPIO callback (ISR)
void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	// Add work to the workqueue
	LOG_INF("Button pressed, system waking up\n");

	// clear APC flag to disable pull-up/down
	LL_PWR_DisablePUPDConfig();
}

int main(void)
{
	LOG_INF("\nWake-up button is connected to %s pin %d\n", button.port->name, button.pin);

	__ASSERT_NO_MSG(gpio_is_ready_dt(&led));
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set(led.port, led.pin, 1);

	/* Setup button GPIO pin as a source for exiting Poweroff */
	gpio_pin_configure_dt(&button, (STM32_GPIO_WKUP | GPIO_PULL_DOWN));

	LOG_INF("Press the user button to power the system from stop3\n\n");

	// Configure the interrupt
	int ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("ERROR: could not configure button as interrupt source\r\n");
		return 0;
	}

	// Connect callback function (ISR) to interrupt source
	gpio_init_callback(&btn_cb_data, button_isr, BIT(button.pin));
	gpio_add_callback(button.port, &btn_cb_data);

	// Do nothing
	while (1) {
		k_sleep(K_SECONDS(5));
		LOG_INF("System woke up from stop3 mode via RTC irq\n\n\n\n\n\n\n");
	}

	return 0;
}
