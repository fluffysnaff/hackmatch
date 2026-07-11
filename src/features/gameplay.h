#pragma once

#include "il2cpp_api.h"
#include "settings.h"

#include <atomic>

namespace hackmatch {
struct Vector3 {
    float x;
    float y;
    float z;
};

class Gameplay {
public:
    bool aiming() const;
    void after_player_update(il2cpp::Object* player);
    bool redirect_shot(Vector3 origin, Vector3& direction, float max_distance);
    il2cpp::Object* target_player() const;
    void tick();
    void restore();

private:
    void update_target();

    il2cpp::Object* target_player_ = nullptr;
    std::atomic<il2cpp::Object*> locked_target_{nullptr};
    std::atomic<bool> locked_target_valid_{false};
    std::atomic<il2cpp::Object*> ready_local_{nullptr};
};

Gameplay& gameplay();
}
