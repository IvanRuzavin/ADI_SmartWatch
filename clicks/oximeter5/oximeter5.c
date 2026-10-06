/****************************************************************************
** Copyright (C) 2026 MikroElektronika d.o.o.
** Contact: https://www.mikroe.com/contact
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
****************************************************************************/
/* Bare-metal transport adaptation; SpO2/heart-rate algorithms are preserved
 * from the user-supplied Oximeter 5 Click package. */

#include "oximeter5.h"
#include "board_config.h"
#include "delays.h"

#define SAMPLING_FREQUENCY          25
#define BUFFER_SIZE                 (SAMPLING_FREQUENCY * 4)
#define MA4_SIZE                    4
#define DATA_18_BIT                 0x03FFFFU
#define OXIMETER5_N_X_DC_MAX        (-16777216)

#define OXIMETER5_REG_INTR_STATUS_1  0x00U
#define OXIMETER5_REG_INTR_ENABLE_1  0x02U
#define OXIMETER5_REG_INTR_ENABLE_2  0x03U
#define OXIMETER5_REG_FIFO_WR_PTR    0x04U
#define OXIMETER5_REG_OVF_COUNTER    0x05U
#define OXIMETER5_REG_FIFO_RD_PTR    0x06U
#define OXIMETER5_REG_FIFO_DATA      0x07U
#define OXIMETER5_REG_FIFO_CONFIG    0x08U
#define OXIMETER5_REG_MODE_CONFIG    0x09U
#define OXIMETER5_REG_SPO2_CONFIG    0x0AU
#define OXIMETER5_REG_LED1_PA        0x0CU
#define OXIMETER5_REG_LED2_PA        0x0DU
#define OXIMETER5_REG_TEMP_INTR      0x1FU
#define OXIMETER5_REG_TEMP_FRAC      0x20U
#define OXIMETER5_REG_TEMP_CONFIG    0x21U
#define OXIMETER5_REG_PART_ID        0xFFU

#define OXIMETER5_SW_RESET               0x40U
#define OXIMETER5_INTR_FIFO_FULL_PPG     (0x80U | 0x40U)
#define OXIMETER5_FIFO_AVG_4_AFULL_15    (0x40U | 0x0FU)
#define OXIMETER5_MODE_SPO2              0x03U
#define OXIMETER5_SPO2_4096_100HZ_18BIT  (0x20U | 0x04U | 0x03U)
#define OXIMETER5_LED_7_2_MA             0x24U
#define OXIMETER5_PART_ID_MAX30102       0x15U
#define OXIMETER5_FINGER_THRESHOLD       10000UL

const uint8_t uch_spo2_table[184] = {
    95,95,95,96,96,96,97,97,97,97,97,98,98,98,98,98,99,99,99,99,
    99,99,99,99,100,100,100,100,100,100,100,100,100,100,100,100,100,
    100,100,100,100,100,100,100,99,99,99,99,99,99,99,99,98,98,98,
    98,98,98,97,97,97,97,96,96,96,96,95,95,95,94,94,94,93,93,93,
    92,92,92,91,91,90,90,89,89,89,88,88,87,87,86,86,85,85,84,84,
    83,82,82,81,81,80,80,79,78,78,77,76,76,75,74,74,73,72,72,71,
    70,69,69,68,67,66,66,65,64,63,62,62,61,60,59,58,57,56,56,55,
    54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,
    34,33,31,30,29,28,27,26,25,23,22,21,20,19,17,16,15,14,12,11,
    10,9,7,6,5,3,2,1
};

static void dev_peaks_above_min_height(int32_t *pn_locs, int32_t *n_npks, int32_t *pn_x, uint8_t n_size, int32_t n_min_height);
static void dev_sort_indices_descend(int32_t *pn_x, int32_t *pn_indx, int32_t n_size);
static void dev_sort_ascend(int32_t *pn_x, int32_t n_size);
static void dev_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance);
static void dev_find_peaks(int32_t *pn_locs, int32_t *n_npks, int32_t *pn_x, uint8_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num);

static oximeter5_status_t reg_write(oximeter5_t *ctx, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = { reg, value };
    return (bm_i2c_soft_write(&ctx->i2c, ctx->address, data, 2U) == BM_I2C_OK) ? OXIMETER5_OK : OXIMETER5_ERROR;
}

