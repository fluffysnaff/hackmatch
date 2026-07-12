#pragma once

#include <algorithm>
#include <cmath>

namespace hackmatch::feature_limits
{
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

inline bool scale_horizontal_movement(float& x, float& z, float target_speed)
{
    const float magnitude_sq = x * x + z * z;
    if (magnitude_sq <= 0.0001f)
    {
        return false;
    }
    const float scale = speed(target_speed) / std::sqrt(magnitude_sq);
    x *= scale;
    z *= scale;
    return true;
}

inline float aim_fov_radius(float viewport_height, float aim_degrees, float camera_degrees)
{
    constexpr float pi = 3.14159265358979323846f;
    if (!std::isfinite(viewport_height) || !std::isfinite(aim_degrees) || !std::isfinite(camera_degrees) ||
        viewport_height <= 0.0f || camera_degrees <= 0.0f || camera_degrees >= 180.0f)
    {
        return 0.0f;
    }
    return viewport_height * 0.5f * std::tan(aim_fov(aim_degrees) * 0.5f * pi / 180.0f) /
           std::tan(camera_degrees * 0.5f * pi / 180.0f);
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
} // namespace hackmatch::feature_limits
