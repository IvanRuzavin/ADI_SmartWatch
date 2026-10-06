#include "bm_spi.h"
#include "bm_gpio.h"
#include "board_config.h"
#include "mcu.h"

#define BM_SPI_FIFO_DEPTH_BYTES   (32U)
#define BM_SPI_TIMEOUT_LOOPS      (1000000UL)

static int bm_wait_mask_clear(volatile const uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = BM_SPI_TIMEOUT_LOOPS;
    while ((*reg & mask) != 0U) {
        if (--timeout == 0U) return -1;
    }
    return 0;
}

static int bm_wait_mask_set(volatile const uint32_t *reg, uint32_t mask)
{
    uint32_t timeout = BM_SPI_TIMEOUT_LOOPS;
    while ((*reg & mask) == 0U) {
        if (--timeout == 0U) return -1;
    }
    return 0;
}

static bm_spi_status_t bm_spi_set_frequency(mxc_spi_regs_t *spi, uint32_t hz)
{
    uint32_t divider;
    uint32_t hi;
    uint32_t lo;
    uint32_t scale = 0U;

    if ((spi == 0) || (hz == 0U) || (hz > (BOARD_SPI_INPUT_HZ / 2U))) {
        return BM_SPI_ERR_CLOCK;
    }

    divider = BOARD_SPI_INPUT_HZ / hz;
    if (divider < 2U) divider = 2U;

    lo = divider / 2U;
    hi = divider - lo;

    while ((hi >= 16U) && (scale < 8U)) {
        hi >>= 1U;
        lo >>= 1U;
        ++scale;
    }

    if ((hi == 0U) || (lo == 0U) || (hi > 0xFFU) || (lo > 0xFFU) || (scale > 0xFU)) {
        return BM_SPI_ERR_CLOCK;
    }

    spi->clkctrl =
        ((lo << MXC_F_SPI_CLKCTRL_LO_POS) & MXC_F_SPI_CLKCTRL_LO) |
        ((hi << MXC_F_SPI_CLKCTRL_HI_POS) & MXC_F_SPI_CLKCTRL_HI) |
        ((scale << MXC_F_SPI_CLKCTRL_CLKDIV_POS) & MXC_F_SPI_CLKCTRL_CLKDIV);

    return BM_SPI_OK;
}

static void bm_spi_common_register_init(mxc_spi_regs_t *spi)
{
    spi->inten = 0U;
    spi->intfl = spi->intfl;
    spi->ctrl0 = 0U;
    spi->ctrl1 = 0U;
    spi->ctrl2 = MXC_S_SPI_CTRL2_NUMBITS_8;
    spi->sstime =
        (1U << MXC_F_SPI_SSTIME_PRE_POS) |
        (1U << MXC_F_SPI_SSTIME_POST_POS) |
        (1U << MXC_F_SPI_SSTIME_INACT_POS);
    spi->dma = 0U;

    /* SPI mode 0, standard single-bit width. */
    spi->ctrl2 &= ~(MXC_F_SPI_CTRL2_CLKPHA | MXC_F_SPI_CTRL2_CLKPOL |
                    MXC_F_SPI_CTRL2_THREE_WIRE | MXC_F_SPI_CTRL2_DATA_WIDTH);
}

