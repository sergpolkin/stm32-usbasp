# stm32-usbasp

USBASP in-circuit programmer, implementation on stm32f3discovery board.

Used [libopencm3](https://github.com/libopencm3/libopencm3).

Connection (defined in src/isp.c)

     ISP          STM32F3-DISCOVERY
    -----        -------------------
    MOSI   <--    PD14
    RST    <--    PD15
    SCK    <--    PD13
    MISO   -->    PD12
