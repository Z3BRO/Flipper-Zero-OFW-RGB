/*
    RGB backlight FlipperZero driver
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

#pragma once

#include <furi.h>
#include "SK6805.h"

#define LED_BACKLIGHT_COUNT 3
#define LED_INTERNAL_COUNT 12
#define LED_COUNT (LED_BACKLIGHT_COUNT + LED_INTERNAL_COUNT)

typedef struct {
    char* name;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGBColor;

typedef struct {
    char* name;
    uint8_t length;
    uint8_t index[12];
} InternalPattern;

typedef enum {
    BacklightModeConstant,
} BacklightMode;

typedef enum {
    InternalModeMatch, // "AUTO" - Turns on when backlight is on.
    InternalModeOn, // Always on.
    InternalModeCount, // Must be last item.  This is the total number of modes.
} InternalMode;

typedef struct {
    uint8_t version;
    uint8_t backlight_colors[LED_BACKLIGHT_COUNT][3]; // RGB
    BacklightMode backlight_mode;
    uint8_t internal_pattern_index;
    float internal_brightness; // Scale (0 to 1.0f)
    InternalMode internal_mode;
    bool settings_loaded;
} RGBBacklightSettings;

/**
 * @brief Loads the .rgb_backlight.settings file
 * 
 * @details In DFU mode, sets settings_loaded to true (and uses defaults).
 * Otherwise, loads the settings from the .rgb_backlight.settings file.
 */
void rgb_backlight_load_settings(void);

/**
 * @brief Saves the settings to the .rgb_backlight.settings file
 */
void rgb_backlight_save_settings(void);

/**
 * @brief Returns the RGB backlight settings.
 * 
 * @details This API returns the RGB backlight settings.  If the settings 
 * were not already loaded, this API will first load the settings.
 * 
 * @return RGBBacklightSettings* 
 */
RGBBacklightSettings* rgb_backlight_get_settings(void);

/**
 * @brief Returns the number of colors available to set the LED to.
 * 
 * @return uint8_t total number of colors
 */
uint8_t rgb_backlight_color_count(void);

/**
 * @brief Returns the name of the color at the specified index.
 * 
 * @param index_color The index of the color
 * @return const char* The name of the color
 */
const char* rgb_backlight_color_text(uint8_t index_color);

/**
 * @brief Gets the RGB colors for the color at the the specified index.
 * 
 * @param index_color The index of the color
 * @param red 0-255 Red value
 * @param green 0-255 Green value
 * @param blue 0-255 Blue value
 */
void rgb_backlight_color_value(uint8_t index_color, uint8_t* red, uint8_t* green, uint8_t* blue);

/**
 * @brief Sets the RGB color of the LED. 
 * 
 * @param led_number the LED number (starting at 0) 
 * @param red 0-255 red value
 * @param green 0-255 green value
 * @param blue 0-255 blue value
 */
void rgb_backlight_led_set_color(uint8_t led_number, uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Gets the RGB color of the LED.
 * 
 * @param led_number the LED number (starting at 0) 
 * @param red Storage for returning the red (0-255) value
 * @param green Storage for returning the green (0-255) value
 * @param blue Storage for returning the blue (0-255) value
 */
void rgb_backlight_led_get_color(uint8_t led_number, uint8_t* red, uint8_t* green, uint8_t* blue);

/**
 * @brief Returns the closest RGB color index for the specified LED
 * 
 * @param led_number the LED number (starting at 0)
 * @return uint8_t The index of the rgb backlight color
 */
uint8_t rgb_backlight_find_index(uint8_t led_number);

/**
 * @brief Returns the number of patterns available to set the LED to.
 * 
 * @return uint8_t total number of patterns
 */
uint8_t rgb_internal_pattern_count(void);

/**
 * @brief Returns the name of the pattern at the specified index.
 * 
 * @param index_pattern The index of the pattern
 * @return const char* The name of the pattern
 */
const char* rgb_internal_pattern_text(uint8_t index_pattern);

/**
 * @brief Sets the pattern for the internal LEDs.
 * 
 * @param index_pattern The pattern for the internal LEDs.
 */
void rgb_internal_set_pattern(uint8_t index_pattern);

/**
 * @brief Sets the brightness for the internal LEDs.
 * 
 * @param brightness The scale factor (0 to 1.0f)
 */
void rgb_internal_set_brightness(float brightness);

/**
 * @brief Sets the mode for the internal LEDs.
 * 
 * @param mode This should be a value defined in InternalMode enum.
 */
void rgb_internal_set_mode(uint32_t mode);

/**
 * @brief Gets the maximum number of internal LEDs.
 * 
 * @return uint8_t The number of internal LEDs.
 */
uint8_t rgb_internal_led_get_count(void);

/**
 * @brief Determines if RGB Backlight or LCD Backlight should be used.
 * 
 * @details There is no way to detect if the hardware is connected to 
 * the vibro pin.  Instead we will return true if at least one of the
 * RGB LEDs have a color assigned.  If all are off, then we will return
 * false.  
 * 
 * @return true If at least one RGB LED is set to a color.
 * @return false If no RGB LED are set to a color.
 */
bool rgb_backlight_connected();

/**
 * @brief Updates the physical LEDs connected to the vibro pin.
 * 
 * @param brightness The brightness (0=OFF, 255=MAX)
 */
void rgb_backlight_update(uint8_t brightness);