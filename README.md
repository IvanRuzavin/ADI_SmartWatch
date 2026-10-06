# MAX32690 RTC24 + Thermo21 + Oximeter5 + IPS Display 2 Dashboard

Bare-metal MAX32690 example using only the MCU register headers (`mcu.h`) and delay library (`delays.h`) on the platform side.

## Click placement

| mikroBUS | Click | Interface | Pins used |
|---|---|---|---|
| 1 | IPS Display 2 | SPI1A + GPIO | SCK P1.26, MISO P1.28, MOSI P1.29, CS P1.27, RST P2.24, D/C P0.23, BCK P3.05 |
| 2 | Oximeter 5 | software I2C + INT | SCL P0.14, SDA P0.13, INT P2.23 |
| 3 | Thermo 21 | SPI0B + GPIO CS | SCK P2.29, MISO P2.27, MOSI P2.28, CS P2.30 |
| 4 | RTC 24 | software I2C + SQW | SCL P0.31, SDA P0.30, SQW P0.09 |

GPIO0/1/2 pins are configured for the MAX32690 VDDIOH domain. VDDIOH must be physically supplied with 3.3 V.

## Buttons

- B1 `P0.02`: enter clock editor / save and exit
- B2 `P0.18`: next field / back from edit to field selection
- B3 `P2.13`: edit selected field
- B4 `P2.22`: increment
- B5 `P0.03`: decrement
- B6 `P2.25`: next screen

B4/B5 change once immediately. Holding either for 1 second starts auto-repeat every 120 ms until the button is released.

## Screen 1 - Clock dashboard

- RTC24-backed 12-hour `HH:MM:SS` clock with AM/PM
- Date `MM.DD.YYYY`
- Weekday
- Incremental digit redraw: only digits whose values changed are painted
- Thermo 21 temperature card
- Oximeter 5 SpO2 card

SpO2 is left unavailable until the Oximeter 5 has collected roughly 10 seconds of samples, matching the validity note in the supplied example.

## Screen 2 - Temperature

- Thermo 21 numeric temperature remains unrestricted by the graphics and displays the actual sensor result
- Thermometer level is graphically clamped to 25.0..43.0 C
- Below 25 C the graphic stays at minimum; above 43 C it stays full
- Fill color changes with temperature
- No humidity card and no `SCREEN 2`/`B1 --` hints
- B6 advances to the Oximeter screen

## Screen 3 - Oximeter 5

The Oximeter 5 / MAX30102 driver is adapted from the supplied package to the existing bare-metal software-I2C layer. It uses address `0x57` and the package's configuration/algorithms.

Displayed values:

- SpO2 (%)
- heart rate (BPM)
- IR ADC
- RED ADC
- MAX30102 die temperature
- finger/sensor status

The sample path is non-blocking. Raw values are sampled continuously but their UI is throttled to 250 ms to avoid unnecessary display traffic. SpO2 and heart rate use the supplied 100-sample algorithm at an effective 25 samples/s.

## Screen order

`CLOCK -> TEMPERATURE -> OXIMETER 5 -> CLOCK`

`APP_SCREEN_COUNT` and the screen enum make this cycle easy to extend later.

## Source layout

- `src/` - MAX32690 bare-metal GPIO/SPI/software-I2C/buttons
- `clicks/ips_display2/` - ST7789 display driver
- `clicks/rtc24/` - RTC24 driver
- `clicks/thermo21/` - ADT7301 temperature driver
- `clicks/oximeter5/` - MAX30102/Oximeter 5 driver and algorithms
- `ui/` - graphics, dashboard screens, clock editor
