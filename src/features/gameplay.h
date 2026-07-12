#pragma once

#include "il2cpp_api.h"
#include "settings.h"

#include <atomic>
#include <mutex>

namespace hackmatch
{
struct Vector3
{
    float x;
    float y;
    float z;
};

struct MovementDiagnostics
{
    Vector3 intent{};
    Vector3 velocity{};
    float measured_speed = 0.0f;
    bool ads = false;
    bool sprinting = false;
};

class Gameplay
{
  public:
    bool aiming() const;
    bool movement_diagnostics(MovementDiagnostics& value) const;
    void before_player_update(il2cpp::Object* player);
    void after_player_update(il2cpp::Object* player);
    bool redirect_shot(Vector3 origin, Vector3& direction, float max_distance);
    il2cpp::Object* target_player() const;
    void tick();
    void restore();

  private:
    void restore_frame_overrides();
    void update_target();

    il2cpp::Object* movement_player_ = nullptr;
    Vector3 original_movement_{};
    Vector3 overridden_movement_{};
    bool movement_overridden_ = false;
    il2cpp::Object* fov_player_ = nullptr;
    il2cpp::Object* fov_camera_ = nullptr;
    float original_fov_ = 0.0f;
    float overridden_fov_ = 0.0f;
    bool fov_overridden_ = false;
    std::mutex frame_overrides_mutex_;
    std::atomic<il2cpp::Object*> target_player_{nullptr};
    std::atomic<il2cpp::Object*> locked_target_{nullptr};
    std::atomic<bool> locked_target_valid_{false};
    std::atomic<il2cpp::Object*> ready_local_{nullptr};
    MovementDiagnostics diagnostics_{};
    bool diagnostics_valid_ = false;
    mutable std::mutex diagnostics_mutex_;
};

Gameplay& gameplay();
} // namespace hackmatch
