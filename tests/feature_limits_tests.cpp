#include "feature_limits.h"

#include <cassert>

int main()
{
    using namespace hackmatch::feature_limits;

    assert(aim_fov(-10.0f) == 1.0f);
    assert(aim_fov(25.0f) == 25.0f);
    assert(aim_fov(300.0f) == 180.0f);
    assert(camera_fov(20.0f) == 40.0f);
    assert(camera_fov(90.0f) == 90.0f);
    assert(camera_fov(200.0f) == 140.0f);
    assert(gravity(-100.0f) == -50.0f);
    assert(gravity(75.0f) == 50.0f);
    assert(speed(1.0f) == 5.0f);
    assert(speed(120.0f) == 80.0f);
    assert(unit(-1.0f) == 0.0f);
    assert(unit(2.0f) == 1.0f);
    assert(esp_thickness(0.1f) == 0.5f);
    assert(esp_thickness(8.0f) == 4.0f);
    assert(esp_text_scale(0.1f) == 0.75f);
    assert(esp_text_scale(3.0f) == 1.75f);
    assert(esp_distance(-10.0f) == 0.0f);
    assert(esp_distance(6000.0f) == 5000.0f);
}
