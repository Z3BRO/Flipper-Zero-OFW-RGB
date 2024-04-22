#include <furi.h>
#include <notification/notification_app.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/variable_item_list.h>
#include <gui/view_dispatcher.h>
#include <lib/toolbox/value_index.h>
#include <applications/settings/notification_settings/rgb_backlight.h>

typedef struct {
    NotificationApp* notification;
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    VariableItemList* variable_item_list;
    ByteInput* byte_input;
    uint8_t bytes[3];
    uint8_t byte_input_selected_index;
    VariableItem* items[4];
} RgbSettingsApp;

typedef struct {
    bool led2_matches_led1;
    bool led3_matches_led1;
} RGBBacklightState;

static RGBBacklightState rgb_state = {
    .led2_matches_led1 = false,
    .led3_matches_led1 = false,
};

typedef enum {
    RGBSettingsViewList,
    RGBSettingsViewInput,
} RGBSettingsViews;

#define BRIGHTNESS_COUNT 21
static const char* const brightness_text[BRIGHTNESS_COUNT] = {
    "0%",  "5%",  "10%", "15%", "20%", "25%", "30%", "35%", "40%", "45%",  "50%",
    "55%", "60%", "65%", "70%", "75%", "80%", "85%", "90%", "95%", "100%",
};
static const float brightness_value[BRIGHTNESS_COUNT] = {
    0.00f, 0.05f, 0.10f, 0.15f, 0.20f, 0.25f, 0.30f, 0.35f, 0.40f, 0.45f, 0.50f,
    0.55f, 0.60f, 0.65f, 0.70f, 0.75f, 0.80f, 0.85f, 0.90f, 0.95f, 1.00f,
};
static const char* const internal_mode_text[InternalModeCount] = {
    [InternalModeMatch] = "Auto",
    [InternalModeOn] = "On",
};

static const uint32_t internal_mode_value[InternalModeCount] = {
    [InternalModeMatch] = InternalModeMatch,
    [InternalModeOn] = InternalModeOn,
};

static void backlight_brightness_changed(VariableItem* item) {
    RgbSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, brightness_text[index]);
    app->notification->settings.display_brightness = brightness_value[index];
    notification_message(app->notification, &sequence_display_backlight_on);
}

static void internal_brightness_changed(VariableItem* item) {
    RgbSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    rgb_internal_set_brightness(brightness_value[index]);
    variable_item_set_current_value_text(item, brightness_text[index]);
    notification_message(app->notification, &sequence_display_backlight_on);
}

static void backlight_color_changed_1(VariableItem* item) {
    RgbSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    uint8_t red, green, blue;
    rgb_backlight_color_value(index, &red, &green, &blue);
    rgb_backlight_led_set_color(0, red, green, blue);
    if(rgb_state.led2_matches_led1) {
        rgb_backlight_led_set_color(1, red, green, blue);
    }
    if(rgb_state.led3_matches_led1) {
        rgb_backlight_led_set_color(2, red, green, blue);
    }
    variable_item_set_current_value_text(item, rgb_backlight_color_text(index));
    notification_message(app->notification, &sequence_display_backlight_on);
}

static void backlight_color_changed_2(VariableItem* item) {
    RgbSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    uint8_t red, green, blue;
    if(index-- == 0) {
        rgb_backlight_led_get_color(0, &red, &green, &blue);
        rgb_backlight_led_set_color(1, red, green, blue);
        rgb_state.led2_matches_led1 = true;
        variable_item_set_current_value_text(item, "RGB 1");
        notification_message(app->notification, &sequence_display_backlight_on);
        return;
    }

    rgb_backlight_color_value(index, &red, &green, &blue);
    rgb_backlight_led_set_color(1, red, green, blue);
    rgb_state.led2_matches_led1 = false;
    variable_item_set_current_value_text(item, rgb_backlight_color_text(index));
    notification_message(app->notification, &sequence_display_backlight_on);
}

static void backlight_color_changed_3(VariableItem* item) {
    RgbSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    uint8_t red, green, blue;
    if(index-- == 0) {
        rgb_backlight_led_get_color(0, &red, &green, &blue);
        rgb_backlight_led_set_color(2, red, green, blue);
        rgb_state.led3_matches_led1 = true;
        variable_item_set_current_value_text(item, "RGB 1");
        notification_message(app->notification, &sequence_display_backlight_on);
        return;
    }

    rgb_backlight_color_value(index, &red, &green, &blue);
    rgb_backlight_led_set_color(2, red, green, blue);
    rgb_state.led3_matches_led1 = false;
    variable_item_set_current_value_text(item, rgb_backlight_color_text(index));
    notification_message(app->notification, &sequence_display_backlight_on);
}

