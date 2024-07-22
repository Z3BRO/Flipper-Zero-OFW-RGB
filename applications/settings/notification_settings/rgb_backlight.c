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

#include "rgb_backlight.h"
#include "rgb_backlight_colors.h"
#include <furi_hal.h>
#include <storage/storage.h>

#define RGB_BACKLIGHT_SETTINGS_VERSION   7
#define RGB_BACKLIGHT_SETTINGS_FILE_NAME ".rgb_backlight.settings"
#define RGB_BACKLIGHT_SETTINGS_PATH      EXT_PATH(RGB_BACKLIGHT_SETTINGS_FILE_NAME)
#define TAG                              "RGB Backlight"
#define RGB_BACKLIGHT_DEFAULT_RGB        {255, 79, 0} /* Orange */

// Pin mapping for backlight to virtual LED (TimedSignal & TimedRainbow modes)
#define RGB_BACKLIGHT_RAINBOW_S0 4
#define RGB_BACKLIGHT_RAINBOW_R0 2
#define RGB_BACKLIGHT_RAINBOW_R1 4
#define RGB_BACKLIGHT_RAINBOW_R2 6

// Comment the next line when using real hardware:
// #define USE_DEBUG_LED_STRIP 1

static RGBBacklightSettings rgb_settings = {
    .version = RGB_BACKLIGHT_SETTINGS_VERSION,
    .backlight_colors =
        {RGB_BACKLIGHT_DEFAULT_RGB, RGB_BACKLIGHT_DEFAULT_RGB, RGB_BACKLIGHT_DEFAULT_RGB},
    .backlight_mode = BacklightModeConstant,
    .internal_pattern_index = 2, // Rainbow
    .internal_brightness = 0.05f,
    .internal_mode = InternalModeMatch,
    .settings_loaded = false,
    .internal_color = {0, 0, 0},
    .rainbow_width = 270, // degrees Hue
    .rainbow_update_time = 250, // ms update delay
    .rainbow_spin_increment = 10, // degrees Hue increment
    .hardware_version = 1, // Version 1 (initial release of hardware)
};

typedef struct {
    FuriTimer* timer;
    uint8_t last_display_brightness;
    int16_t offset;
} RGBBacklightState;

static RGBBacklightState rgb_state = {
    .last_display_brightness = 123,
    .offset = 80,
    .timer = NULL,
};

void rgb_backlight_timer_callback(void* context) {
    UNUSED(context);
    if(!rgb_settings.settings_loaded) {
        return;
    }

    if(rgb_settings.backlight_mode == BacklightModeConstant &&
       rgb_settings.internal_pattern_index != 1 && rgb_settings.internal_pattern_index != 2) {
        return;
    }

    if(rgb_settings.rainbow_update_time) {
        rgb_state.offset += rgb_settings.rainbow_spin_increment;
        if(rgb_state.offset >= 360) {
            rgb_state.offset -= 360;
        } else if(rgb_state.offset < 0) {
            rgb_state.offset += 360;
        }
    }

    rgb_backlight_update(rgb_state.last_display_brightness);
}

