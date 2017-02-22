#pragma once

/* Prepare connection to target device */
void ispConnect(void);

/* Close connection to target device */
void ispDisconnect(void);

/* pointer transmit function */
extern uint8_t (*ispTransmit)(uint8_t);

/* enter programming mode */
uint8_t ispEnterProgrammingMode(void);

/* read byte from eeprom at given address */
uint8_t ispReadEEPROM(unsigned int address);

/* write byte to flash at given address */
uint8_t ispWriteFlash(uint32_t address, uint8_t data, uint8_t pollmode);

uint8_t ispFlushPage(uint32_t address, uint8_t pollvalue);

/* read byte from flash at given address */
uint8_t ispReadFlash(uint32_t address);

/* write byte to eeprom at given address */
uint8_t ispWriteEEPROM(unsigned int address, uint8_t data);

/* set SCK speed. call before ispConnect! */
void ispSetSCKOption(uint8_t sckoption);

/* load extended address byte */
void ispLoadExtendedAddressByte(unsigned long address);