static oximeter5_status_t reg_read(oximeter5_t *ctx, uint8_t reg, uint8_t *data, uint16_t len)
{
    return (bm_i2c_soft_write_then_read(&ctx->i2c, ctx->address, &reg, 1U, data, len) == BM_I2C_OK) ? OXIMETER5_OK : OXIMETER5_ERROR;
}

void oximeter5_cfg_setup(oximeter5_cfg_t *cfg)
{
    if (cfg == 0) return;
    cfg->scl = (bm_gpio_pin_t){ BOARD_OXIMETER_SCL_PORT, BOARD_OXIMETER_SCL_MASK };
    cfg->sda = (bm_gpio_pin_t){ BOARD_OXIMETER_SDA_PORT, BOARD_OXIMETER_SDA_MASK };
    cfg->int_pin = (bm_gpio_pin_t){ BOARD_OXIMETER_INT_PORT, BOARD_OXIMETER_INT_MASK };
    cfg->i2c_half_period_us = BOARD_OXIMETER_I2C_HALF_PERIOD_US;
    cfg->i2c_address = OXIMETER5_DEVICE_ADDRESS;
}

oximeter5_status_t oximeter5_init(oximeter5_t *ctx, const oximeter5_cfg_t *cfg)
{
    uint8_t part = 0U;
    uint16_t i;
    if ((ctx == 0) || (cfg == 0)) return OXIMETER5_ERROR;

    ctx->address = cfg->i2c_address;
    ctx->int_pin = cfg->int_pin;
    ctx->sample_count = 0U;
    ctx->warmup_samples = 0U;
    ctx->samples_since_calculation = 0U;
    ctx->data.sensor_valid = 0U;
    ctx->data.finger_present = 0U;
    ctx->data.spo2_valid = 0U;
    ctx->data.spo2_percent = OXIMETER5_SPO2_ERROR;
    ctx->data.heart_rate_valid = 0U;
    ctx->data.heart_rate_bpm = OXIMETER5_HEART_RATE_ERROR;
    ctx->data.temperature_valid = 0U;
    ctx->data.temperature_deci_c = 0;
    ctx->data.ir_raw = 0U;
    ctx->data.red_raw = 0U;
    for (i = 0U; i < OXIMETER5_BUFFER_SIZE; ++i) {
        ctx->ir_buffer[i] = 0U;
        ctx->red_buffer[i] = 0U;
    }

    if (bm_i2c_soft_init(&ctx->i2c, cfg->scl, cfg->sda, cfg->i2c_half_period_us) != BM_I2C_OK) return OXIMETER5_ERROR;
    bm_gpio_init_input(ctx->int_pin, 1U);
    if (reg_read(ctx, OXIMETER5_REG_PART_ID, &part, 1U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (part != OXIMETER5_PART_ID_MAX30102) return OXIMETER5_ERROR;
    ctx->data.sensor_valid = 1U;
    return OXIMETER5_OK;
}

oximeter5_status_t oximeter5_default_cfg(oximeter5_t *ctx)
{
    uint8_t status;
    if (ctx == 0) return OXIMETER5_ERROR;

    if (reg_write(ctx, OXIMETER5_REG_MODE_CONFIG, OXIMETER5_SW_RESET) != OXIMETER5_OK) return OXIMETER5_ERROR;
    Delay_ms(1000U);
    if (reg_read(ctx, OXIMETER5_REG_INTR_STATUS_1, &status, 1U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_INTR_ENABLE_1, OXIMETER5_INTR_FIFO_FULL_PPG) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_INTR_ENABLE_2, 0x00U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_FIFO_WR_PTR, 0x00U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_OVF_COUNTER, 0x00U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_FIFO_RD_PTR, 0x00U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_FIFO_CONFIG, OXIMETER5_FIFO_AVG_4_AFULL_15) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_MODE_CONFIG, OXIMETER5_MODE_SPO2) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_SPO2_CONFIG, OXIMETER5_SPO2_4096_100HZ_18BIT) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_LED1_PA, OXIMETER5_LED_7_2_MA) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_LED2_PA, OXIMETER5_LED_7_2_MA) != OXIMETER5_OK) return OXIMETER5_ERROR;
    return OXIMETER5_OK;
}