bm_spi_status_t bm_spi1_init(uint32_t spi_hz)
{
    bm_spi_status_t status;
    bm_gpio_pin_t sck  = { BOARD_SPI1_SCK_PORT,  BOARD_SPI1_SCK_MASK };
    bm_gpio_pin_t miso = { BOARD_SPI1_MISO_PORT, BOARD_SPI1_MISO_MASK };
    bm_gpio_pin_t mosi = { BOARD_SPI1_MOSI_PORT, BOARD_SPI1_MOSI_MASK };

    MXC_GCR->rst0 = MXC_F_GCR_RST0_SPI1;
    if (bm_wait_mask_clear(&MXC_GCR->rst0, MXC_F_GCR_RST0_SPI1) != 0) {
        return BM_SPI_ERR_TIMEOUT;
    }

    MXC_GCR->pclkdis0 &= ~MXC_F_GCR_PCLKDIS0_SPI1;

    /* SPI1A: P1.26/P1.28/P1.29, ALT1, VDDIOH. */
    bm_gpio_init_alt1(sck);
    bm_gpio_init_alt1(miso);
    bm_gpio_init_alt1(mosi);

    bm_spi_common_register_init(BOARD_IPS_SPI);
    status = bm_spi_set_frequency(BOARD_IPS_SPI, spi_hz);
    if (status != BM_SPI_OK) return status;

    BOARD_IPS_SPI->ctrl0 = MXC_F_SPI_CTRL0_EN | MXC_F_SPI_CTRL0_MST_MODE;
    return BM_SPI_OK;
}

bm_spi_status_t bm_spi1_write(const uint8_t *data, size_t length)
{
    while (length > 0U) {
        uint32_t i;
        uint32_t chunk = (length > BM_SPI_FIFO_DEPTH_BYTES) ? BM_SPI_FIFO_DEPTH_BYTES : (uint32_t)length;

        if (data == 0) return BM_SPI_ERR_PARAM;
        if (bm_wait_mask_clear(&BOARD_IPS_SPI->stat, MXC_F_SPI_STAT_BUSY) != 0) return BM_SPI_ERR_TIMEOUT;

        BOARD_IPS_SPI->ctrl0 &= ~MXC_F_SPI_CTRL0_EN;
        BOARD_IPS_SPI->ctrl1 &= ~(MXC_F_SPI_CTRL1_TX_NUM_CHAR | MXC_F_SPI_CTRL1_RX_NUM_CHAR);
        BOARD_IPS_SPI->ctrl1 |=
            (chunk << MXC_F_SPI_CTRL1_TX_NUM_CHAR_POS) & MXC_F_SPI_CTRL1_TX_NUM_CHAR;

        BOARD_IPS_SPI->dma &= ~(MXC_F_SPI_DMA_TX_FIFO_EN | MXC_F_SPI_DMA_RX_FIFO_EN);
        BOARD_IPS_SPI->dma |= MXC_F_SPI_DMA_TX_FIFO_EN;
        BOARD_IPS_SPI->dma |= MXC_F_SPI_DMA_TX_FLUSH | MXC_F_SPI_DMA_RX_FLUSH;

        BOARD_IPS_SPI->intfl = MXC_F_SPI_INTFL_MST_DONE;
        BOARD_IPS_SPI->ctrl0 |= MXC_F_SPI_CTRL0_EN;

        for (i = 0U; i < chunk; ++i) BOARD_IPS_SPI->fifo8[0] = data[i];

        BOARD_IPS_SPI->ctrl0 |= MXC_F_SPI_CTRL0_START;
        if (bm_wait_mask_set(&BOARD_IPS_SPI->intfl, MXC_F_SPI_INTFL_MST_DONE) != 0) {
            return BM_SPI_ERR_TIMEOUT;
        }
        BOARD_IPS_SPI->intfl = MXC_F_SPI_INTFL_MST_DONE;

        data += chunk;
        length -= chunk;
    }
    return BM_SPI_OK;
}

