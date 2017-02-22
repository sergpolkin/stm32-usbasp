#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "usbasp.h"
#include "usbasp-config.h"
#include "isp.h"

static uint8_t prog_state = PROG_STATE_IDLE;
static uint8_t prog_sck = USBASP_ISP_SCK_AUTO;

static uint8_t prog_address_newmode = 0;
static uint32_t prog_address;
static uint16_t prog_nbytes = 0;
static uint16_t prog_pagesize;
static uint8_t prog_blockflags;
static uint8_t prog_pagecounter;

static uint16_t usbFunctionRead(uint8_t* data, uint16_t len);
static uint16_t usbFunctionWrite(uint8_t* data, uint16_t len);

static int usbFunctionSetup(usbd_device *usbd_dev, struct usb_setup_data *req,
	uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)usbd_dev;
	(void)complete;

	if ((req->bmRequestType & 0x7F) != USB_REQ_TYPE_VENDOR)
		return 0;
	(*len) = 0;
	if (req->bRequest == USBASP_FUNC_CONNECT) {
		/* set SCK speed */
		// if ((PINC & (1 << PC2)) == 0) {
		// 	ispSetSCKOption(USBASP_ISP_SCK_8);
		// } else {
		// 	ispSetSCKOption(prog_sck);
		// }
		ispSetSCKOption(prog_sck);

		/* set compatibility mode of address delivering */
		prog_address_newmode = 0;

		ledRedOn();
		ispConnect();

	} else if (req->bRequest == USBASP_FUNC_DISCONNECT) {
		ispDisconnect();
		ledRedOff();

	} else if (req->bRequest == USBASP_FUNC_TRANSMIT) {
		(*buf)[0] = ispTransmit(req->wValue & 0xFF);
		(*buf)[1] = ispTransmit(req->wValue >> 8);
		(*buf)[2] = ispTransmit(req->wIndex & 0xFF);
		(*buf)[3] = ispTransmit(req->wIndex >> 8);
		(*len) = 4;

	} else if (req->bRequest == USBASP_FUNC_READFLASH) {

		if (!prog_address_newmode)
			prog_address = req->wValue;

		prog_nbytes = req->wLength;
		prog_state = PROG_STATE_READFLASH;
		(*len) = usbFunctionRead((*buf), prog_nbytes); /* multiple in */

	} else if (req->bRequest == USBASP_FUNC_READEEPROM) {

		if (!prog_address_newmode)
			prog_address = req->wValue;

		prog_nbytes = req->wLength;
		prog_state = PROG_STATE_READEEPROM;
		(*len) = usbFunctionRead((*buf), prog_nbytes); /* multiple in */

	} else if (req->bRequest == USBASP_FUNC_ENABLEPROG) {
		(*buf)[0] = ispEnterProgrammingMode();
		(*len) = 1;

	} else if (req->bRequest == USBASP_FUNC_WRITEFLASH) {

		if (!prog_address_newmode)
			prog_address = req->wValue;

		prog_pagesize = req->wIndex & 0xFF;
		prog_blockflags = (req->wIndex >> 8) & 0x0F;
		prog_pagesize += ((req->wIndex >> 8) & 0xF0) << 4;
		if (prog_blockflags & PROG_BLOCKFLAG_FIRST) {
			prog_pagecounter = prog_pagesize;
		}
		prog_nbytes = req->wLength;
		prog_state = PROG_STATE_WRITEFLASH;
		(*len) = usbFunctionWrite((*buf), prog_nbytes); /* multiple out */

	} else if (req->bRequest == USBASP_FUNC_WRITEEEPROM) {

		if (!prog_address_newmode)
			prog_address = req->wValue;

		prog_pagesize = 0;
		prog_blockflags = 0;
		prog_nbytes = req->wLength;
		prog_state = PROG_STATE_WRITEEEPROM;
		(*len) = usbFunctionWrite((*buf), prog_nbytes); /* multiple out */

	} else if (req->bRequest == USBASP_FUNC_SETLONGADDRESS) {

		/* set new mode of address delivering (ignore address delivered in commands) */
		prog_address_newmode = 1;
		/* set new address */
		prog_address = (req->wIndex<<16) | req->wValue;
		(*len) = 0;

	} else if (req->bRequest == USBASP_FUNC_SETISPSCK) {

		/* set sck option */
		prog_sck = req->wValue;
		(*buf)[0] = 0;
		(*len) = 1;

	} else if (req->bRequest == USBASP_FUNC_TPI_CONNECT) {
		// tpi_dly_cnt = data[2] | (data[3] << 8);
		//
		// /* RST high */
		// ISP_OUT |= (1 << ISP_RST);
		// ISP_DDR |= (1 << ISP_RST);
		//
		// clockWait(3);
		//
		// /* RST low */
		// ISP_OUT &= ~(1 << ISP_RST);
		// ledRedOn();
		//
		// clockWait(16);
		// tpi_init();

	} else if (req->bRequest == USBASP_FUNC_TPI_DISCONNECT) {

		// tpi_send_byte(TPI_OP_SSTCS(TPISR));
		// tpi_send_byte(0);
		//
		// clockWait(10);
		//
		// /* pulse RST */
		// ISP_OUT |= (1 << ISP_RST);
		// clockWait(5);
		// ISP_OUT &= ~(1 << ISP_RST);
		// clockWait(5);
		//
		// /* set all ISP pins inputs */
		// ISP_DDR &= ~((1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI));
		// /* switch pullups off */
		// ISP_OUT &= ~((1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI));
		//
		// ledRedOff();

	} else if (req->bRequest == USBASP_FUNC_TPI_RAWREAD) {
		// replyBuffer[0] = tpi_recv_byte();
		// len = 1;

	} else if (req->bRequest == USBASP_FUNC_TPI_RAWWRITE) {
		// tpi_send_byte(data[2]);

	} else if (req->bRequest == USBASP_FUNC_TPI_READBLOCK) {
		// prog_address = (data[3] << 8) | data[2];
		// prog_nbytes = (data[7] << 8) | data[6];
		// prog_state = PROG_STATE_TPI_READ;
		// len = 0xff; /* multiple in */

	} else if (req->bRequest == USBASP_FUNC_TPI_WRITEBLOCK) {
		// prog_address = (data[3] << 8) | data[2];
		// prog_nbytes = (data[7] << 8) | data[6];
		// prog_state = PROG_STATE_TPI_WRITE;
		// len = 0xff; /* multiple out */

	} else if (req->bRequest == USBASP_FUNC_GETCAPABILITIES) {
		(*buf)[0] = 0;
		(*buf)[1] = 0;
		(*buf)[2] = 0;
		(*buf)[3] = 0;
		(*len) = 4;
	}
	return 1;
}

