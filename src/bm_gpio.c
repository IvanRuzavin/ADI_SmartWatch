#include "bm_gpio.h"

static void bm_gpio_enable_clock(mxc_gpio_regs_t *port)
{
    if (port == MXC_GPIO0) {
        MXC_GCR->pclkdis0 &= ~MXC_F_GCR_PCLKDIS0_GPIO0;
    } else if (port == MXC_GPIO1) {
        MXC_GCR->pclkdis0 &= ~MXC_F_GCR_PCLKDIS0_GPIO1;
    } else if (port == MXC_GPIO2) {
        MXC_GCR->pclkdis0 &= ~MXC_F_GCR_PCLKDIS0_GPIO2;
    } else if (port == MXC_GPIO3) {
        MXC_LPGCR->pclkdis &= ~MXC_F_LPGCR_PCLKDIS_GPIO3;
    }
}

static void bm_gpio_select_3v3_domain(bm_gpio_pin_t pin)
{
    /* GPIO0/1/2 support per-pin VDDIOH selection. VDDIOH must be powered at 3.3 V. */
    if ((pin.port == MXC_GPIO0) ||
        (pin.port == MXC_GPIO1) ||
        (pin.port == MXC_GPIO2)) {
        pin.port->vssel |= pin.mask;
    }
}

static void bm_gpio_common_pad_config(bm_gpio_pin_t pin)
{
    bm_gpio_select_3v3_domain(pin);

    /* No internal pull by default. PADCTRL[1:0] = 00 -> high impedance. */
    pin.port->padctrl0 &= ~pin.mask;
    pin.port->padctrl1 &= ~pin.mask;

    /* Conservative drive/slew configuration. */
    pin.port->ds0 &= ~pin.mask;
    pin.port->ds1 &= ~pin.mask;
    pin.port->srsel &= ~pin.mask;

    pin.port->inten_clr = pin.mask;
    pin.port->wken_clr  = pin.mask;
}

static void bm_gpio_select_gpio_function(bm_gpio_pin_t pin)
{
    /* GPIO function: EN0=1, EN1=0, EN2=0. */
    pin.port->en0_set = pin.mask;
    pin.port->en1_clr = pin.mask;
    pin.port->en2_clr = pin.mask;
}

void bm_gpio_init_output(bm_gpio_pin_t pin, uint8_t initial_high)
{
    bm_gpio_enable_clock(pin.port);
    bm_gpio_common_pad_config(pin);
    bm_gpio_select_gpio_function(pin);

    pin.port->inen &= ~pin.mask;

    if (initial_high) {
        pin.port->out_set = pin.mask;
    } else {
        pin.port->out_clr = pin.mask;
    }
    pin.port->outen_set = pin.mask;
}

void bm_gpio_init_input(bm_gpio_pin_t pin, uint8_t pull_up)
{
    bm_gpio_enable_clock(pin.port);
    bm_gpio_common_pad_config(pin);
    bm_gpio_select_gpio_function(pin);

    pin.port->outen_clr = pin.mask;
    pin.port->inen |= pin.mask;

    if (pull_up) {
        /* PADCTRL[1:0] = 01 -> pull-up. */
        pin.port->padctrl1 &= ~pin.mask;
        pin.port->padctrl0 |= pin.mask;
    }
}

void bm_gpio_init_open_drain(bm_gpio_pin_t pin)
{
    bm_gpio_enable_clock(pin.port);
    bm_gpio_common_pad_config(pin);
    bm_gpio_select_gpio_function(pin);

    /* Input remains enabled so SDA/SCL can be sampled while released. */
    pin.port->inen |= pin.mask;

    /* Latch low permanently; OUTEN controls low vs released/high-Z. */
    pin.port->out_clr = pin.mask;
    pin.port->outen_clr = pin.mask;
}

void bm_gpio_init_alt1(bm_gpio_pin_t pin)
{
    bm_gpio_enable_clock(pin.port);
    bm_gpio_common_pad_config(pin);

    /* MAX32690 ALT1: EN0=0, EN1=0, EN2=0. */
    pin.port->outen_clr = pin.mask;
    pin.port->inen |= pin.mask;
    pin.port->en2_clr = pin.mask;
    pin.port->en1_clr = pin.mask;
    pin.port->en0_clr = pin.mask;
}


void bm_gpio_init_alt2(bm_gpio_pin_t pin)
{
    bm_gpio_enable_clock(pin.port);
    bm_gpio_common_pad_config(pin);

    /* MAX32690 ALT2: EN0=0, EN1=1, EN2=0 (EN3=0 on RevA GPIO). */
    pin.port->outen_clr = pin.mask;
    pin.port->inen |= pin.mask;
    pin.port->en2_clr = pin.mask;
    pin.port->en1_set = pin.mask;
    pin.port->en0_clr = pin.mask;
}

void bm_gpio_write(bm_gpio_pin_t pin, uint8_t high)
{
    if (high) {
        pin.port->out_set = pin.mask;
    } else {
        pin.port->out_clr = pin.mask;
    }
}

void bm_gpio_set(bm_gpio_pin_t pin)
{
    pin.port->out_set = pin.mask;
}

void bm_gpio_clear(bm_gpio_pin_t pin)
{
    pin.port->out_clr = pin.mask;
}

uint8_t bm_gpio_read(bm_gpio_pin_t pin)
{
    return (pin.port->in & pin.mask) ? 1U : 0U;
}

void bm_gpio_od_low(bm_gpio_pin_t pin)
{
    pin.port->out_clr = pin.mask;
    pin.port->outen_set = pin.mask;
}

void bm_gpio_od_release(bm_gpio_pin_t pin)
{
    pin.port->outen_clr = pin.mask;
}
