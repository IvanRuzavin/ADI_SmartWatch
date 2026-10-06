#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>
#include "mcu.h"

/* --------------------------------------------------------------------------
 * MAX32690 system clocks
 * -------------------------------------------------------------------------- */
#define BOARD_SYSCLK_HZ                 (120000000UL)
#define BOARD_PERIPHERAL_HZ             (BOARD_SYSCLK_HZ / 2UL)
#define BOARD_SPI_INPUT_HZ              (BOARD_PERIPHERAL_HZ / 2UL)

/* --------------------------------------------------------------------------
 * IPS Display 2 Click on mikroBUS 1 - SPI1A
 * -------------------------------------------------------------------------- */
#define BOARD_IPS_SPI_HZ                (5000000UL)
#define BOARD_IPS_SPI                   MXC_SPI1

#define BOARD_SPI1_SCK_PORT             MXC_GPIO1
#define BOARD_SPI1_SCK_MASK             (1UL << 26)
#define BOARD_SPI1_MISO_PORT            MXC_GPIO1
#define BOARD_SPI1_MISO_MASK            (1UL << 28)
#define BOARD_SPI1_MOSI_PORT            MXC_GPIO1
#define BOARD_SPI1_MOSI_MASK            (1UL << 29)

#define BOARD_IPS_CS_PORT               MXC_GPIO1
#define BOARD_IPS_CS_MASK               (1UL << 27)
#define BOARD_IPS_RST_PORT              MXC_GPIO2
#define BOARD_IPS_RST_MASK              (1UL << 24)
#define BOARD_IPS_DC_PORT               MXC_GPIO0
#define BOARD_IPS_DC_MASK               (1UL << 23)
#define BOARD_IPS_BCK_PORT              MXC_GPIO3
#define BOARD_IPS_BCK_MASK              (1UL << 5)

/* --------------------------------------------------------------------------
 * Thermo 21 Click on mikroBUS 3 - SPI0B
 *   CS   -> P2.30 GPIO
 *   SCK  -> P2.29 SPI0B_SCK  (ALT2)
 *   MISO -> P2.27 SPI0B_MISO (ALT2)
 *   MOSI -> P2.28 SPI0B_MOSI (ALT2, not required by ADT7301 reads but mapped)
 * -------------------------------------------------------------------------- */
#define BOARD_THERMO_SPI                MXC_SPI0
#define BOARD_THERMO_SPI_HZ             (100000UL)

#define BOARD_SPI0_SCK_PORT             MXC_GPIO2
#define BOARD_SPI0_SCK_MASK             (1UL << 29)
#define BOARD_SPI0_MISO_PORT            MXC_GPIO2
#define BOARD_SPI0_MISO_MASK            (1UL << 27)
#define BOARD_SPI0_MOSI_PORT            MXC_GPIO2
#define BOARD_SPI0_MOSI_MASK            (1UL << 28)
#define BOARD_THERMO_CS_PORT            MXC_GPIO2
#define BOARD_THERMO_CS_MASK            (1UL << 30)

/* --------------------------------------------------------------------------
 * RTC 24 Click on mikroBUS 4
 *   SCL -> P0.31
 *   SDA -> P0.30
 *   SQW -> P0.09 (mikroBUS INT)
 * -------------------------------------------------------------------------- */
#define BOARD_RTC_SCL_PORT              MXC_GPIO0
#define BOARD_RTC_SCL_MASK              (1UL << 31)
#define BOARD_RTC_SDA_PORT              MXC_GPIO0
#define BOARD_RTC_SDA_MASK              (1UL << 30)
#define BOARD_RTC_SQW_PORT              MXC_GPIO0
#define BOARD_RTC_SQW_MASK              (1UL << 9)
#define BOARD_RTC_I2C_HALF_PERIOD_US    (5U)


/* --------------------------------------------------------------------------
 * Oximeter 5 Click on mikroBUS 2 - software I2C
 *   SCL -> P0.14
 *   SDA -> P0.13
 *   INT -> P2.23 (active low)
 * -------------------------------------------------------------------------- */
#define BOARD_OXIMETER_SCL_PORT         MXC_GPIO0
#define BOARD_OXIMETER_SCL_MASK         (1UL << 14)
#define BOARD_OXIMETER_SDA_PORT         MXC_GPIO0
#define BOARD_OXIMETER_SDA_MASK         (1UL << 13)
#define BOARD_OXIMETER_INT_PORT         MXC_GPIO2
#define BOARD_OXIMETER_INT_MASK         (1UL << 23)
#define BOARD_OXIMETER_I2C_HALF_PERIOD_US (5U)
#define BOARD_OXIMETER_TEMP_POLL_MS     (2000U)
#define BOARD_OXIMETER_UI_REFRESH_MS    (250U)

/* --------------------------------------------------------------------------
 * User buttons - active low with pull-ups.
 * -------------------------------------------------------------------------- */
#define BOARD_B1_PORT                   MXC_GPIO0
#define BOARD_B1_MASK                   (1UL << 2)
#define BOARD_B2_PORT                   MXC_GPIO0
#define BOARD_B2_MASK                   (1UL << 18)
#define BOARD_B3_PORT                   MXC_GPIO2
#define BOARD_B3_MASK                   (1UL << 13)
#define BOARD_B4_PORT                   MXC_GPIO2
#define BOARD_B4_MASK                   (1UL << 22)
#define BOARD_B5_PORT                   MXC_GPIO0
#define BOARD_B5_MASK                   (1UL << 3)
#define BOARD_B6_PORT                   MXC_GPIO2
#define BOARD_B6_MASK                   (1UL << 25)

#define BOARD_BUTTON_ACTIVE_LOW         (1U)
#define BOARD_BUTTON_DEBOUNCE_TICKS     (3U)
#define BOARD_BUTTON_REPEAT_DELAY_MS    (1000U)
#define BOARD_BUTTON_REPEAT_INTERVAL_MS (120U)
#define BOARD_APP_TICK_MS               (10U)
#define BOARD_THERMO_POLL_MS            (500U)

#endif /* BOARD_CONFIG_H */