bm_spi_status_t bm_spi0_init(uint32_t spi_hz)
{
    bm_spi_status_t status;
    bm_gpio_pin_t sck  = { BOARD_SPI0_SCK_PORT,  BOARD_SPI0_SCK_MASK };
    bm_gpio_pin_t miso = { BOARD_SPI0_MISO_PORT, BOARD_SPI0_MISO_MASK };
    bm_gpio_pin_t mosi = { BOARD_SPI0_MOSI_PORT, BOARD_SPI0_MOSI_MASK };

    MXC_GCR->rst0 = MXC_F_GCR_RST0_SPI0;
    if (bm_wait_mask_clear(&MXC_GCR->rst0, MXC_F_GCR_RST0_SPI0) != 0) {
        return BM_SPI_ERR_TIMEOUT;
    }

    MXC_GCR->pclkdis0 &= ~MXC_F_GCR_PCLKDIS0_SPI0;

    /* mikroBUS 3 maps to SPI0B on P2.27/P2.28/P2.29, which is ALT2. */
    bm_gpio_init_alt2(sck);
    bm_gpio_init_alt2(miso);
    bm_gpio_init_alt2(mosi);

    bm_spi_common_register_init(BOARD_THERMO_SPI);
    status = bm_spi_set_frequency(BOARD_THERMO_SPI, spi_hz);
    if (status != BM_SPI_OK) return status;

    BOARD_THERMO_SPI->ctrl0 = MXC_F_SPI_CTRL0_EN | MXC_F_SPI_CTRL0_MST_MODE;
    return BM_SPI_OK;
}

bm_spi_status_t bm_spi0_read(uint8_t *data, size_t length, uint8_t dummy)
{
    size_t offset = 0U;

    if ((data == 0) || (length == 0U)) return BM_SPI_ERR_PARAM;

    while (offset < length) {
        uint32_t i;
        uint32_t chunk = (uint32_t)(length - offset);
        if (chunk > BM_SPI_FIFO_DEPTH_BYTES) chunk = BM_SPI_FIFO_DEPTH_BYTES;

        if (bm_wait_mask_clear(&BOARD_THERMO_SPI->stat, MXC_F_SPI_STAT_BUSY) != 0) {
            return BM_SPI_ERR_TIMEOUT;
        }

        BOARD_THERMO_SPI->ctrl0 &= ~MXC_F_SPI_CTRL0_EN;
        BOARD_THERMO_SPI->ctrl1 &= ~(MXC_F_SPI_CTRL1_TX_NUM_CHAR | MXC_F_SPI_CTRL1_RX_NUM_CHAR);
        BOARD_THERMO_SPI->ctrl1 |=
            ((chunk << MXC_F_SPI_CTRL1_TX_NUM_CHAR_POS) & MXC_F_SPI_CTRL1_TX_NUM_CHAR) |
            ((chunk << MXC_F_SPI_CTRL1_RX_NUM_CHAR_POS) & MXC_F_SPI_CTRL1_RX_NUM_CHAR);

        BOARD_THERMO_SPI->dma &= ~(MXC_F_SPI_DMA_TX_FIFO_EN | MXC_F_SPI_DMA_RX_FIFO_EN);
        BOARD_THERMO_SPI->dma |= MXC_F_SPI_DMA_TX_FIFO_EN | MXC_F_SPI_DMA_RX_FIFO_EN;
        BOARD_THERMO_SPI->dma |= MXC_F_SPI_DMA_TX_FLUSH | MXC_F_SPI_DMA_RX_FLUSH;

        BOARD_THERMO_SPI->intfl = MXC_F_SPI_INTFL_MST_DONE;
        BOARD_THERMO_SPI->ctrl0 |= MXC_F_SPI_CTRL0_EN;

        /* Full-duplex SPI requires TX clocks while reading. */
        for (i = 0U; i < chunk; ++i) BOARD_THERMO_SPI->fifo8[0] = dummy;

        BOARD_THERMO_SPI->ctrl0 |= MXC_F_SPI_CTRL0_START;
        if (bm_wait_mask_set(&BOARD_THERMO_SPI->intfl, MXC_F_SPI_INTFL_MST_DONE) != 0) {
            return BM_SPI_ERR_TIMEOUT;
        }
        BOARD_THERMO_SPI->intfl = MXC_F_SPI_INTFL_MST_DONE;

        for (i = 0U; i < chunk; ++i) data[offset + i] = BOARD_THERMO_SPI->fifo8[0];
        offset += chunk;
    }

    return BM_SPI_OK;
}
