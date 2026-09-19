// Copyright 2026 bhe
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef uint8_t pin_t;
void            ch582_rust_gpio_mode(pin_t pin, uint8_t mode);
void            ch582_rust_gpio_write(pin_t pin, bool high);
bool            ch582_rust_gpio_read(pin_t pin);
enum {
    CH582_GPIO_INPUT = 0,
    CH582_GPIO_INPUT_PULLUP,
    CH582_GPIO_INPUT_PULLDOWN,
    CH582_GPIO_OUTPUT,
    CH582_GPIO_OUTPUT_OPEN_DRAIN,
};
#define gpio_set_pin_input(pin) ch582_rust_gpio_mode((pin), CH582_GPIO_INPUT)
#define gpio_set_pin_input_high(pin) ch582_rust_gpio_mode((pin), CH582_GPIO_INPUT_PULLUP)
#define gpio_set_pin_input_low(pin) ch582_rust_gpio_mode((pin), CH582_GPIO_INPUT_PULLDOWN)
#define gpio_set_pin_output_push_pull(pin) ch582_rust_gpio_mode((pin), CH582_GPIO_OUTPUT)
#define gpio_set_pin_output_open_drain(pin) ch582_rust_gpio_mode((pin), CH582_GPIO_OUTPUT_OPEN_DRAIN)
#define gpio_set_pin_output(pin) gpio_set_pin_output_push_pull(pin)
#define gpio_write_pin_high(pin) ch582_rust_gpio_write((pin), true)
#define gpio_write_pin_low(pin) ch582_rust_gpio_write((pin), false)
#define gpio_write_pin(pin, level) ch582_rust_gpio_write((pin), (level))
#define gpio_read_pin(pin) ch582_rust_gpio_read(pin)
#define gpio_toggle_pin(pin) ch582_rust_gpio_write((pin), !ch582_rust_gpio_read(pin))