static oximeter5_status_t read_sensor_data(oximeter5_t *ctx, uint32_t *ir, uint32_t *red)
{
    uint8_t rx[6];
    if (reg_read(ctx, OXIMETER5_REG_FIFO_DATA, rx, 6U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    *ir = (((uint32_t)rx[0] << 16) | ((uint32_t)rx[1] << 8) | rx[2]) & DATA_18_BIT;
    *red = (((uint32_t)rx[3] << 16) | ((uint32_t)rx[4] << 8) | rx[5]) & DATA_18_BIT;
    return OXIMETER5_OK;
}

oximeter5_status_t oximeter5_read_temperature_deci_c(oximeter5_t *ctx, int16_t *temperature_deci_c)
{
    uint8_t integer_raw;
    uint8_t frac_raw;
    int16_t integer_signed;
    int16_t deci;
    if ((ctx == 0) || (temperature_deci_c == 0)) return OXIMETER5_ERROR;
    if (reg_write(ctx, OXIMETER5_REG_TEMP_CONFIG, 0x01U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    Delay_ms(10U);
    if (reg_read(ctx, OXIMETER5_REG_TEMP_INTR, &integer_raw, 1U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    if (reg_read(ctx, OXIMETER5_REG_TEMP_FRAC, &frac_raw, 1U) != OXIMETER5_OK) return OXIMETER5_ERROR;
    integer_signed = (integer_raw & 0x80U) ? (int16_t)integer_raw - 256 : (int16_t)integer_raw;
    frac_raw &= 0x0FU;
    deci = (int16_t)(integer_signed * 10);
    deci += (int16_t)(((uint16_t)frac_raw * 10U + 8U) / 16U);
    *temperature_deci_c = deci;
    return OXIMETER5_OK;
}

oximeter5_status_t oximeter5_service(oximeter5_t *ctx)
{
    uint32_t ir;
    uint32_t red;
    uint16_t i;
    uint8_t spo2;
    int32_t heart_rate;
    oximeter5_status_t spo2_status;
    oximeter5_status_t hr_status;

    if ((ctx == 0) || !ctx->data.sensor_valid) return OXIMETER5_ERROR;

    /* MAX30102 INT is active-low. Nothing to consume while the pin is high. */
    if (bm_gpio_read(ctx->int_pin)) return OXIMETER5_OK;
    /* Match the supplied example: FIFO channel 1 is RED, channel 2 is IR. */
    if (read_sensor_data(ctx, &red, &ir) != OXIMETER5_OK) return OXIMETER5_ERROR;

    ctx->data.ir_raw = ir;
    ctx->data.red_raw = red;
    if (ctx->warmup_samples < 250U) ctx->warmup_samples++;
    ctx->data.finger_present = (ir > OXIMETER5_FINGER_THRESHOLD) ? 1U : 0U;

    if (ctx->sample_count < OXIMETER5_BUFFER_SIZE) {
        ctx->ir_buffer[ctx->sample_count] = ir;
        ctx->red_buffer[ctx->sample_count] = red;
        ctx->sample_count++;
        if (ctx->sample_count < OXIMETER5_BUFFER_SIZE) return OXIMETER5_OK;
        ctx->samples_since_calculation = 25U;
    } else {
        for (i = 1U; i < OXIMETER5_BUFFER_SIZE; ++i) {
            ctx->ir_buffer[i - 1U] = ctx->ir_buffer[i];
            ctx->red_buffer[i - 1U] = ctx->red_buffer[i];
        }
        ctx->ir_buffer[OXIMETER5_BUFFER_SIZE - 1U] = ir;
        ctx->red_buffer[OXIMETER5_BUFFER_SIZE - 1U] = red;
        if (ctx->samples_since_calculation < 255U) ctx->samples_since_calculation++;
    }

    if (ctx->samples_since_calculation >= 25U) {
        ctx->samples_since_calculation = 0U;
        spo2 = OXIMETER5_SPO2_ERROR;
        heart_rate = OXIMETER5_HEART_RATE_ERROR;
        spo2_status = oximeter5_get_oxygen_saturation(ctx->ir_buffer, OXIMETER5_BUFFER_SIZE,
                                                       ctx->red_buffer, &spo2);
        hr_status = oximeter5_get_heart_rate(ctx->ir_buffer, OXIMETER5_BUFFER_SIZE,
                                             ctx->red_buffer, &heart_rate);
        ctx->data.spo2_valid = (ctx->data.finger_present &&
                                (ctx->warmup_samples >= 250U) &&
                                (spo2_status == OXIMETER5_OK) &&
                                (spo2 <= 100U)) ? 1U : 0U;
        ctx->data.spo2_percent = ctx->data.spo2_valid ? spo2 : OXIMETER5_SPO2_ERROR;
        ctx->data.heart_rate_valid = (ctx->data.finger_present &&
                                      (hr_status == OXIMETER5_OK) &&
                                      (heart_rate > 0)) ? 1U : 0U;
        ctx->data.heart_rate_bpm = ctx->data.heart_rate_valid ? heart_rate : OXIMETER5_HEART_RATE_ERROR;
    }

    return OXIMETER5_OK;
}

const oximeter5_data_t *oximeter5_get_data(const oximeter5_t *ctx)
{
    return (ctx != 0) ? &ctx->data : 0;
}

oximeter5_status_t oximeter5_get_oxygen_saturation ( uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, uint8_t *pn_spo2 )
{
    uint32_t un_ir_mean;
    int32_t n_i_ratio_count;
    int32_t n_exact_ir_valley_locs_count, n_middle_idx;
    int32_t n_th1, n_npks;   
    int32_t an_ir_valley_locs[ 15 ];
    int32_t n_y_ac, n_x_ac;
    int32_t n_spo2_calc; 
    int32_t n_y_dc_max, n_x_dc_max; 
    int32_t n_y_dc_max_idx, n_x_dc_max_idx; 
    int32_t an_ratio[ 5 ], n_ratio_average; 
    int32_t n_nume, n_denom ;
    int32_t an_x[ BUFFER_SIZE ];
    int32_t an_y[ BUFFER_SIZE ];
    oximeter5_status_t error_flag;

    un_ir_mean = 0; 
    
    for ( int32_t n_cnt_k = 0; n_cnt_k < n_ir_buffer_length; n_cnt_k++ ) 
    {
        un_ir_mean += pun_ir_buffer[ n_cnt_k ];
    }

    un_ir_mean =un_ir_mean/n_ir_buffer_length ;
    
    // remove DC and invert signal so that we can use peak detector as valley detector
    for ( int32_t n_cnt_k = 0; n_cnt_k < n_ir_buffer_length; n_cnt_k++ )
    {
        an_x[ n_cnt_k ] = -1*(pun_ir_buffer[ n_cnt_k ] - un_ir_mean );
    }
    
    // 4 pt Moving Average
    for ( int32_t n_cnt_k = 0; n_cnt_k < BUFFER_SIZE-MA4_SIZE; n_cnt_k++ )
    {
        an_x[ n_cnt_k ] = ( an_x[ n_cnt_k ] + an_x[ n_cnt_k + 1 ] + an_x[ n_cnt_k + 2 ] + an_x[ n_cnt_k + 3 ] ) / 4;        
    }
  
    // calculate threshold  
    n_th1 = 0; 
    for ( int32_t n_cnt_k = 0; n_cnt_k < BUFFER_SIZE; n_cnt_k++ )
    {
        n_th1 +=  an_x[ n_cnt_k ];
    }
    
    n_th1=  n_th1 / BUFFER_SIZE;
    if ( n_th1 < 30 )
    {
        n_th1 = 30; // min allowed
    }
    
    if ( n_th1 > 60 )
    {
        n_th1 = 60; // max allowed
    } 

    for ( int32_t n_cnt_k = 0; n_cnt_k < 15; n_cnt_k++ )
    {
        an_ir_valley_locs[ n_cnt_k ] = 0;
    }
  
    // since we flipped signal, we use peak detector as valley detector
    dev_find_peaks( an_ir_valley_locs, &n_npks, an_x, BUFFER_SIZE, n_th1, 4, 15 );//peak_height, peak_distance, max_num_peaks 
  
    //  load raw value again for SPO2 calculation : RED(=y) and IR(=X)
    for ( int32_t n_cnt_k = 0; n_cnt_k < n_ir_buffer_length; n_cnt_k++ )
    {
        an_x[ n_cnt_k ] = pun_ir_buffer[ n_cnt_k ]; 
        an_y[ n_cnt_k ] = pun_red_buffer[ n_cnt_k ]; 
    }

    // find precise min near an_ir_valley_locs
    n_exact_ir_valley_locs_count = n_npks; 
  
    //using exact_ir_valley_locs , find ir-red DC andir-red AC for SPO2 calibration an_ratio
    //finding AC/DC maximum of raw
    n_ratio_average = 0; 
    n_i_ratio_count = 0; 
    
    for ( int32_t n_cnt_k = 0; n_cnt_k < 5; n_cnt_k++ )
    {
        an_ratio[ n_cnt_k ] = 0;
    }
  
    for ( int32_t n_cnt_k = 0; n_cnt_k < n_exact_ir_valley_locs_count; n_cnt_k++ )
    {
        if ( an_ir_valley_locs[ n_cnt_k ] > BUFFER_SIZE )
        {
            // do not use SPO2 since valley loc is out of range
            *pn_spo2 = OXIMETER5_SPO2_ERROR; 
            error_flag  = OXIMETER5_ERROR; 
        }
    }
  
    // find max between two valley locations 
    // and use an_ratio betwen AC compoent of Ir & Red and DC compoent of Ir & Red for SPO2 
    for ( int32_t n_cnt_k = 0; n_cnt_k < n_exact_ir_valley_locs_count - 1; n_cnt_k++ )
    {
        n_y_dc_max= OXIMETER5_N_X_DC_MAX; 
        n_x_dc_max= OXIMETER5_N_X_DC_MAX; 
        
        if ( an_ir_valley_locs[ n_cnt_k + 1 ] - an_ir_valley_locs[ n_cnt_k ] > 3 )
        {
            for ( int32_t n_cnt_i = an_ir_valley_locs[ n_cnt_k]; n_cnt_i < an_ir_valley_locs[ n_cnt_k + 1 ]; n_cnt_i++ )
            {
                if ( an_x[ n_cnt_i ] > n_x_dc_max )
                {
                    n_x_dc_max = an_x[ n_cnt_i ]; 
                    n_x_dc_max_idx = n_cnt_i;
                }
                
                if ( an_y[ n_cnt_i ] > n_y_dc_max )
                {
                    n_y_dc_max = an_y[ n_cnt_i ];
                    n_y_dc_max_idx = n_cnt_i;
                    
                }
            }
            
            //red
            n_y_ac = ( an_y[ an_ir_valley_locs[ n_cnt_k + 1 ] ] - an_y[ an_ir_valley_locs[ n_cnt_k ] ] ) * ( n_y_dc_max_idx - an_ir_valley_locs[ n_cnt_k ] ); 
            n_y_ac =  an_y[an_ir_valley_locs[ n_cnt_k ] ] + n_y_ac / ( an_ir_valley_locs[ n_cnt_k + 1 ] - an_ir_valley_locs[ n_cnt_k ] ); 
            // subracting linear DC compoenents from raw
            n_y_ac =  an_y[ n_y_dc_max_idx ] - n_y_ac;     
            // ir
            n_x_ac = ( an_x[ an_ir_valley_locs[ n_cnt_k + 1 ] ] - an_x[ an_ir_valley_locs[ n_cnt_k ] ] ) * ( n_x_dc_max_idx - an_ir_valley_locs[ n_cnt_k ] ); 
            // subracting linear DC compoenents from raw 
            n_x_ac =  an_x[ an_ir_valley_locs[ n_cnt_k ] ] + n_x_ac / ( an_ir_valley_locs[ n_cnt_k + 1 ] - an_ir_valley_locs[ n_cnt_k ] ); 
            n_x_ac =  an_x[ n_y_dc_max_idx ] - n_x_ac;
            //prepare X100 to preserve floating value
            n_nume =( n_y_ac * n_x_dc_max ) >> 7; 
            n_denom = ( n_x_ac * n_y_dc_max ) >> 7;
            
            if ( ( n_denom > 0 )  && ( n_i_ratio_count < 5 ) && ( n_nume != 0 ) )
            {   
                an_ratio[ n_i_ratio_count ] = ( n_nume * 100 ) / n_denom;
                n_i_ratio_count++;
            }
        }
    }
  
    // choose median value since PPG signal may varies from beat to beat
    dev_sort_ascend( an_ratio, n_i_ratio_count );
    n_middle_idx = n_i_ratio_count / 2;

    if ( n_middle_idx > 1 )
    {
        // use median
        n_ratio_average = ( an_ratio[ n_middle_idx - 1 ] + an_ratio[ n_middle_idx ] ) / 2; 
    }
    else
    {
        n_ratio_average = an_ratio[ n_middle_idx ];
    }
    
    if ( ( n_ratio_average > 2 ) && ( n_ratio_average < 184 ) )
    {
        n_spo2_calc = uch_spo2_table[ n_ratio_average ];
        *pn_spo2 = n_spo2_calc;
        error_flag = OXIMETER5_OK;
    }
    else
    {
        *pn_spo2 = OXIMETER5_SPO2_ERROR;
        error_flag  = OXIMETER5_ERROR; 
    }
  
    return error_flag;
}

oximeter5_status_t oximeter5_get_heart_rate ( uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, int32_t *pn_heart_rate )
{
    (void)pun_red_buffer;
    uint32_t un_ir_mean;
    int32_t n_th1, n_npks;   
    int32_t an_ir_valley_locs[ 15 ];
    int32_t n_peak_interval_sum;
    int32_t an_x[ BUFFER_SIZE ];
     oximeter5_status_t error_flag;

    // calculates DC mean and subtract DC from ir
    un_ir_mean = 0; 
    for ( int32_t n_cnt_k = 0; n_cnt_k < n_ir_buffer_length; n_cnt_k++ ) 
    {
        un_ir_mean += pun_ir_buffer[ n_cnt_k ];
    }
    
    un_ir_mean = un_ir_mean / n_ir_buffer_length;
    
    // remove DC and invert signal so that we can use peak detector as valley detector
    for ( int32_t n_cnt_k = 0; n_cnt_k < n_ir_buffer_length; n_cnt_k++ )
    {
        an_x[ n_cnt_k ] = -1 * ( pun_ir_buffer[ n_cnt_k ] - un_ir_mean );
    }
    
    // 4 pt Moving Average
    for( int32_t n_cnt_k = 0; n_cnt_k < BUFFER_SIZE - MA4_SIZE; n_cnt_k++ )
    {
        an_x[ n_cnt_k ]=( an_x[ n_cnt_k ] + an_x[ n_cnt_k + 1 ] + an_x[ n_cnt_k + 2 ] + an_x[ n_cnt_k + 3 ] ) / 4;        
    }
  
    // calculate threshold  
    n_th1 = 0; 
    for ( int32_t n_cnt_k = 0; n_cnt_k < BUFFER_SIZE; n_cnt_k++ )
    {
        n_th1 +=  an_x[ n_cnt_k ];
    }
  
    n_th1 = n_th1 / BUFFER_SIZE;
  
    if ( n_th1 < 30 ) 
    {
        n_th1 = 30; // min allowed
    }
    
    if( n_th1 > 60 )
    {
        n_th1 = 60; // max allowed
    }

    for ( int32_t n_cnt_k = 0; n_cnt_k < 15; n_cnt_k++ )
    {
        an_ir_valley_locs[ n_cnt_k ] = 0;
    }
    
    // since we flipped signal, we use peak detector as valley detector
    dev_find_peaks( an_ir_valley_locs, &n_npks, an_x, BUFFER_SIZE, n_th1, 4, 15 );//peak_height, peak_distance, max_num_peaks 
  
    n_peak_interval_sum = 0;
  
    if ( n_npks >= 2 )
    {
        for ( int32_t n_cnt_k = 1; n_cnt_k < n_npks; n_cnt_k++ )
        {
            n_peak_interval_sum += ( an_ir_valley_locs[ n_cnt_k ] -an_ir_valley_locs[ n_cnt_k - 1 ] );
        }
        
        n_peak_interval_sum = n_peak_interval_sum / ( n_npks - 1 );
        *pn_heart_rate = ( int32_t ) ( ( SAMPLING_FREQUENCY * 60 ) / n_peak_interval_sum );
        error_flag  = OXIMETER5_OK;
    }
    else  
    { 
        *pn_heart_rate = OXIMETER5_HEART_RATE_ERROR; // unable to calculate because # of peaks are too small
        error_flag  = OXIMETER5_ERROR;
    }
  
  return error_flag;

}

static void dev_peaks_above_min_height ( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, uint8_t n_size, int32_t n_min_height )
{
    uint8_t n_width;
    uint8_t n_cnt = 1;
    
    *n_npks = 0;
  
    while ( n_cnt < ( n_size - 1 ) )
    {
        if ( pn_x[ n_cnt ] > n_min_height && pn_x[ n_cnt ] > pn_x[ n_cnt - 1 ] ) 
        {    
            n_width = 1;
            
            while ( n_cnt + n_width < n_size && pn_x[ n_cnt ] == pn_x[ n_cnt + n_width ] ) 
            {
                n_width++;    
            }
        
            if ( pn_x[ n_cnt ] > pn_x[ n_cnt + n_width ] && ( *n_npks ) < 15 )
            {      
                pn_locs[( *n_npks )++ ] = n_cnt;
                n_cnt += n_width + 1;
            }
            else
            {
                n_cnt += n_width;    
            }
        }
        else
        {
            n_cnt++;    
        }
    }
}


static void dev_sort_indices_descend ( int32_t *pn_x, int32_t *pn_indx, int32_t n_size )
{
    int32_t n_temp;
    
    for ( int32_t n_cnt_i = 1; n_cnt_i < n_size; n_cnt_i++ ) 
    {
        n_temp = pn_indx[ n_cnt_i ];
        
        int32_t n_cnt_j;
        for ( n_cnt_j = n_cnt_i; n_cnt_j > 0 && pn_x[ n_temp ] > pn_x[ pn_indx[ n_cnt_j - 1 ] ]; n_cnt_j-- )
        {
            pn_indx[ n_cnt_j ] = pn_indx[ n_cnt_j - 1 ];
        }
      
        pn_indx[ n_cnt_j ] = n_temp;
    }
}

static void dev_sort_ascend ( int32_t  *pn_x, int32_t n_size ) 
{
    int32_t n_temp;
    
    for ( int32_t n_cnt_i = 1; n_cnt_i < n_size; n_cnt_i++ ) 
    {
        n_temp = pn_x[ n_cnt_i ];
        
        int32_t n_cnt_j;
        for ( n_cnt_j = n_cnt_i; n_cnt_j > 0 && n_temp < pn_x[ n_cnt_j - 1 ]; n_cnt_j-- )
        {
            pn_x[ n_cnt_j ] = pn_x[ n_cnt_j - 1 ];
        }
        
        pn_x[ n_cnt_j ] = n_temp;
    }
}

static void dev_remove_close_peaks ( int32_t *pn_locs, int32_t *pn_npks, int32_t *pn_x, int32_t n_min_distance )
{
    int32_t n_old_npks, n_dist;
    
    dev_sort_indices_descend( pn_x, pn_locs, *pn_npks );

    for ( int32_t n_cnt_i = -1; n_cnt_i < *pn_npks; n_cnt_i++ )
    {
        n_old_npks = *pn_npks;
        *pn_npks = n_cnt_i + 1;
    
        for ( int32_t n_cnt_j = n_cnt_i + 1; n_cnt_j < n_old_npks; n_cnt_j++ )
        {
            n_dist =  pn_locs[ n_cnt_j ] - ( n_cnt_i == -1 ? -1 : pn_locs[ n_cnt_i ] ); 
      
            if ( n_dist > n_min_distance || n_dist < -n_min_distance )
            {
                pn_locs[ (*pn_npks)++ ] = pn_locs[ n_cnt_j ];
            }
        }
    }

    dev_sort_ascend( pn_locs, *pn_npks );
}

static void dev_find_peaks ( int32_t *pn_locs, int32_t *n_npks,  int32_t  *pn_x, uint8_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num )
{
    dev_peaks_above_min_height( pn_locs, n_npks, pn_x, n_size, n_min_height );
    dev_remove_close_peaks( pn_locs, n_npks, pn_x, n_min_distance );
    if ( *n_npks > n_max_num )
    {
        *n_npks = n_max_num;
    }
}

// ------------------------------------------------------------------------- END
