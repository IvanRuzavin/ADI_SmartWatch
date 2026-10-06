#ifndef OXIMETER5_BAREMETAL_H
#define OXIMETER5_BAREMETAL_H

#include <stdint.h>
#include "bm_gpio.h"
#include "bm_i2c_soft.h"

#define OXIMETER5_DEVICE_ADDRESS        (0x57U)
#define OXIMETER5_BUFFER_SIZE           (100U)
#define OXIMETER5_SPO2_ERROR            (255U)
#define OXIMETER5_HEART_RATE_ERROR      (-999)

typedef enum {
    OXIMETER5_OK = 0,
    OXIMETER5_ERROR = -1
} oximeter5_status_t;

typedef struct {
    uint8_t sensor_valid;
    uint8_t finger_present;
    uint8_t spo2_valid;
    uint8_t spo2_percent;
    uint8_t heart_rate_valid;
    int32_t heart_rate_bpm;
    uint8_t temperature_valid;
    int16_t temperature_deci_c;
    uint32_t ir_raw;
    uint32_t red_raw;
} oximeter5_data_t;

typedef struct {
    bm_i2c_soft_t i2c;
    bm_gpio_pin_t int_pin;
    uint8_t address;
    uint32_t ir_buffer[OXIMETER5_BUFFER_SIZE];
    uint32_t red_buffer[OXIMETER5_BUFFER_SIZE];
    uint16_t sample_count;
    uint16_t warmup_samples;
    uint8_t samples_since_calculation;
    oximeter5_data_t data;
} oximeter5_t;

typedef struct {
    bm_gpio_pin_t scl;
    bm_gpio_pin_t sda;
    bm_gpio_pin_t int_pin;
    uint16_t i2c_half_period_us;
    uint8_t i2c_address;
} oximeter5_cfg_t;

void oximeter5_cfg_setup(oximeter5_cfg_t *cfg);
oximeter5_status_t oximeter5_init(oximeter5_t *ctx, const oximeter5_cfg_t *cfg);
oximeter5_status_t oximeter5_default_cfg(oximeter5_t *ctx);
oximeter5_status_t oximeter5_service(oximeter5_t *ctx);
oximeter5_status_t oximeter5_read_temperature_deci_c(oximeter5_t *ctx, int16_t *temperature_deci_c);
const oximeter5_data_t *oximeter5_get_data(const oximeter5_t *ctx);

/* Algorithms preserved from the supplied Oximeter 5 Click package. */
oximeter5_status_t oximeter5_get_oxygen_saturation(uint32_t *pun_ir_buffer,
                                                    int32_t n_ir_buffer_length,
                                                    uint32_t *pun_red_buffer,
                                                    uint8_t *pn_spo2);
oximeter5_status_t oximeter5_get_heart_rate(uint32_t *pun_ir_buffer,
                                            int32_t n_ir_buffer_length,
                                            uint32_t *pun_red_buffer,
                                            int32_t *pn_heart_rate);

#endif /* OXIMETER5_BAREMETAL_H */
