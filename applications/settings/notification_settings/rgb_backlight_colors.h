#pragma once
#include "rgb_backlight.h"

enum ColorName {
    ColorNameCustom,
    ColorNameOrange,
    ColorNameYellow,
    ColorNameSpring,
    ColorNameLime,
    ColorNameAqua,
    ColorNameCyan,
    ColorNameAzure,
    ColorNameBlue,
    ColorNamePurple,
    ColorNameMagenta,
    ColorNamePink,
    ColorNameRed,
    ColorNameWhite,
    ColorNameOff, // This must be the last color.
    ColorCount, // This must be the last entry.
};

static const RGBColor rgb_colors[ColorCount] = {
    [ColorNameCustom] = {"Custom", .red = 255, .green = 79, .blue = 0},
    [ColorNameOrange] = {"Orange", .red = 255, .green = 79, .blue = 0},
    [ColorNameYellow] = {"Yellow", .red = 255, .green = 170, .blue = 0},
    [ColorNameSpring] = {"Spring", .red = 167, .green = 255, .blue = 0},
    [ColorNameLime] = {"Lime", .red = 0, .green = 255, .blue = 0},
    [ColorNameAqua] = {"Aqua", .red = 0, .green = 255, .blue = 127},
    [ColorNameCyan] = {"Cyan", .red = 0, .green = 210, .blue = 210},
    [ColorNameAzure] = {"Azure", .red = 0, .green = 127, .blue = 255},
    [ColorNameBlue] = {"Blue", .red = 0, .green = 0, .blue = 255},
    [ColorNamePurple] = {"Purple", .red = 127, .green = 0, .blue = 255},
    [ColorNameMagenta] = {"Magenta", .red = 210, .green = 0, .blue = 210},
    [ColorNamePink] = {"Pink", .red = 255, .green = 0, .blue = 127},
    [ColorNameRed] = {"Red", .red = 255, .green = 0, .blue = 0},
    [ColorNameWhite] = {"White", .red = 140, .green = 140, .blue = 140},
    [ColorNameOff] = {"Off", 0, 0, 0}, // This must be the LAST color
};

static const InternalPattern internal_pattern[] = {
    {"Custom", .length = 1, {ColorNameCustom}},
    {"Slide", .length = 2, {ColorNameCustom, ColorNameCustom}},
    {"Rainbow", .length = 3, {ColorNameCustom, ColorNameCustom, ColorNameCustom}},
    {"Orange", .length = 1, {ColorNameOrange}},
    {"Yellow", .length = 1, {ColorNameYellow}},
    {"Spring", .length = 1, {ColorNameSpring}},
    {"Lime", .length = 1, {ColorNameLime}},
    {"Aqua", .length = 1, {ColorNameAqua}},
    {"Cyan", .length = 1, {ColorNameCyan}},
    {"Azure", .length = 1, {ColorNameAzure}},
    {"Blue", .length = 1, {ColorNameBlue}},
    {"Purple", .length = 1, {ColorNamePurple}},
    {"Magenta", .length = 1, {ColorNameMagenta}},
    {"Pink", .length = 1, {ColorNamePink}},
    {"Red", .length = 1, {ColorNameRed}},
    {"White", .length = 1, {ColorNameWhite}},
    {"OYOY", .length = 2, {ColorNameOrange, ColorNameYellow}},
    {"BluPuPi", .length = 3, {ColorNameBlue, ColorNamePurple, ColorNamePink}},
    {"RGBW", .length = 4, {ColorNameRed, ColorNameLime, ColorNameBlue, ColorNameWhite}},
    {"Off", .length = 1, {ColorNameOff}}, // This must be the LAST pattern
};