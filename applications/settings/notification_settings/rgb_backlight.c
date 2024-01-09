/*
    RGB backlight FlipperZero driver
    Copyright (C) 2022-2023 Victor Nikitchuk (https://github.com/quen0n)

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
#include <furi_hal.h>
#include <storage/storage.h>

#define RGB_BACKLIGHT_SETTINGS_VERSION 5
#define RGB_BACKLIGHT_SETTINGS_FILE_NAME ".rgb_backlight.settings"
#define RGB_BACKLIGHT_SETTINGS_PATH EXT_PATH(RGB_BACKLIGHT_SETTINGS_FILE_NAME)

#define COLOR_COUNT (sizeof(colors) / sizeof(RGBBacklightColor))

#define TAG "RGB Backlight"

static RGBBacklightSettings rgb_settings = {
    .version = RGB_BACKLIGHT_SETTINGS_VERSION,
    .display_color_index = 0,
    .settings_is_loaded = false,
    .internal_color_index = 0,
    .internal_brightness = 0, // By default internal lights are off
};

static const RGBBacklightColor colors[] = {
    {"Orange", 255, 79, 0},
    {"Yellow", 255, 170, 0},
    {"Spring", 167, 255, 0},
    {"Lime", 0, 255, 0},
    {"Aqua", 0, 255, 127},
    {"Cyan", 0, 210, 210},
    {"Azure", 0, 127, 255},
    {"Blue", 0, 0, 255},
    {"Purple", 127, 0, 255},
    {"Magenta", 210, 0, 210},
    {"Pink", 255, 0, 127},
    {"Red", 255, 0, 0},
    {"White", 140, 140, 140},
    {"Off", 0, 0, 0}, // This must be the LAST color
};

uint8_t rgb_backlight_get_color_count(void) {
    return COLOR_COUNT;
}

const char* rgb_backlight_get_color_text(uint8_t index) {
    return colors[index].name;
}

void rgb_backlight_load_settings(void) {
    //Не загружать данные из внутренней памяти при загрузке в режиме DFU
    FuriHalRtcBootMode bm = furi_hal_rtc_get_boot_mode();
    if(bm == FuriHalRtcBootModeDfu) {
        rgb_settings.settings_is_loaded = true;
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

        // Previous version had first 3 bytes only.
        if((bytes_count != settings_size) && (bytes_count != 3)) {
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
            if(settings.display_color_index >= COLOR_COUNT) {
                settings.display_color_index = COLOR_COUNT - 1;
            }
            if(settings.internal_color_index >= COLOR_COUNT) {
                settings.internal_color_index = COLOR_COUNT - 1;
            }
            memcpy(&rgb_settings, &settings, bytes_count);
        }
    } else {
        FURI_LOG_E(TAG, "load failed, %s", storage_file_get_error_desc(file));
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    rgb_settings.settings_is_loaded = true;
};

void rgb_backlight_save_settings(void) {
    RGBBacklightSettings settings;
    File* file = storage_file_alloc(furi_record_open(RECORD_STORAGE));
    const size_t settings_size = sizeof(RGBBacklightSettings);

    FURI_LOG_I(TAG, "saving settings to \"%s\"", RGB_BACKLIGHT_SETTINGS_PATH);

    memcpy(&settings, &rgb_settings, settings_size);
    if(settings.display_color_index == COLOR_COUNT - 1) {
        settings.display_color_index = 255;
    }
    if(settings.internal_color_index == COLOR_COUNT - 1) {
        settings.internal_color_index = 255;
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
};

RGBBacklightSettings* rgb_backlight_get_settings(void) {
    if(!rgb_settings.settings_is_loaded) {
        rgb_backlight_load_settings();
    }
    return &rgb_settings;
}

bool rgb_backlight_has_color() {
    return (rgb_settings.display_color_index != rgb_backlight_get_color_count() - 1);
}

void rgb_backlight_set_color(uint8_t color_index) {
    if(color_index > (rgb_backlight_get_color_count() - 1)) color_index = 0;
    rgb_settings.display_color_index = color_index;
}

void rgb_internal_set_color(uint8_t color_index) {
    if(color_index > (rgb_backlight_get_color_count() - 1)) color_index = 0;
    rgb_settings.internal_color_index = color_index;
}

void rgb_internal_set_brightness(uint8_t brightness) {
    rgb_settings.internal_brightness = brightness;
}

void rgb_internal_set_mode(uint32_t mode) {
    if(mode > 255) {
        mode = 0;
    }

    rgb_settings.internal_mode = (uint8_t)mode;
}

void rgb_backlight_update(uint8_t brightness) {
    if(!rgb_settings.settings_is_loaded) {
        rgb_backlight_load_settings();
    }

    static uint8_t last_display_color_index = 255;
    static uint8_t last_display_brightness = 123;
    static uint8_t last_internal_color_index = 255;
    static uint8_t last_internal_brightness = 123;

    uint8_t led_count = 0;

    if(last_display_brightness == brightness &&
       last_display_color_index == rgb_settings.display_color_index &&
       last_internal_color_index == rgb_settings.internal_color_index &&
       last_internal_brightness == rgb_settings.internal_brightness)
        return;

    last_display_brightness = brightness;
    last_display_color_index = rgb_settings.display_color_index;

    if(rgb_backlight_has_color()) {
        for(uint8_t i = 0; i < SK6805_get_led_backlight_count(); i++) {
            uint8_t r = colors[rgb_settings.display_color_index].red * (brightness / 255.0f);
            uint8_t g = colors[rgb_settings.display_color_index].green * (brightness / 255.0f);
            uint8_t b = colors[rgb_settings.display_color_index].blue * (brightness / 255.0f);

            SK6805_set_led_color(i, r, g, b);
            led_count++;
        }
    }

    uint8_t internal_count_start_index =
        rgb_backlight_has_color() ? SK6805_get_led_backlight_count() : 0;
    uint8_t internal_brightness = rgb_settings.internal_brightness;

    if(rgb_settings.internal_mode == RGB_BACKLIGHT_INTERNAL_MODE_ON &&
       (last_internal_color_index == rgb_settings.internal_color_index) &&
       (last_internal_brightness == rgb_settings.internal_brightness)) {
        // Ignore
    } else {
        if((rgb_settings.internal_mode == RGB_BACKLIGHT_INTERNAL_MODE_AUTO) && (brightness == 0)) {
            internal_brightness = 0;
        }
        for(uint8_t i = 0; i < SK6805_get_led_internal_count(); i++) {
            uint8_t color_index = rgb_settings.internal_color_index;
            uint8_t r = colors[color_index].red * (internal_brightness / 255.0f);
            uint8_t g = colors[color_index].green * (internal_brightness / 255.0f);
            uint8_t b = colors[color_index].blue * (internal_brightness / 255.0f);

            SK6805_set_led_color(i + internal_count_start_index, r, g, b);
            led_count++;
        }
    }

    last_internal_color_index = rgb_settings.internal_color_index;
    last_internal_brightness = rgb_settings.internal_brightness;

    SK6805_update(led_count);
}