static void internal_pattern_changed(VariableItem* item) {
    RgbSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    rgb_internal_set_pattern(index);
    variable_item_set_current_value_text(item, rgb_internal_pattern_text(index));
    notification_message(app->notification, &sequence_display_backlight_on);
}

static void internal_mode_changed(VariableItem* item) {
    RgbSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    rgb_internal_set_mode(internal_mode_value[index]);
    variable_item_set_current_value_text(item, internal_mode_text[index]);
    notification_message(app->notification, &sequence_display_backlight_on);
}

static const NotificationMessage led_apply_message = {
    .type = NotificationMessageTypeLedBrightnessSettingApply,
};
const NotificationSequence led_apply_sequence = {
    &led_apply_message,
    NULL,
};
static void led_changed(VariableItem* item) {
    RgbSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, brightness_text[index]);
    app->notification->settings.led_brightness = brightness_value[index];
    notification_message(app->notification, &led_apply_sequence);
    notification_internal_message(app->notification, &led_apply_sequence);
    notification_message(app->notification, &sequence_blink_white_100);
}

static uint32_t rgb_settings_app_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

static uint32_t rgb_settings_app_show_list(void* context) {
    UNUSED(context);
    return RGBSettingsViewList;
}

static void rgb_settings_byte_input_result(void* context) {
    RgbSettingsApp* app = context;
    if(app->byte_input_selected_index <= 2) {
        uint8_t index = app->byte_input_selected_index;
        variable_item_set_current_value_index(app->items[index], (index == 0) ? 0 : 1);
        variable_item_set_current_value_text(app->items[index], "Custom");
        if(index == 1) {
            rgb_state.led2_matches_led1 = false;
        } else if(index == 2) {
            rgb_state.led3_matches_led1 = false;
        }

        rgb_backlight_led_set_color(
            app->byte_input_selected_index, app->bytes[0], app->bytes[1], app->bytes[2]);

        if(index == 0) {
            if(rgb_state.led2_matches_led1) {
                rgb_backlight_led_set_color(1, app->bytes[0], app->bytes[1], app->bytes[2]);
            }
            if(rgb_state.led3_matches_led1) {
                rgb_backlight_led_set_color(2, app->bytes[0], app->bytes[1], app->bytes[2]);
            }
        }
        notification_message(app->notification, &sequence_display_backlight_on);
    } else {
        rgb_internal_custom_set_color(app->bytes[0], app->bytes[1], app->bytes[2]);
        rgb_internal_set_pattern(0); // Custom
        float brightness = rgb_internal_get_brightness();
        rgb_internal_set_brightness(brightness + 0.0001f);
        notification_message(app->notification, &sequence_display_backlight_on);
        variable_item_set_current_value_index(app->items[3], 0);
        variable_item_set_current_value_text(app->items[3], rgb_internal_pattern_text(0));
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, RGBSettingsViewList);
}

static void rgb_settings_list_enter(void* context, uint32_t index) {
    // First three settings (0-2) are RGB colors.
    // Setting 4 is internal pattern.
    if(index > 2 && index != 4) {
        return;
    }

    RgbSettingsApp* app = context;
    app->byte_input_selected_index = index;

    if(index <= 2) {
        rgb_backlight_led_get_color(index, &app->bytes[0], &app->bytes[1], &app->bytes[2]);
    } else {
        rgb_internal_custom_get_color(&app->bytes[0], &app->bytes[1], &app->bytes[2]);
    }

    byte_input_set_result_callback(
        app->byte_input, rgb_settings_byte_input_result, NULL, app, app->bytes, 3);

    view_dispatcher_switch_to_view(app->view_dispatcher, RGBSettingsViewInput);
}