uint16_t usbFunctionRead(uint8_t* data, uint16_t len) {

	uint8_t i;

	/* check if programmer is in correct read state */
	if ((prog_state != PROG_STATE_READFLASH) && (prog_state
		!= PROG_STATE_READEEPROM) && (prog_state != PROG_STATE_TPI_READ)) {
		return 0xff;
	}

	/* fill packet TPI mode */
	if(prog_state == PROG_STATE_TPI_READ)
	{
		// tpi_read_block(prog_address, data, len);
		// prog_address += len;
		// return len;
		return 0xff;
	}

	/* fill packet ISP mode */
	for (i = 0; i < len; i++) {
		if (prog_state == PROG_STATE_READFLASH) {
			data[i] = ispReadFlash(prog_address);
		} else {
			data[i] = ispReadEEPROM(prog_address);
		}
		prog_address++;
	}

	/* last packet? */
	if (len < 8) {
		prog_state = PROG_STATE_IDLE;
	}

	return len;
}

uint16_t usbFunctionWrite(uint8_t* data, uint16_t len) {
	uint8_t retVal = 0;
	uint8_t i;

	/* check if programmer is in correct write state */
	if ((prog_state != PROG_STATE_WRITEFLASH) && (prog_state
		!= PROG_STATE_WRITEEEPROM) && (prog_state != PROG_STATE_TPI_WRITE)) {
		return 0xff;
	}

	if (prog_state == PROG_STATE_TPI_WRITE)
	{
		// tpi_write_block(prog_address, data, len);
		// prog_address += len;
		// prog_nbytes -= len;
		// if(prog_nbytes <= 0)
		// {
		// 	prog_state = PROG_STATE_IDLE;
		// 	return 1;
		// }
		// return 0;
		return 0xff;
	}

	for (i = 0; i < len; i++) {

		if (prog_state == PROG_STATE_WRITEFLASH) {
			/* Flash */

			if (prog_pagesize == 0) {
				/* not paged */
				ispWriteFlash(prog_address, data[i], 1);
			} else {
				/* paged */
				ispWriteFlash(prog_address, data[i], 0);
				prog_pagecounter--;
				if (prog_pagecounter == 0) {
					ispFlushPage(prog_address, data[i]);
					prog_pagecounter = prog_pagesize;
				}
			}

		} else {
			/* EEPROM */
			ispWriteEEPROM(prog_address, data[i]);
		}

		prog_nbytes--;

		if (prog_nbytes == 0) {
			prog_state = PROG_STATE_IDLE;
			if ((prog_blockflags & PROG_BLOCKFLAG_LAST) && (prog_pagecounter
					!= prog_pagesize)) {

				/* last block and page flush pending, so flush it now */
				ispFlushPage(prog_address, data[i]);
			}

			retVal = 1; // Need to return 1 when no more data is to be received
		}

		prog_address++;
	}

	return retVal;
}

static void config_setup(usbd_device *usbd_dev, uint16_t wValue) {
	(void)wValue;
	(void)usbd_dev;
	usbd_register_control_callback(
		usbd_dev,
		USB_REQ_TYPE_VENDOR, // type
		USB_REQ_TYPE_TYPE, // type mask
		usbFunctionSetup);
}

static void usb_setup(void) {
	rcc_clock_setup_hsi(&rcc_hsi_8mhz[RCC_CLOCK_48MHZ]);
	rcc_usb_prescale_1();
	rcc_periph_clock_enable(RCC_USB);
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF14, GPIO11| GPIO12);
}

usbd_device * usbasp_init(void) {
	usbd_device *usbd_dev;
	static uint8_t usbd_control_buffer[128]; // Buffer to be used for control requests

	usb_setup();

	usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev_desc, &config,
		usbasp_strings, 2,
		usbd_control_buffer, sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(usbd_dev, config_setup);

	return usbd_dev;
}
