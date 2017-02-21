#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "src/usbasp.h"

static void dummy_delay(void) {
	for (int i = 0; i < 2000000; i++)
		__asm__("nop");
}
static void gpio_setup(void) {
	// LEDS
	const uint16_t leds_pin = GPIO8 | GPIO9 | GPIO10 | GPIO11 |
		GPIO12 | GPIO13 | GPIO14 | GPIO15;
	rcc_periph_clock_enable(RCC_GPIOE);
	gpio_mode_setup(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, leds_pin);
	gpio_clear(GPIOE, leds_pin);
}
static void usb_reset(void) {
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_clear(GPIOA, GPIO11 | GPIO12);
	gpio_set(GPIOE, GPIO9 | GPIO10);
	dummy_delay();
	gpio_clear(GPIOE, GPIO9 | GPIO10);
}
int main(void) {
	gpio_setup();
	usb_reset();
	usbd_device *usbd_dev = usbasp_init();

	while (1) {
		gpio_toggle(GPIOE, GPIO11);
		usbd_poll(usbd_dev);
	}
	return 0;
}
