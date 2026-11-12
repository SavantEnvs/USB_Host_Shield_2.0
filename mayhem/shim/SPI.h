#ifndef MAYHEM_SHIM_SPI_H
#define MAYHEM_SHIM_SPI_H
#include "Arduino.h"

#define SPI_HAS_TRANSACTION 1
#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 3

class SPISettings {
public:
        SPISettings(uint32_t = 0, uint8_t = 0, uint8_t = 0) {}
};

class SPIClass {
public:
        void begin() {}
        void end() {}
        void beginTransaction(SPISettings) {}
        void endTransaction() {}
        uint8_t transfer(uint8_t) { return 0; }
        void transfer(void *buf, size_t n) { memset(buf, 0, n); }
};

extern SPIClass SPI;

#endif
