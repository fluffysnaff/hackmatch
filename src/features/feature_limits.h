#pragma once

#include <algorithm>

namespace hackmatch::feature_limits {
constexpr float aim_fov(float value)
{
    return std::clamp(value, 1.0f, 180.0f);
}

constexpr float camera_fov(float value)
{
    return std::clamp(value, 40.0f, 140.0f);
}

constexpr float gravity(float value)
{
    return std::clamp(value, -50.0f, 50.0f);
}

constexpr float speed(float value)
{
    return std::clamp(value, 5.0f, 80.0f);
}

constexpr float reload_time(float value)
{
    return std::clamp(value, 0.0f, 2.0f);
}

constexpr float unit(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

constexpr float esp_thickness(float value)
{
    return std::clamp(value, 0.5f, 4.0f);
}

constexpr float esp_text_scale(float value)
{
    return std::clamp(value, 0.75f, 1.75f);
}

constexpr float esp_distance(float value)
{
    return std::clamp(value, 0.0f, 5000.0f);
}
}
