/*
 * Copyright (C) 2016 Intel Corporation
 * SPDX-License-Identifier:     GPL-2.0+ BSD-2-Clause
 */

#include <common.h>
#include <status_led.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <common.h>

#define GPIO_PIN_GREEN_LED 44
#define GPIO_PIN_BLUE_LED 45
#define GPIO_PIN_RED_LED 46

static void set_gpio_direction_out(led_id_t mask) {
  uint8_t reg;
  uint8_t gpio_pin;
  uint32_t* gpdr_reg;

  if (mask == STATUS_LED_RED)
    gpio_pin = GPIO_PIN_RED_LED;
  else if (mask == STATUS_LED_GREEN)
    gpio_pin = GPIO_PIN_GREEN_LED;
  else if (mask == STATUS_LED_BLUE)
    gpio_pin = GPIO_PIN_BLUE_LED;
  else {
    printf("invalid led mask\n");
    return;
  }

  reg = gpio_pin / 32;
  gpdr_reg = GPIO_REG_ADDR(reg, GPDR_REG_OFFSET);
  *gpdr_reg |= (0x1 << (gpio_pin % 32));
}

static void set_gpio_value(led_id_t mask, int state) {
  uint8_t reg;
  uint8_t gpio_pin;
  uint32_t* gpsr_reg, *gpcr_reg;

  if (mask == STATUS_LED_RED)
    gpio_pin = GPIO_PIN_RED_LED;
  else if (mask == STATUS_LED_GREEN)
    gpio_pin = GPIO_PIN_GREEN_LED;
  else if (mask == STATUS_LED_BLUE)
    gpio_pin = GPIO_PIN_BLUE_LED;
  else {
    printf("invalid led mask\n");
    return;
  }

  reg = gpio_pin / 32;

  gpsr_reg = GPIO_REG_ADDR(reg, GPSR_REG_OFFSET);
  gpcr_reg = GPIO_REG_ADDR(reg, GPCR_REG_OFFSET);
  if (STATUS_LED_ON == state) *gpsr_reg |= (0x1 << (gpio_pin % 32));
  if (STATUS_LED_OFF == state) *gpcr_reg |= (0x1 << (gpio_pin % 32));
}

void __led_init(led_id_t mask, int state) {
  __led_set(mask, state);
}

void __led_set(led_id_t mask, int state) {
  /*Set direction of GPIO to out*/
  set_gpio_direction_out(mask);
  /*set gpio data value to "state"*/
  set_gpio_value(mask, state);
}

void red_led_on() {
  __led_set(STATUS_LED_RED, STATUS_LED_ON);
}

void red_led_off() {
  __led_set(STATUS_LED_RED, STATUS_LED_OFF);
}

void green_led_on() {
  __led_set(STATUS_LED_GREEN, STATUS_LED_ON);
}

void green_led_off() {
  __led_set(STATUS_LED_GREEN, STATUS_LED_OFF);
}

void blue_led_on() {
  __led_set(STATUS_LED_BLUE, STATUS_LED_ON);
}

void blue_led_off() {
  __led_set(STATUS_LED_BLUE, STATUS_LED_OFF);
}