static RgbSettingsApp* alloc_settings() {
    RgbSettingsApp* app = malloc(sizeof(RgbSettingsApp));
    app->notification = furi_record_open(RECORD_NOTIFICATION);
    app->gui = furi_record_open(RECORD_GUI);

    app->byte_input = byte_input_alloc();
    byte_input_set_header_text(app->byte_input, "Enter RGB color");
    view_set_previous_callback(byte_input_get_view(app->byte_input), rgb_settings_app_show_list);

    app->variable_item_list = variable_item_list_alloc();
    View* view = variable_item_list_get_view(app->variable_item_list);
    variable_item_list_set_enter_callback(app->variable_item_list, rgb_settings_list_enter, app);
    view_set_previous_callback(view, rgb_settings_app_exit);

    VariableItem* item;
    uint8_t value_index;
    uint8_t value_index_rgb1;

    app->items[0] = variable_item_list_add(
        app->variable_item_list,
        "LCD RGB 1",
        rgb_backlight_color_count(),
        backlight_color_changed_1,
        app);
    value_index_rgb1 = rgb_backlight_find_index(0);
    variable_item_set_current_value_index(app->items[0], value_index_rgb1);
    variable_item_set_current_value_text(
        app->items[0], rgb_backlight_color_text(value_index_rgb1));

    app->items[1] = variable_item_list_add(
        app->variable_item_list,
        "LCD RGB 2",
        rgb_backlight_color_count() + 1,
        backlight_color_changed_2,
        app);
    value_index = rgb_backlight_find_index(1);
    variable_item_set_current_value_index(app->items[1], value_index);
    if(value_index - 1 == value_index_rgb1) {
        variable_item_set_current_value_index(app->items[1], 0);
        variable_item_set_current_value_text(app->items[1], "RGB 1");
        rgb_state.led2_matches_led1 = true;
    } else {
        variable_item_set_current_value_text(
            app->items[1], rgb_backlight_color_text(value_index - 1));
        rgb_state.led2_matches_led1 = false;
    }

    app->items[2] = variable_item_list_add(
        app->variable_item_list,
        "LCD RGB 3",
        rgb_backlight_color_count() + 1,
        backlight_color_changed_3,
        app);
    value_index = rgb_backlight_find_index(2);
    variable_item_set_current_value_index(app->items[2], value_index);
    if(value_index - 1 == value_index_rgb1) {
        variable_item_set_current_value_index(app->items[2], 0);
        variable_item_set_current_value_text(app->items[2], "RGB 1");
        rgb_state.led3_matches_led1 = true;
    } else {
        variable_item_set_current_value_text(
            app->items[2], rgb_backlight_color_text(value_index - 1));
        rgb_state.led3_matches_led1 = false;
    }

    item = variable_item_list_add(
        app->variable_item_list,
        "LCD Brightness",
        BRIGHTNESS_COUNT,
        backlight_brightness_changed,
        app);
    value_index = value_index_float(
        app->notification->settings.display_brightness, brightness_value, BRIGHTNESS_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, brightness_text[value_index]);

    app->items[3] = variable_item_list_add(
        app->variable_item_list,
        "Internal Pattern",
        rgb_internal_pattern_count(),
        internal_pattern_changed,
        app);
    value_index = rgb_backlight_get_settings()->internal_pattern_index;
    variable_item_set_current_value_index(app->items[3], value_index);
    variable_item_set_current_value_text(app->items[3], rgb_internal_pattern_text(value_index));

    item = variable_item_list_add(
        app->variable_item_list,
        "Internal Bright",
        BRIGHTNESS_COUNT,
        internal_brightness_changed,
        app);
    value_index = value_index_float(
        rgb_backlight_get_settings()->internal_brightness, brightness_value, BRIGHTNESS_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, brightness_text[value_index]);

    item = variable_item_list_add(
        app->variable_item_list, "Internal Mode", InternalModeCount, internal_mode_changed, app);
    value_index = value_index_uint32(
        rgb_backlight_get_settings()->internal_mode, internal_mode_value, InternalModeCount);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, internal_mode_text[value_index]);

    item = variable_item_list_add(
        app->variable_item_list, "LED Brightness", BRIGHTNESS_COUNT, led_changed, app);
    value_index = value_index_float(
        app->notification->settings.led_brightness, brightness_value, BRIGHTNESS_COUNT);
    variable_item_set_current_value_index(item, value_index);
    variable_item_set_current_value_text(item, brightness_text[value_index]);

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_add_view(app->view_dispatcher, RGBSettingsViewList, view);
    view_dispatcher_add_view(
        app->view_dispatcher, RGBSettingsViewInput, byte_input_get_view(app->byte_input));
    view_dispatcher_switch_to_view(app->view_dispatcher, RGBSettingsViewList);

    return app;
}

static void free_settings(RgbSettingsApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, RGBSettingsViewInput);
    view_dispatcher_remove_view(app->view_dispatcher, RGBSettingsViewList);
    variable_item_list_free(app->variable_item_list);
    byte_input_free(app->byte_input);
    view_dispatcher_free(app->view_dispatcher);

    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    free(app);
}

int32_t rgb_settings_app(void* p) {
    UNUSED(p);
    RgbSettingsApp* app = alloc_settings();
    view_dispatcher_run(app->view_dispatcher);
    notification_message_save_settings(app->notification);
    free_settings(app);
    return 0;
}
