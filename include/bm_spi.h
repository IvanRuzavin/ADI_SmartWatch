#ifndef BM_SPI_H
#define BM_SPI_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    BM_SPI_OK = 0,
    BM_SPI_ERR_PARAM = -1,
    BM_SPI_ERR_TIMEOUT = -2,
    BM_SPI_ERR_CLOCK = -3
} bm_spi_status_t;

bm_spi_status_t bm_spi1_init(uint32_t spi_hz);
bm_spi_status_t bm_spi1_write(const uint8_t *data, size_t length);

bm_spi_status_t bm_spi0_init(uint32_t spi_hz);
bm_spi_status_t bm_spi0_read(uint8_t *data, size_t length, uint8_t dummy);

#endif /* BM_SPI_H */
