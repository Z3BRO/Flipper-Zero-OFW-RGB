/*
    SK6805 FlipperZero driver
    Copyright (C) 2022-2023 Victor Nikitchuk (https://github.com/quen0n)
    Modified 2023-2024 Derek Jamison (https://github.com/jamisonderek)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "SK6805.h"
#include <furi_hal.h>

/* Настройки */
#define SK6805_LED_BACKLIGHT_COUNT 3
#define SK6805_LED_INTERNAL_COUNT 12
#define SK6805_LED_COUNT (SK6805_LED_BACKLIGHT_COUNT + SK6805_LED_INTERNAL_COUNT)
#define SK6805_LED_PIN &led_pin //Порт подключения светодиодов

#ifdef FURI_DEBUG
#define DEBUG_PIN &gpio_ext_pa7
#define DEBUG_INIT() \
    furi_hal_gpio_init(DEBUG_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh)
#define DEBUG_SET_HIGH() furi_hal_gpio_write(DEBUG_PIN, true)
#define DEBUG_SET_LOW() furi_hal_gpio_write(DEBUG_PIN, false)
#else
#define DEBUG_INIT()
#define DEBUG_SET_HIGH()
#define DEBUG_SET_LOW()
#endif

static const GpioPin led_pin = {.port = GPIOA, .pin = LL_GPIO_PIN_8}; // VIBRO
//static const GpioPin led_pin = {.port = GPIOA, .pin = LL_GPIO_PIN_7}; // PA7 (Unmod-testing)
static uint8_t led_buffer[SK6805_LED_COUNT][3];

void SK6805_init(void) {
    DEBUG_INIT();
    furi_hal_gpio_write(SK6805_LED_PIN, false);
    furi_hal_gpio_init(SK6805_LED_PIN, GpioModeOutputPushPull, GpioPullNo, GpioSpeedVeryHigh);
}

uint8_t SK6805_get_led_backlight_count(void) {
    return (const uint8_t)SK6805_LED_BACKLIGHT_COUNT;
}
uint8_t SK6805_get_led_internal_count(void) {
    return (const uint8_t)SK6805_LED_INTERNAL_COUNT;
}

void SK6805_set_led_color(uint8_t led_index, uint8_t r, uint8_t g, uint8_t b) {
    furi_check(led_index < SK6805_LED_COUNT);

    led_buffer[led_index][0] = g;
    led_buffer[led_index][1] = r;
    led_buffer[led_index][2] = b;
}

static void SK6805_internal_update(uint8_t led_count) {
    uint32_t end;

    /* Последовательная отправка цветов светодиодов */
    for(uint8_t lednumber = 0; lednumber < led_count; lednumber++) {
        //Последовательная отправка цветов светодиода
        for(uint8_t color = 0; color < 3; color++) {
            //Последовательная отправка битов цвета
            uint8_t i = 0b10000000;
            while(i != 0) {
                if(led_buffer[lednumber][color] & (i)) {
                    furi_hal_gpio_write(SK6805_LED_PIN, true);
                    DEBUG_SET_HIGH();
                    end = DWT->CYCCNT + 30;
                    //T1H 600 us (615 us)
                    while(DWT->CYCCNT < end) {
                    }
                    furi_hal_gpio_write(SK6805_LED_PIN, false);
                    DEBUG_SET_LOW();
                    end = DWT->CYCCNT + 26;
                    //T1L  600 us (587 us)
                    while(DWT->CYCCNT < end) {
                    }
                } else {
                    furi_hal_gpio_write(SK6805_LED_PIN, true);
                    DEBUG_SET_HIGH();
                    end = DWT->CYCCNT + 11;
                    //T0H 300 ns (312 ns)
                    while(DWT->CYCCNT < end) {
                    }
                    furi_hal_gpio_write(SK6805_LED_PIN, false);
                    DEBUG_SET_LOW();
                    end = DWT->CYCCNT + 43;
                    //T0L 900 ns (890 ns)
                    while(DWT->CYCCNT < end) {
                    }
                }
                i >>= 1;
            }
        }
    }
}

void SK6805_update(uint8_t led_count) {
    SK6805_init();
    if(led_count > SK6805_LED_COUNT) {
        led_count = SK6805_LED_COUNT;
    }
    furi_kernel_lock();
    uint32_t end = DWT->CYCCNT;
    SK6805_internal_update(led_count);
    end = DWT->CYCCNT - end;

    // FURI_LOG_D("SK6805", "Update time: %ld   Led count: %d", end, led_count);

    furi_kernel_unlock();
}