void rgb_backlight_load_settings(void) {
    FuriHalRtcBootMode bm = furi_hal_rtc_get_boot_mode();
    if(bm == FuriHalRtcBootModeDfu) {
        rgb_settings.settings_loaded = true;
        return;
    }

    RGBBacklightSettings settings;
    File* file = storage_file_alloc(furi_record_open(RECORD_STORAGE));
    const size_t settings_size = sizeof(RGBBacklightSettings);
    memset(&settings, 0, settings_size);

    FURI_LOG_I(TAG, "loading settings from \"%s\"", RGB_BACKLIGHT_SETTINGS_PATH);
    bool fs_result =
        storage_file_open(file, RGB_BACKLIGHT_SETTINGS_PATH, FSAM_READ, FSOM_OPEN_EXISTING);

    uint16_t bytes_count;
    if(fs_result) {
        bytes_count = storage_file_read(file, &settings, settings_size);

        if(bytes_count == sizeof(RGBBacklightSettingsOriginal)) {
            settings.hardware_version = 1; // Version 1
        } else if(bytes_count != settings_size) {
            fs_result = false;
        }
    }

    if(fs_result) {
        FURI_LOG_I(TAG, "load success");
        if(settings.version != RGB_BACKLIGHT_SETTINGS_VERSION) {
            FURI_LOG_E(
                TAG,
                "version(%d != %d) mismatch",
                settings.version,
                RGB_BACKLIGHT_SETTINGS_VERSION);
        } else {
            if(settings.internal_pattern_index >= COUNT_OF(internal_pattern)) {
                settings.internal_pattern_index = COUNT_OF(internal_pattern) - 1;
            }
            memcpy(&rgb_settings, &settings, bytes_count);
        }
    } else {
        FURI_LOG_E(TAG, "load failed, %s", storage_file_get_error_desc(file));
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    rgb_settings.settings_loaded = true;

    if(bm == FuriHalRtcBootModeNormal && rgb_settings.rainbow_update_time) {
        if(rgb_state.timer == NULL) {
            rgb_state.timer =
                furi_timer_alloc(rgb_backlight_timer_callback, FuriTimerTypePeriodic, NULL);
            furi_timer_start(rgb_state.timer, rgb_settings.rainbow_update_time);
        }
    }
}

void rgb_backlight_save_settings(void) {
    RGBBacklightSettings settings;
    File* file = storage_file_alloc(furi_record_open(RECORD_STORAGE));
    const size_t settings_size = sizeof(RGBBacklightSettings);

    FURI_LOG_I(TAG, "saving settings to \"%s\"", RGB_BACKLIGHT_SETTINGS_PATH);

    memcpy(&settings, &rgb_settings, settings_size);
    if(settings.internal_pattern_index == COUNT_OF(internal_pattern) - 1) {
        settings.internal_pattern_index = 255;
    }

    bool fs_result =
        storage_file_open(file, RGB_BACKLIGHT_SETTINGS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS);

    if(fs_result) {
        uint16_t bytes_count = storage_file_write(file, &settings, settings_size);

        if(bytes_count != settings_size) {
            fs_result = false;
        }
    }

    if(fs_result) {
        FURI_LOG_I(TAG, "save success");
    } else {
        FURI_LOG_E(TAG, "save failed, %s", storage_file_get_error_desc(file));
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

RGBBacklightSettings* rgb_backlight_get_settings(void) {
    if(!rgb_settings.settings_loaded) {
        rgb_backlight_load_settings();
    }
    return &rgb_settings;
}

uint8_t rgb_backlight_color_count(void) {
    return COUNT_OF(rgb_colors);
}

const char* rgb_backlight_color_text(uint8_t index_color) {
    furi_assert(index_color < COUNT_OF(rgb_colors));
    return rgb_colors[index_color].name;
}

void rgb_backlight_color_value(uint8_t index_color, uint8_t* red, uint8_t* green, uint8_t* blue) {
    *red = rgb_colors[index_color].red;
    *green = rgb_colors[index_color].green;
    *blue = rgb_colors[index_color].blue;
}

void rgb_backlight_led_set_color(uint8_t led_number, uint8_t red, uint8_t green, uint8_t blue) {
    rgb_settings.backlight_colors[led_number][0] = red;
    rgb_settings.backlight_colors[led_number][1] = green;
    rgb_settings.backlight_colors[led_number][2] = blue;
}

void rgb_backlight_led_get_color(uint8_t led_number, uint8_t* red, uint8_t* green, uint8_t* blue) {
    *red = rgb_settings.backlight_colors[led_number][0];
    *green = rgb_settings.backlight_colors[led_number][1];
    *blue = rgb_settings.backlight_colors[led_number][2];
}

uint8_t rgb_backlight_find_index(uint8_t led_number) {
    uint8_t red, green, blue, r, g, b;
    uint8_t nearest_index = 0;
    uint32_t distance = 1 << 31;
    red = rgb_settings.backlight_colors[led_number][0];
    green = rgb_settings.backlight_colors[led_number][1];
    blue = rgb_settings.backlight_colors[led_number][2];

    for(int i = 0; i < rgb_backlight_color_count(); i++) {
        rgb_backlight_color_value(i, &r, &g, &b);
        // TODO: Scale RGB based on observed brightness?
        uint32_t d = (red - r) * (red - r) + (green - g) * (green - g) + (blue - b) * (blue - b);
        if(d <= distance) {
            nearest_index = i;
            distance = d;
        }
    }

    if(distance != 0) {
        nearest_index = 0; // Custom.
    }

    // Slot 0 is reserved for "SAME AS RGB 1" choice for other LEDs.
    if(led_number > 0) {
        nearest_index++;
    }

    return nearest_index;
}

BacklightMode rgb_backlight_get_mode(void) {
    return rgb_settings.backlight_mode;
}

void rgb_backlight_set_mode(BacklightMode mode) {
    rgb_settings.backlight_mode = mode;
}

void rgb_rainbow_set_width(uint16_t width) {
    rgb_settings.rainbow_width = width;
}

uint16_t rgb_rainbow_get_width(void) {
    return rgb_settings.rainbow_width;
}

int8_t rgb_rainbow_get_spin(void) {
    return rgb_settings.rainbow_spin_increment;
}

void rgb_rainbow_set_spin(int8_t width) {
    rgb_settings.rainbow_spin_increment = width;
}

uint16_t rgb_rainbow_get_update_time(void) {
    return rgb_settings.rainbow_update_time;
}

void rgb_rainbow_set_update_time(uint16_t width) {
    rgb_settings.rainbow_update_time = width;

    FuriHalRtcBootMode bm = furi_hal_rtc_get_boot_mode();
    if(bm == FuriHalRtcBootModeNormal && rgb_settings.rainbow_update_time) {
        if(rgb_state.timer == NULL) {
            rgb_state.timer =
                furi_timer_alloc(rgb_backlight_timer_callback, FuriTimerTypePeriodic, NULL);
        }
        furi_timer_restart(rgb_state.timer, rgb_settings.rainbow_update_time);
    }
}

uint8_t rgb_internal_pattern_count(void) {
    return COUNT_OF(internal_pattern);
}

const char* rgb_internal_pattern_text(uint8_t index_pattern) {
    furi_assert(index_pattern < COUNT_OF(internal_pattern));
    return internal_pattern[index_pattern].name;
}

void rgb_internal_set_pattern(uint8_t index_pattern) {
    if(index_pattern > (COUNT_OF(internal_pattern) - 1)) {
        index_pattern = COUNT_OF(internal_pattern) - 1;
    }
    rgb_settings.internal_pattern_index = index_pattern;
}

void rgb_internal_custom_get_color(uint8_t* red, uint8_t* green, uint8_t* blue) {
    *red = rgb_settings.internal_color[0];
    *green = rgb_settings.internal_color[1];
    *blue = rgb_settings.internal_color[2];
}

void rgb_internal_custom_set_color(uint8_t red, uint8_t green, uint8_t blue) {
    rgb_settings.internal_color[0] = red;
    rgb_settings.internal_color[1] = green;
    rgb_settings.internal_color[2] = blue;
}

void rgb_internal_set_brightness(float brightness) {
    rgb_settings.internal_brightness = brightness;
}

float rgb_internal_get_brightness() {
    return rgb_settings.internal_brightness;
}

void rgb_internal_set_mode(uint32_t mode) {
    if(mode >= InternalModeCount) {
        mode = InternalModeMatch;
    }

    rgb_settings.internal_mode = (InternalMode)mode;
}

bool rgb_backlight_connected(void) {
    for(int i = 0; i < LED_BACKLIGHT_COUNT; i++) {
        for(int rgb = 0; rgb < 3; rgb++) {
            if(rgb_settings.backlight_colors[i][rgb]) {
                return true;
            }
        }
    }
    return false;
}

static void rgb_internal_color(
    uint8_t led_number,
    uint8_t index_pattern,
    uint8_t* red,
    uint8_t* green,
    uint8_t* blue) {
    if(index_pattern == 0) {
        // Custom color
        *red = rgb_settings.internal_color[0];
        *green = rgb_settings.internal_color[1];
        *blue = rgb_settings.internal_color[2];
        return;
    }
    if(index_pattern > COUNT_OF(internal_pattern)) {
        index_pattern = COUNT_OF(internal_pattern);
    }
    InternalPattern pattern = internal_pattern[index_pattern];
    uint8_t color_index = pattern.index[led_number % pattern.length];
    if(color_index > COUNT_OF(rgb_colors)) {
        color_index = COUNT_OF(rgb_colors) - 1;
    }
    *red = rgb_colors[color_index].red;
    *green = rgb_colors[color_index].green;
    *blue = rgb_colors[color_index].blue;
}

void rgb_internal_set_hardware_version(uint8_t version) {
    rgb_settings.hardware_version = version;
}

uint8_t rgb_internal_get_hardware_version(void) {
    return rgb_settings.hardware_version;
}

// Map phyiscal LEDs (0..11) to logical LEDs (0..7)
uint8_t led_mapping_v1[] = {0, 2, 3, 4, 1, 3, 4, 5, 6, 7, 5, 6};
uint8_t led_mapping_v2[] = {0, 3, 1, 3, 4, 5, 6, 7, 5, 6};

/**
 * @brief Converts a physical internal LED number into a logical index.
 * 
 * @details Groups multiple LEDs together so that the ordering of
 * the lights makes sense in the physical world (instead of electrical
 * ordering).  Multiple LEDs can map to the same place in the physical
 * world.  This is helpful for LEDs that are near each other.
 * 
 * @attention This mapping may change after user testing.
 * 
 * @param hardware_version Hardware version number.
 * @param led_number Physical internal LED number (0..11)
 * @return uint8_t Logical index.
 */
static uint8_t mapped_internal_led(uint8_t hardware_version, uint8_t led_number) {
    uint8_t mapped_index = 0;

    if(hardware_version == 2) {
        if(led_number < COUNT_OF(led_mapping_v2)) {
            mapped_index = led_mapping_v2[led_number];
        }
    } else {
        if(led_number < COUNT_OF(led_mapping_v1)) {
            mapped_index = led_mapping_v1[led_number];
        }
    }

    return mapped_index;
}

// hue=0..359.     n=5 (red), n=3 (green), n=1 (blue)
static uint8_t rainbow(uint16_t hue, uint8_t n) {
    hue = hue % 360;
    float v1 = hue / 60.0f;
    v1 += n;
    while(v1 >= 6.0f) {
        v1 -= 6.0f;
    }
    if((4 - v1) < v1) {
        v1 = 4 - v1;
    }
    if(v1 > 1) {
        v1 = 1;
    }
    if(v1 < 0) {
        v1 = 0;
    }

    float s = 0.9;
    float b = 255.0;
    return b * (1 - s * v1);
}

void rgb_backlight_update(uint8_t brightness) {
    if(!rgb_settings.settings_loaded) {
        rgb_backlight_load_settings();
    }

    static uint32_t last_display_color[LED_BACKLIGHT_COUNT][3] = {0};
    static uint8_t last_internal_pattern_index = 255;
    static uint32_t last_internal_color[3] = {0};
    static float last_internal_brightness = 1.1f;

    uint8_t led_count = 0;

    if(rgb_state.last_display_brightness == brightness &&
       last_internal_pattern_index == rgb_settings.internal_pattern_index &&
       last_internal_color[0] == rgb_settings.internal_color[0] &&
       last_internal_color[1] == rgb_settings.internal_color[1] &&
       last_internal_color[2] == rgb_settings.internal_color[2] &&
       fabsf(last_internal_brightness - rgb_settings.internal_brightness) < 0.02f) {
        bool same_color = true;

        for(int i = 0; i < LED_BACKLIGHT_COUNT; i++) {
            for(int rgb = 0; rgb < 3; rgb++) {
                if(rgb_settings.backlight_colors[i][rgb] != last_display_color[i][rgb]) {
                    same_color = false;
                }
            }
        }

        if(same_color && rgb_settings.rainbow_width == 0) {
            return;
        }
    }

    rgb_state.last_display_brightness = brightness;
    for(int i = 0; i < LED_BACKLIGHT_COUNT; i++) {
        for(int rgb = 0; rgb < 3; rgb++) {
            last_display_color[i][rgb] = rgb_settings.backlight_colors[i][rgb];
        }
    }
    // Don't update last internal data yet, we need it later.

    if(rgb_backlight_connected()) {
        uint8_t led_index = SK6805_get_led_backlight_count() - 1;
        for(uint8_t i = 0; i < SK6805_get_led_backlight_count(); i++) {
            uint8_t red, green, blue;
            rgb_backlight_led_get_color(i, &red, &green, &blue);

            if(rgb_settings.backlight_mode != BacklightModeConstant) {
                uint8_t mapped_index = 0;
                if(rgb_settings.backlight_mode == BacklightModeTimedSingle) {
                    mapped_index = RGB_BACKLIGHT_RAINBOW_S0;
                } else if(rgb_settings.backlight_mode == BacklightModeTimedRainbow) {
                    mapped_index = (i == 0) ? RGB_BACKLIGHT_RAINBOW_R0 :
                                   (i == 1) ? RGB_BACKLIGHT_RAINBOW_R1 :
                                              RGB_BACKLIGHT_RAINBOW_R2;
                }

                uint8_t mult = rgb_settings.rainbow_width ? rgb_settings.rainbow_width / 8 : 0;
                red = rainbow(rgb_state.offset + (mapped_index * mult), 5);
                green = rainbow(rgb_state.offset + (mapped_index * mult), 3);
                blue = rainbow(rgb_state.offset + (mapped_index * mult), 1);
            }

            uint8_t r = red * (brightness / 255.0f);
            uint8_t g = green * (brightness / 255.0f);
            uint8_t b = blue * (brightness / 255.0f);
            SK6805_set_led_color(led_index--, r, g, b);
            led_count++;
        }
    }

    uint8_t internal_count_start_index =
        rgb_backlight_connected() ? SK6805_get_led_backlight_count() : 0;
    float internal_brightness = rgb_settings.internal_brightness;

    if(rgb_settings.rainbow_width == 0 && rgb_settings.internal_mode == InternalModeOn &&
       (last_internal_pattern_index == rgb_settings.internal_pattern_index) &&
       last_internal_color[0] == rgb_settings.internal_color[0] &&
       last_internal_color[1] == rgb_settings.internal_color[1] &&
       last_internal_color[2] == rgb_settings.internal_color[2] &&
       fabsf(last_internal_brightness - rgb_settings.internal_brightness) < 0.02f) {
        // Ignore
    } else {
        if((rgb_settings.internal_mode == InternalModeMatch) && (brightness == 0)) {
            internal_brightness = 0.0f;
        }

        uint8_t color_index = rgb_settings.internal_pattern_index;
        uint8_t mult = rgb_settings.rainbow_width / 8;

        for(uint8_t i = 0; i < SK6805_get_led_internal_count(); i++) {
            uint8_t red, green, blue;
            uint8_t mapped_index = mapped_internal_led(rgb_settings.hardware_version, i);
            rgb_internal_color(mapped_index, color_index, &red, &green, &blue);

#ifdef USE_DEBUG_LED_STRIP
            mapped_index = i;
#endif

            if(color_index == 1) {
                red = rainbow(rgb_state.offset + (RGB_BACKLIGHT_RAINBOW_S0 * mult), 5);
                green = rainbow(rgb_state.offset + (RGB_BACKLIGHT_RAINBOW_S0 * mult), 3);
                blue = rainbow(rgb_state.offset + (RGB_BACKLIGHT_RAINBOW_S0 * mult), 1);
            } else if(color_index == 2) {
                red = rainbow(rgb_state.offset + (mapped_index * mult), 5);
                green = rainbow(rgb_state.offset + (mapped_index * mult), 3);
                blue = rainbow(rgb_state.offset + (mapped_index * mult), 1);
            }

#ifdef USE_DEBUG_LED_STRIP
            if(mapped_index >= 8) {
                red = 0;
                green = 0;
                blue = 0;
            }
#endif

            uint8_t r = red * internal_brightness;
            uint8_t g = green * internal_brightness;
            uint8_t b = blue * internal_brightness;
            SK6805_set_led_color(i + internal_count_start_index, r, g, b);
            led_count++;
        }
    }

    last_internal_pattern_index = rgb_settings.internal_pattern_index;
    last_internal_brightness = rgb_settings.internal_brightness;
    last_internal_color[0] = rgb_settings.internal_color[0];
    last_internal_color[1] = rgb_settings.internal_color[1];
    last_internal_color[2] = rgb_settings.internal_color[2];

    SK6805_update(led_count);
}
