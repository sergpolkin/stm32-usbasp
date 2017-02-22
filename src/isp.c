#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#include "isp.h"

#define ISP_RCC  RCC_GPIOD
#define ISP_PORT GPIOD
#define ISP_RST  GPIO15
#define ISP_MOSI GPIO14
#define ISP_MISO GPIO12
#define ISP_SCK  GPIO13

uint8_t (*ispTransmit)(uint8_t) = 0;

static uint8_t isp_hiaddr;
static uint16_t sck_sw_delay;

static void timer_init(void);
static void ispDelay(uint16_t delay);
static uint8_t ispTransmit_sw(uint8_t send_byte);

void ispSetSCKOption(uint8_t option) {
	(void)option;
	ispTransmit = ispTransmit_sw;
	sck_sw_delay = 320;
	timer_init();
}

void ispConnect() {
	rcc_periph_clock_enable(ISP_RCC);

	/* all ISP pins are inputs before */
	/* now set output pins */
	gpio_mode_setup(ISP_PORT, GPIO_MODE_OUTPUT,
		GPIO_PUPD_NONE, ISP_RST | ISP_MOSI | ISP_SCK );

	/* reset device */
	gpio_clear(ISP_PORT, ISP_RST | ISP_SCK); /* RST, SCK low */

	/* positive reset pulse > 2 SCK (target) */
	ispDelay(sck_sw_delay);
	gpio_set(ISP_PORT, ISP_RST); /* RST high */
	ispDelay(sck_sw_delay);
	gpio_clear(ISP_PORT, ISP_RST); /* RST low */

	/* Initial extended address value */
	isp_hiaddr = 0;
}

void ispDisconnect() {
	/* set all ISP pins inputs */
	/* switch pullups off */
	gpio_mode_setup(ISP_PORT, GPIO_MODE_INPUT,
		GPIO_PUPD_NONE, ISP_RST | ISP_MOSI | ISP_MISO | ISP_SCK );
	/* disable hardware SPI */
	// spiHWdisable();
}

uint8_t ispTransmit_sw(uint8_t send_byte) {
	uint8_t rec_byte = 0;
	uint8_t i;
	for (i = 0; i < 8; i++) {

		/* set MSB to MOSI-pin */
		if ((send_byte & 0x80) != 0) {
			gpio_set(ISP_PORT, ISP_MOSI); /* MOSI high */
		} else {
			gpio_clear(ISP_PORT, ISP_MOSI); /* MOSI low */
		}
		/* shift to next bit */
		send_byte = send_byte << 1;

		/* receive data */
		rec_byte = rec_byte << 1;
		if (gpio_get(ISP_PORT, ISP_MISO) != 0) {
			rec_byte++;
		}

		/* pulse SCK */
		gpio_set(ISP_PORT, ISP_SCK); /* SCK high */
		ispDelay(1);
		gpio_clear(ISP_PORT, ISP_SCK); /* SCK low */
		ispDelay(1);
	}

	return rec_byte;
}

uint8_t ispEnterProgrammingMode() {
	uint8_t check;
	uint8_t count = 32;

	while (count--) {
		ispTransmit(0xAC);
		ispTransmit(0x53);
		check = ispTransmit(0);
		ispTransmit(0);

		if (check == 0x53) {
			return 0;
		}

		// spiHWdisable();

		/* pulse RST */
		ispDelay(sck_sw_delay);
		gpio_set(ISP_PORT, ISP_RST); /* RST high */
		ispDelay(sck_sw_delay);
		gpio_clear(ISP_PORT, ISP_RST); /* RST low */
		ispDelay(sck_sw_delay);

		// if (ispTransmit == ispTransmit_hw) {
		// 	spiHWenable();
		// }

	}

	return 1; /* error: device dosn't answer */
}

static void ispUpdateExtended(uint32_t address)
{
	uint8_t curr_hiaddr;

	curr_hiaddr = (address >> 17);

	/* check if extended address byte is changed */
	if(isp_hiaddr != curr_hiaddr)
	{
		isp_hiaddr = curr_hiaddr;
		/* Load Extended Address byte */
		ispTransmit(0x4D);
		ispTransmit(0x00);
		ispTransmit(isp_hiaddr);
		ispTransmit(0x00);
	}
}

uint8_t ispReadFlash(uint32_t address) {

	ispUpdateExtended(address);

	ispTransmit(0x20 | ((address & 1) << 3));
	ispTransmit(address >> 9);
	ispTransmit(address >> 1);
	return ispTransmit(0);
}

uint8_t ispWriteFlash(uint32_t address, uint8_t data, uint8_t pollmode) {

	/* 0xFF is value after chip erase, so skip programming
	 if (data == 0xFF) {
	 return 0;
	 }
	 */

	ispUpdateExtended(address);

	ispTransmit(0x40 | ((address & 1) << 3));
	ispTransmit(address >> 9);
	ispTransmit(address >> 1);
	ispTransmit(data);

	if (pollmode == 0)
		return 0;

	if (data == 0x7F) {
		ispDelay(4800); /* wait 4,8 ms */
		return 0;
	} else {

		/* polling flash */
		uint8_t retries = 30;
		while (retries != 0) {
			if (ispReadFlash(address) != 0x7F) {
				return 0;
			};

			ispDelay(320);
			retries--;

		}
	}

	return 1; /* error */
}

uint8_t ispFlushPage(uint32_t address, uint8_t pollvalue) {

	ispUpdateExtended(address);

	ispTransmit(0x4C);
	ispTransmit(address >> 9);
	ispTransmit(address >> 1);
	ispTransmit(0);

	if (pollvalue == 0xFF) {
		ispDelay(4800); /* wait 4,8 ms */
		return 0;
	} else {

		/* polling flash */
		uint8_t retries = 30;
		while (retries != 0) {
			if (ispReadFlash(address) != 0xFF) {
				return 0;
			};

			ispDelay(320);
			retries--;

		}

	}

	return 1; /* error */
}

void timer_init(void) {
	/* Enable TIM2 clock. */
	rcc_periph_clock_enable(RCC_TIM2);
	/* Reset TIM2 peripheral to defaults. */
	rcc_periph_reset_pulse(RST_TIM2);

	/* Timer global mode:
	* - No divider
	* - Alignment edge
	* - Direction up
	* (These are actually default values after reset above, so this call
	* is strictly unnecessary, but demos the api for alternative settings)
	*/
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
		TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

	/* Disable preload. */
	timer_disable_preload(TIM2);
	timer_continuous_mode(TIM2);

	/* Set prescaler */
	timer_set_prescaler(TIM2, 48);

	/* Counter enable. */
	timer_enable_counter(TIM2);
}

void ispDelay(uint16_t delay) {
	timer_set_period(TIM2, delay);
	timer_set_counter(TIM2, 0);
	timer_clear_flag(TIM2, TIM_SR_UIF);
	while(!timer_get_flag(TIM2, TIM_SR_UIF));
}
