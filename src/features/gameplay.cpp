#include "gameplay.h"

#include "build_info.h"
#include "esp_players.h"
#include "feature_limits.h"
#include "game.h"
#include "game_offsets.h"
#include "gameplay_items.h"
#include "il2cpp_api.h"

#include <windows.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace hackmatch
{
namespace
{
struct AimTarget
{
    il2cpp::Object* player = nullptr;
    Vector3 position{};
    float dot = -1.0f;
    float distance_sq = 0.0f;
    bool found = false;
};

struct TargetProbe
{
    bool attempted = false;
    bool detected = false;
};

struct ManagedString : il2cpp::Object
{
    int length;
    wchar_t chars[1];
};

struct Vector2
{
    float x;
    float y;
};

struct RaycastHit
{
    Vector3 point;
    Vector3 normal;
    std::uint32_t face_id;
    float distance;
    Vector2 uv;
    int collider;
};

struct AimHitDistances
{
    float wall = 0.0f;
    float player = 0.0f;
    bool has_wall = false;
    bool has_player = false;
    float wall_distances[16]{};
    int wall_count = 0;
};

struct StatDictionaryEntry
{
    int hash_code;
    int next;
    il2cpp::Object* key;
    il2cpp::Object* value;
};

struct StatDictionary : il2cpp::Object
{
    il2cpp::Array* buckets;
    il2cpp::Array* entries;
    int count;
    int version;
    int free_list;
    int free_count;
};

struct StatPatch
{
    il2cpp::Object* stat = nullptr;
    float modifier = 0.0f;
    float offset = 0.0f;
    bool used = false;
};

constexpr int max_stat_patches = 64;

bool raw_bindings_compatible()
{
    return build_compatibility().status == BuildStatus::Compatible;
}

bool any_feature_enabled(const AppSettings& config)
{
    const auto& weapons = config.weapons;
    const auto& movement = config.movement;
    const auto& player = config.player;
    return weapons.no_spread || weapons.infinite_ammo || weapons.instant_reload || weapons.no_camera_shake ||
           config.aim.enabled || weapons.rapid_fire || player.custom_fov || movement.auto_sprint ||
           movement.no_gravity || movement.custom_gravity || movement.high_speed || player.disable_spawn_protection ||
           player.movement_diagnostics;
}

bool any_movement_feature_enabled(const MovementSettings& movement)
{
    return movement.no_gravity || movement.custom_gravity;
}

float aim_min_dot(const AimSettings& aim)
{
    constexpr float pi = 3.14159265358979323846f;
    const float fov = aim.ignore_fov ? 180.0f : feature_limits::aim_fov(aim.fov);
    return fov >= 179.0f ? -1.0f : std::cos(fov * 0.5f * pi / 180.0f);
}

template <class T> bool boxed_value(il2cpp::Object* object, T& out)
{
    if (!object)
    {
        return false;
    }

    out = *reinterpret_cast<T*>(reinterpret_cast<char*>(object) + sizeof(il2cpp::Object));
    return true;
}

float distance_sq(const Vector3& a, const Vector3& b)
{
    const float x = a.x - b.x;
    const float y = a.y - b.y;
    const float z = a.z - b.z;
    return x * x + y * y + z * z;
}

bool unity_alive(il2cpp::Object* object)
{
    static MethodInfo* object_equals =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Object", "op_Equality", 2);
    if (!object || !object_equals)
    {
        return false;
    }

    void* args[] = {object, nullptr};
    il2cpp::Object* exception = nullptr;
    bool is_null = true;
    return boxed_value(il2cpp::runtime_invoke(object_equals, nullptr, args, &exception), is_null) && !exception &&
           !is_null;
}

bool spawn_protected(il2cpp::Object* player)
{
    if (player && raw_bindings_compatible() &&
        *reinterpret_cast<bool*>(reinterpret_cast<char*>(player) + game_offsets::fields::player_shield_state))
    {
        return true;
    }
    static il2cpp::FieldInfo* shield_object = il2cpp::field(game().player_controller(), "shieldObject");
    static MethodInfo* active_in_hierarchy =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "GameObject", "get_activeInHierarchy", 0);
    il2cpp::Object* shield = nullptr;
    if (!player || !shield_object || !active_in_hierarchy || !il2cpp::read_field(player, shield_object, &shield) ||
        !unity_alive(shield))
    {
        return false;
    }

    il2cpp::Object* exception = nullptr;
    bool active = false;
    return boxed_value(il2cpp::runtime_invoke(active_in_hierarchy, shield, nullptr, &exception), active) &&
           !exception && active;
}

bool position_of(il2cpp::Object* object, Vector3& out)
{
    static MethodInfo* get_transform =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "get_transform", 0);
    static MethodInfo* get_position =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Transform", "get_position", 0);
    if (!get_transform || !get_position || !unity_alive(object))
    {
        return false;
    }

    il2cpp::Object* exception = nullptr;
    il2cpp::Object* transform = il2cpp::runtime_invoke(get_transform, object, nullptr, &exception);
    if (exception || !unity_alive(transform))
    {
        return false;
    }

    exception = nullptr;
    return boxed_value(il2cpp::runtime_invoke(get_position, transform, nullptr, &exception), out) && !exception;
}

il2cpp::Object* component(il2cpp::Object* object, il2cpp::Class* component_class)
{
    static MethodInfo* get_component =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "GetComponent", 1);
    il2cpp::Object* type = il2cpp::type_object(component_class);
    if (!object || !get_component || !type)
    {
        return nullptr;
    }

    void* args[] = {type};
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* found = il2cpp::runtime_invoke(get_component, object, args, &exception);
    return exception ? nullptr : found;
}

void choose_target(AimTarget& best, il2cpp::Object* player, const Vector3& camera_position,
                   const Vector3& camera_forward, float min_dot, Vector3 target_position)
{
    const float distance = distance_sq(camera_position, target_position);
    if (distance < 0.0001f)
    {
        return;
    }

    Vector3 direction{target_position.x - camera_position.x, target_position.y - camera_position.y,
                      target_position.z - camera_position.z};
    const float inv_length = 1.0f / std::sqrt(distance);
    const float dot =
        (direction.x * camera_forward.x + direction.y * camera_forward.y + direction.z * camera_forward.z) * inv_length;
    if (dot < min_dot)
    {
        return;
    }

    if (!best.found || dot > best.dot + 0.002f || (dot + 0.002f >= best.dot && distance < best.distance_sq))
    {
        best.player = player;
        best.position = target_position;
        best.dot = dot;
        best.distance_sq = distance;
        best.found = true;
    }
}

bool add_wall_distance(AimHitDistances& hits, float distance)
{
    for (int i = 0; i < hits.wall_count; ++i)
    {
        if (std::fabs(hits.wall_distances[i] - distance) < 0.35f)
        {
            return false;
        }
    }
    if (hits.wall_count < static_cast<int>(sizeof(hits.wall_distances) / sizeof(hits.wall_distances[0])))
    {
        hits.wall_distances[hits.wall_count++] = distance;
    }
    return true;
}

il2cpp::Object* raycast_hit_collider(RaycastHit& hit)
{
    static MethodInfo* find_object =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Object", "FindObjectFromInstanceID", 1);
    if (!find_object || hit.collider == 0)
    {
        return nullptr;
    }

    void* args[] = {&hit.collider};
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* collider = il2cpp::runtime_invoke(find_object, nullptr, args, &exception);
    return exception ? nullptr : collider;
}

il2cpp::Object* component_player(il2cpp::Object* component)
{
    static MethodInfo* get_component =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "GetComponent", 1);
    static MethodInfo* get_component_in_parent =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "GetComponentInParent", 1);
    static il2cpp::Object* player_type = il2cpp::type_object(game().player_controller());
    if (!component || !player_type)
    {
        return nullptr;
    }

    void* args[] = {player_type};
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* player =
        get_component ? il2cpp::runtime_invoke(get_component, component, args, &exception) : nullptr;
    if (!exception && player)
    {
        return player;
    }

    exception = nullptr;
    player = get_component_in_parent ? il2cpp::runtime_invoke(get_component_in_parent, component, args, &exception)
                                     : nullptr;
    return exception ? nullptr : player;
}

il2cpp::Object* collider_player(il2cpp::Object* collider)
{
    if (!collider)
    {
        return nullptr;
    }

    il2cpp::Object* player = component_player(collider);
    if (player)
    {
        return player;
    }

    static MethodInfo* get_attached_rigidbody =
        il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Collider", "get_attachedRigidbody", 0);
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* rigidbody = get_attached_rigidbody
                                    ? il2cpp::runtime_invoke(get_attached_rigidbody, collider, nullptr, &exception)
                                    : nullptr;
    return exception ? nullptr : component_player(rigidbody);
}

bool walls_before_player(const Vector3& origin, const Vector3& direction, float max_distance, il2cpp::Object* local,
                         il2cpp::Object* target, int& walls)
{
    static MethodInfo* raycast_all =
        il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Physics", "RaycastAll", 5);
    if (!raycast_all || !target)
    {
        return false;
    }

    Vector3 ray_origin = origin;
    Vector3 ray_direction = direction;
    float distance = max_distance;
    int layer_mask = -1;
    int query_trigger_interaction = 2;
    void* args[] = {&ray_origin, &ray_direction, &distance, &layer_mask, &query_trigger_interaction};
    il2cpp::Object* exception = nullptr;
    il2cpp::Array* hits =
        reinterpret_cast<il2cpp::Array*>(il2cpp::runtime_invoke(raycast_all, nullptr, args, &exception));
    if (exception || !hits || hits->size() == 0)
    {
        return false;
    }

    AimHitDistances distances{};
    RaycastHit* data = reinterpret_cast<RaycastHit*>(reinterpret_cast<char*>(hits) + offsetof(il2cpp::Array, vector));
    for (std::uintptr_t i = 0; i < hits->size(); ++i)
    {
        RaycastHit& hit = data[i];
        if (hit.distance < 0.01f || hit.distance > max_distance)
        {
            continue;
        }

        il2cpp::Object* collider = raycast_hit_collider(hit);
        il2cpp::Object* player = collider_player(collider);
        if (player == local)
        {
            continue;
        }
        if (player == target)
        {
            if (!distances.has_player || hit.distance < distances.player)
            {
                distances.player = hit.distance;
                distances.has_player = true;
            }
            continue;
        }
        if (!player)
        {
            add_wall_distance(distances, hit.distance);
        }
    }
    if (!distances.has_player)
    {
        return false;
    }

    walls = 0;
    for (int i = 0; i < distances.wall_count; ++i)
    {
        if (distances.wall_distances[i] < distances.player)
        {
            ++walls;
        }
    }
    return true;
}

void choose_allowed_target(AimTarget& best, il2cpp::Object* local, il2cpp::Object* player, const Vector3& origin,
                           const Vector3& camera_forward, float min_dot, Vector3 target_position, float max_distance,
                           int max_walls, TargetProbe* probe)
{
    const float distance = distance_sq(origin, target_position);
    if (distance < 0.0001f)
    {
        return;
    }

    Vector3 direction{target_position.x - origin.x, target_position.y - origin.y, target_position.z - origin.z};
    const float length = std::sqrt(distance);
    if (length > max_distance)
    {
        return;
    }
    direction.x /= length;
    direction.y /= length;
    direction.z /= length;

    const float dot = direction.x * camera_forward.x + direction.y * camera_forward.y + direction.z * camera_forward.z;
    if (dot < min_dot)
    {
        return;
    }

    if (probe)
        probe->attempted = true;
    int walls = 0;
    if (!walls_before_player(origin, direction, length + 2.0f, local, player, walls))
    {
        return;
    }
    if (probe)
        probe->detected = true;
    if (walls > max_walls)
    {
        return;
    }
    choose_target(best, player, origin, camera_forward, min_dot, target_position);
}

void choose_shot_targets(AimTarget& best, const Vector3& origin, const Vector3& camera_forward, float min_dot,
                         float max_distance, int max_walls, il2cpp::Object* locked_target, TargetProbe* probe)
{
    static il2cpp::FieldInfo* local_instance = il2cpp::field(game().player_controller(), "LocalInstance");
    il2cpp::Object* local = nullptr;
    il2cpp::Array* players = il2cpp::objects_of_type(game().player_controller());
    if (!players || !local_instance || !il2cpp::read_static_field(local_instance, &local))
    {
        return;
    }

    const int total = static_cast<int>(players->size());
    bool locked_target_seen = !locked_target;
    for (int i = 0; i < total; ++i)
    {
        il2cpp::Object* player = players->at(i);
        if (player == locked_target)
            locked_target_seen = true;
        Vector3 position{};
        if (player == local || (locked_target && player != locked_target))
        {
            continue;
        }
        if (!settings().aim.target_teammates && esp_players().is_teammate(player))
        {
            if (probe)
                probe->attempted = true;
            continue;
        }
        if (!position_of(player, position))
        {
            if (probe)
                probe->attempted = true;
            continue;
        }
        if (settings().aim.ignore_spawn_protected_targets && spawn_protected(player))
        {
            if (probe)
                probe->attempted = true;
            continue;
        }
        const AimTargetPoint point = settings().aim.target_point;
        if (point == AimTargetPoint::Automatic || point == AimTargetPoint::Torso)
        {
            choose_allowed_target(best, local, player, origin, camera_forward, min_dot,
                                  {position.x, position.y + 1.0f, position.z}, max_distance, max_walls, probe);
        }
        if (point == AimTargetPoint::Automatic || point == AimTargetPoint::Head)
        {
            choose_allowed_target(best, local, player, origin, camera_forward, min_dot,
                                  {position.x, position.y + 1.8f, position.z}, max_distance, max_walls, probe);
        }
        if (point == AimTargetPoint::Automatic || point == AimTargetPoint::Base)
        {
            choose_allowed_target(best, local, player, origin, camera_forward, min_dot, position, max_distance,
                                  max_walls, probe);
        }
    }
    if (!locked_target_seen && probe)
        probe->attempted = true;
}

float delta_time()
{
    static MethodInfo* get_delta_time =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Time", "get_deltaTime", 0);
    il2cpp::Object* exception = nullptr;
    float value = 0.0f;
    if (boxed_value(il2cpp::runtime_invoke(get_delta_time, nullptr, nullptr, &exception), value) && !exception &&
        value > 0.0f)
    {
        return value;
    }
    return 1.0f / 60.0f;
}

il2cpp::Object* player_rigidbody(il2cpp::Object* local)
{
    static il2cpp::Class* rigidbody_class = il2cpp::klass("UnityEngine.CoreModule", "UnityEngine", "Rigidbody");
    return component(local, rigidbody_class);
}

MethodInfo* set_use_gravity_method()
{
    static MethodInfo* method =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Rigidbody", "set_useGravity", 1);
    return method;
}

void set_rigidbody_bool(il2cpp::Object* rigidbody, MethodInfo* method, bool value)
{
    if (!rigidbody || !method)
    {
        return;
    }

    void* args[] = {&value};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(method, rigidbody, args, &exception);
}

void set_rigidbody_velocity(il2cpp::Object* rigidbody, const Vector3& velocity)
{
    static MethodInfo* set_velocity =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Rigidbody", "set_velocity", 1);
    if (!rigidbody || !set_velocity)
    {
        return;
    }

    Vector3 value = velocity;
    void* args[] = {&value};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(set_velocity, rigidbody, args, &exception);
}

bool rigidbody_velocity(il2cpp::Object* rigidbody, Vector3& velocity)
{
    static MethodInfo* get_velocity =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Rigidbody", "get_velocity", 0);
    if (!rigidbody || !get_velocity)
    {
        return false;
    }

    il2cpp::Object* exception = nullptr;
    return boxed_value(il2cpp::runtime_invoke(get_velocity, rigidbody, nullptr, &exception), velocity) && !exception;
}

il2cpp::Object*& movement_rigidbody()
{
    static il2cpp::Object* rigidbody = nullptr;
    return rigidbody;
}

bool& original_use_gravity()
{
    static bool value = true;
    return value;
}

bool& rigidbody_gravity_patched()
{
    static bool patched = false;
    return patched;
}

void restore_movement_state()
{
    il2cpp::Object*& rigidbody = movement_rigidbody();
    if (!unity_alive(rigidbody))
    {
        rigidbody = nullptr;
        rigidbody_gravity_patched() = false;
        return;
    }

    if (rigidbody_gravity_patched())
    {
        set_rigidbody_bool(rigidbody, set_use_gravity_method(), original_use_gravity());
    }
    rigidbody = nullptr;
    rigidbody_gravity_patched() = false;
}

bool stat_fields(il2cpp::Object* stat, il2cpp::FieldInfo*& default_value, il2cpp::FieldInfo*& modifier,
                 il2cpp::FieldInfo*& offset)
{
    if (!stat)
    {
        return false;
    }

    static il2cpp::Class* stat_class = nullptr;
    static il2cpp::FieldInfo* default_value_field = nullptr;
    static il2cpp::FieldInfo* modifier_field = nullptr;
    static il2cpp::FieldInfo* offset_field = nullptr;
    if (stat->klass != stat_class)
    {
        stat_class = stat->klass;
        default_value_field = il2cpp::field(stat_class, "defaultValue");
        modifier_field = il2cpp::field(stat_class, "modifier");
        offset_field = il2cpp::field(stat_class, "offset");
    }

    default_value = default_value_field;
    modifier = modifier_field;
    offset = offset_field;
    return default_value && modifier && offset;
}

StatPatch* stat_patches()
{
    static StatPatch patches[max_stat_patches];
    return patches;
}

StatPatch* stat_patch(il2cpp::Object* stat)
{
    if (!stat)
    {
        return nullptr;
    }

    StatPatch* patches = stat_patches();
    for (int i = 0; i < max_stat_patches; ++i)
    {
        if (patches[i].stat == stat)
        {
            return &patches[i];
        }
    }

    il2cpp::FieldInfo* default_value = nullptr;
    il2cpp::FieldInfo* modifier = nullptr;
    il2cpp::FieldInfo* offset = nullptr;
    if (!stat_fields(stat, default_value, modifier, offset))
    {
        return nullptr;
    }

    for (int i = 0; i < max_stat_patches; ++i)
    {
        if (!patches[i].stat)
        {
            patches[i].stat = stat;
            il2cpp::read_field(stat, modifier, &patches[i].modifier);
            il2cpp::read_field(stat, offset, &patches[i].offset);
            return &patches[i];
        }
    }
    return nullptr;
}

void begin_stat_patches()
{
    StatPatch* patches = stat_patches();
    for (int i = 0; i < max_stat_patches; ++i)
    {
        patches[i].used = false;
    }
}

void restore_stat_patches(bool all)
{
    StatPatch* patches = stat_patches();
    for (int i = 0; i < max_stat_patches; ++i)
    {
        StatPatch* patch = &patches[i];
        if (!patch || !patch->stat || (!all && patch->used))
        {
            continue;
        }

        il2cpp::FieldInfo* default_value = nullptr;
        il2cpp::FieldInfo* modifier = nullptr;
        il2cpp::FieldInfo* offset = nullptr;
        if (stat_fields(patch->stat, default_value, modifier, offset))
        {
            il2cpp::write_field(patch->stat, modifier, &patch->modifier);
            il2cpp::write_field(patch->stat, offset, &patch->offset);
        }
        patch->stat = nullptr;
        patch->used = false;
    }
}

char lower_ascii(wchar_t ch)
{
    return ch >= L'A' && ch <= L'Z' ? static_cast<char>(ch - L'A' + L'a') : static_cast<char>(ch);
}

bool string_contains(il2cpp::Object* object, const char* needle)
{
    ManagedString* string = reinterpret_cast<ManagedString*>(object);
    if (!string || !needle || string->length <= 0)
    {
        return false;
    }

    for (int i = 0; i < string->length; ++i)
    {
        int j = 0;
        while (needle[j] && i + j < string->length && lower_ascii(string->chars[i + j]) == needle[j])
        {
            ++j;
        }
        if (!needle[j])
        {
            return true;
        }
    }
    return false;
}

bool gravity_stat(il2cpp::Object* key)
{
    return string_contains(key, "gravity");
}

void set_stat_value(il2cpp::Object* stat, float target)
{
    StatPatch* patch = stat_patch(stat);
    il2cpp::FieldInfo* default_value_field = nullptr;
    il2cpp::FieldInfo* modifier_field = nullptr;
    il2cpp::FieldInfo* offset_field = nullptr;
    if (!patch || !stat_fields(stat, default_value_field, modifier_field, offset_field))
    {
        return;
    }

    float default_value = 0.0f;
    if (!il2cpp::read_field(stat, default_value_field, &default_value))
    {
        return;
    }

    const float modifier = 1.0f;
    const float offset = target - default_value;
    il2cpp::write_field(stat, modifier_field, &modifier);
    il2cpp::write_field(stat, offset_field, &offset);
    patch->used = true;
}

StatDictionary* player_stats(il2cpp::Object* local)
{
    return local ? *reinterpret_cast<StatDictionary**>(reinterpret_cast<char*>(local) +
                                                       game_offsets::fields::player_stats)
                 : nullptr;
}

void apply_controller_stat_patches(const MovementSettings& movement, il2cpp::Object* local)
{
    begin_stat_patches();
    if (!raw_bindings_compatible())
    {
        restore_stat_patches(false);
        return;
    }

    StatDictionary* stats = player_stats(local);
    if (!stats || !stats->entries || stats->count <= 0)
    {
        restore_stat_patches(false);
        return;
    }

    StatDictionaryEntry* entries = reinterpret_cast<StatDictionaryEntry*>(reinterpret_cast<char*>(stats->entries) +
                                                                          offsetof(il2cpp::Array, vector));
    const int total = static_cast<int>(stats->entries->size());
    for (int i = 0; i < stats->count && i < total; ++i)
    {
        StatDictionaryEntry& entry = entries[i];
        if (entry.hash_code < 0 || !entry.key || !entry.value)
        {
            continue;
        }
        if ((movement.no_gravity || movement.custom_gravity) && gravity_stat(entry.key))
        {
            set_stat_value(entry.value,
                           movement.no_gravity ? 0.0f : std::fabs(feature_limits::gravity(movement.gravity)));
        }
    }

    restore_stat_patches(false);
}

Vector3& original_physics_gravity()
{
    static Vector3 gravity{};
    return gravity;
}

bool& physics_gravity_patched()
{
    static bool patched = false;
    return patched;
}

void set_physics_gravity(Vector3 gravity)
{
    static MethodInfo* get_gravity =
        il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Physics", "get_gravity", 0);
    static MethodInfo* set_gravity =
        il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Physics", "set_gravity", 1);
    if (!get_gravity || !set_gravity)
    {
        return;
    }

    bool& patched = physics_gravity_patched();
    if (!patched)
    {
        il2cpp::Object* exception = nullptr;
        boxed_value(il2cpp::runtime_invoke(get_gravity, nullptr, nullptr, &exception), original_physics_gravity());
        patched = !exception;
    }

    void* args[] = {&gravity};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(set_gravity, nullptr, args, &exception);
}

void restore_physics_gravity()
{
    if (!physics_gravity_patched())
    {
        return;
    }

    static MethodInfo* set_gravity =
        il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Physics", "set_gravity", 1);
    if (set_gravity)
    {
        Vector3 gravity = original_physics_gravity();
        void* args[] = {&gravity};
        il2cpp::Object* exception = nullptr;
        il2cpp::runtime_invoke(set_gravity, nullptr, args, &exception);
    }
    physics_gravity_patched() = false;
}

il2cpp::Object* current_local_player()
{
    static il2cpp::FieldInfo* local_instance = il2cpp::field(game().player_controller(), "LocalInstance");
    il2cpp::Object* local = nullptr;
    return local_instance && il2cpp::read_static_field(local_instance, &local) ? local : nullptr;
}

bool local_player(il2cpp::Object* player)
{
    return player && player == current_local_player();
}

void apply_movement_features(const MovementSettings& movement, il2cpp::Object* local)
{
    if (!any_movement_feature_enabled(movement))
    {
        restore_movement_state();
        restore_stat_patches(true);
        restore_physics_gravity();
        return;
    }

    apply_controller_stat_patches(movement, local);
    if (movement.no_gravity)
    {
        set_physics_gravity({0.0f, 0.0f, 0.0f});
    }
    else if (movement.custom_gravity)
    {
        set_physics_gravity({0.0f, feature_limits::gravity(movement.gravity), 0.0f});
    }
    else
    {
        restore_physics_gravity();
    }

    il2cpp::Object* rigidbody = player_rigidbody(local);
    if (!rigidbody)
    {
        return;
    }
    if (movement_rigidbody() != rigidbody)
        restore_movement_state();
    movement_rigidbody() = rigidbody;
    if (!rigidbody_gravity_patched())
    {
        static MethodInfo* get_use_gravity =
            il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Rigidbody", "get_useGravity", 0);
        if (get_use_gravity)
        {
            il2cpp::Object* exception = nullptr;
            rigidbody_gravity_patched() =
                boxed_value(il2cpp::runtime_invoke(get_use_gravity, rigidbody, nullptr, &exception),
                            original_use_gravity()) &&
                !exception;
        }
    }
    if (!rigidbody_gravity_patched())
        return;

    set_rigidbody_bool(rigidbody, set_use_gravity_method(), !movement.no_gravity);

    Vector3 velocity{};
    if (!rigidbody_velocity(rigidbody, velocity))
    {
        return;
    }
    if (movement.custom_gravity && !movement.no_gravity)
    {
        velocity.y += feature_limits::gravity(movement.gravity) * delta_time();
        set_rigidbody_velocity(rigidbody, velocity);
    }
}

il2cpp::Object*& patched_shield()
{
    static il2cpp::Object* shield = nullptr;
    return shield;
}

il2cpp::Object*& patched_shield_player()
{
    static il2cpp::Object* player = nullptr;
    return player;
}

bool& original_shield_active()
{
    static bool active = false;
    return active;
}

bool& shield_patched()
{
    static bool patched = false;
    return patched;
}

bool& original_shield_state()
{
    static bool state = false;
    return state;
}

void set_game_object_active(il2cpp::Object* object, bool active)
{
    static MethodInfo* set_active =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "GameObject", "SetActive", 1);
    if (!object || !set_active)
        return;
    void* args[] = {&active};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(set_active, object, args, &exception);
}

void restore_spawn_protection()
{
    if (shield_patched() && unity_alive(patched_shield()))
    {
        set_game_object_active(patched_shield(), original_shield_active());
    }
    if (shield_patched() && raw_bindings_compatible() && unity_alive(patched_shield_player()))
    {
        *reinterpret_cast<bool*>(reinterpret_cast<char*>(patched_shield_player()) +
                                 game_offsets::fields::player_shield_state) = original_shield_state();
    }
    patched_shield() = nullptr;
    patched_shield_player() = nullptr;
    shield_patched() = false;
}

void apply_spawn_protection_disable(il2cpp::Object* local)
{
    static il2cpp::FieldInfo* shield_object = il2cpp::field(game().player_controller(), "shieldObject");
    static MethodInfo* get_active_self =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "GameObject", "get_activeSelf", 0);
    il2cpp::Object* shield = nullptr;
    if (!raw_bindings_compatible() || !shield_object || !get_active_self ||
        !il2cpp::read_field(local, shield_object, &shield) || !unity_alive(shield))
    {
        return;
    }

    if (patched_shield_player() != local || patched_shield() != shield)
    {
        restore_spawn_protection();
        il2cpp::Object* exception = nullptr;
        shield_patched() = boxed_value(il2cpp::runtime_invoke(get_active_self, shield, nullptr, &exception),
                                       original_shield_active()) &&
                           !exception;
        if (shield_patched())
        {
            patched_shield_player() = local;
            patched_shield() = shield;
            original_shield_state() =
                *reinterpret_cast<bool*>(reinterpret_cast<char*>(local) + game_offsets::fields::player_shield_state);
        }
    }
    if (shield_patched())
    {
        if (*reinterpret_cast<bool*>(reinterpret_cast<char*>(local) + game_offsets::fields::player_shield_state))
        {
            original_shield_state() = true;
        }
        il2cpp::Object* exception = nullptr;
        bool active = false;
        if (boxed_value(il2cpp::runtime_invoke(get_active_self, shield, nullptr, &exception), active) && !exception &&
            active)
        {
            original_shield_active() = true;
        }
        *reinterpret_cast<bool*>(reinterpret_cast<char*>(local) + game_offsets::fields::player_shield_state) = false;
        set_game_object_active(shield, false);
    }
}

il2cpp::Object* player_camera(il2cpp::Object* local)
{
    static il2cpp::FieldInfo* camera_field = il2cpp::field(game().player_controller(), "cam");
    il2cpp::Object* camera = nullptr;
    return camera_field && il2cpp::read_field(local, camera_field, &camera) && unity_alive(camera) ? camera : nullptr;
}

bool camera_fov(il2cpp::Object* camera, float& value)
{
    static MethodInfo* get_field_of_view =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Camera", "get_fieldOfView", 0);
    if (!camera || !get_field_of_view)
    {
        return false;
    }
    il2cpp::Object* exception = nullptr;
    return boxed_value(il2cpp::runtime_invoke(get_field_of_view, camera, nullptr, &exception), value) && !exception;
}

bool set_camera_fov(il2cpp::Object* camera, float value)
{
    static MethodInfo* set_field_of_view =
        il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Camera", "set_fieldOfView", 1);
    if (!camera || !set_field_of_view)
    {
        return false;
    }
    void* args[] = {&value};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(set_field_of_view, camera, args, &exception);
    return !exception;
}

void* verified_game_method(std::uintptr_t rva)
{
    if (!raw_bindings_compatible())
        return nullptr;
    HMODULE game_assembly = GetModuleHandleW(L"GameAssembly.dll");
    return game_assembly ? reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(game_assembly) + rva) : nullptr;
}

void refresh_local_spread(il2cpp::Object* player)
{
    using Refresh = void (*)(il2cpp::Object*, const MethodInfo*);
    if (!raw_bindings_compatible())
        return;
    if (!player ||
        !*reinterpret_cast<il2cpp::Object**>(reinterpret_cast<char*>(player) + game_offsets::fields::player_rigidbody))
        return;
    il2cpp::Array* items =
        *reinterpret_cast<il2cpp::Array**>(reinterpret_cast<char*>(player) + game_offsets::fields::player_items);
    const auto selected =
        *reinterpret_cast<std::uint32_t*>(reinterpret_cast<char*>(player) + game_offsets::fields::player_selected_item);
    if (!items || selected >= items->size())
        return;
    il2cpp::Object* item = items->at(selected);
    if (!item || !*reinterpret_cast<il2cpp::Object**>(reinterpret_cast<char*>(item) + game_offsets::fields::item_info))
        return;

    Refresh compute = reinterpret_cast<Refresh>(verified_game_method(game_offsets::methods::compute_weapon_spread));
    if (!compute)
        return;
    compute(player, nullptr);
    if (*reinterpret_cast<bool*>(reinterpret_cast<char*>(player) + game_offsets::fields::player_ads))
        return;

    const auto style = *reinterpret_cast<std::uint32_t*>(reinterpret_cast<char*>(player) +
                                                         game_offsets::fields::player_crosshair_style);
    const std::ptrdiff_t offsets[] = {
        game_offsets::fields::player_crosshair_left, game_offsets::fields::player_crosshair_right,
        game_offsets::fields::player_crosshair_up, game_offsets::fields::player_crosshair_down};
    for (std::ptrdiff_t offset : offsets)
    {
        il2cpp::Array* transforms = *reinterpret_cast<il2cpp::Array**>(reinterpret_cast<char*>(player) + offset);
        if (!transforms || style >= transforms->size() || !transforms->at(style))
            return;
    }
    Refresh crosshair = reinterpret_cast<Refresh>(verified_game_method(game_offsets::methods::update_crosshair_spread));
    if (crosshair)
        crosshair(player, nullptr);
}

void apply_player_features(const AppSettings& config, il2cpp::Object* local)
{
    apply_movement_features(config.movement, local);
    if (config.player.disable_spawn_protection)
    {
        apply_spawn_protection_disable(local);
    }
    else
    {
        restore_spawn_protection();
    }
}

void apply_item_features(const WeaponSettings& weapons, GameplayItems& items, const GameplayItem& item)
{
    if (weapons.no_spread)
    {
        items.zero_spread(item);
    }
    if (weapons.infinite_ammo)
    {
        items.infinite_ammo(item);
    }
    if (weapons.instant_reload)
    {
        items.instant_reload(item, feature_limits::reload_time(weapons.reload_time));
    }
    if (weapons.no_camera_shake)
    {
        items.zero_camera_shake(item);
    }
    if (weapons.rapid_fire)
    {
        items.rapid_fire(item);
    }
}
} // namespace

bool Gameplay::redirect_shot(Vector3 origin, Vector3& direction, float max_distance)
{
    const AimSettings& aim = settings().aim;
    target_player_.store(nullptr, std::memory_order_release);
    const bool active = aiming();
    if (!active || !il2cpp::ready() || !game().ready() || distance_sq(direction, {}) < 0.0001f)
    {
        if (!active)
        {
            locked_target_.store(nullptr, std::memory_order_release);
            locked_target_valid_.store(false, std::memory_order_release);
        }
        return false;
    }

    il2cpp::Object* locked_target = locked_target_.load(std::memory_order_acquire);
    locked_target_valid_.store(false, std::memory_order_release);
    AimTarget best{};
    TargetProbe probe{};
    choose_shot_targets(best, origin, direction, aim_min_dot(aim), max_distance, aim.wallbang ? 2 : 0, locked_target,
                        locked_target ? &probe : nullptr);
    if (locked_target && !best.found && probe.attempted && !probe.detected)
    {
        locked_target_.store(nullptr, std::memory_order_release);
        locked_target = nullptr;
        choose_shot_targets(best, origin, direction, aim_min_dot(aim), max_distance, aim.wallbang ? 2 : 0, nullptr,
                            nullptr);
    }
    if (!best.found)
    {
        return false;
    }

    if (!locked_target)
        locked_target_.store(best.player, std::memory_order_release);
    locked_target_valid_.store(true, std::memory_order_release);
    target_player_.store(best.player, std::memory_order_release);
    direction = {best.position.x - origin.x, best.position.y - origin.y, best.position.z - origin.z};
    const float length = std::sqrt(distance_sq(direction, {}));
    direction.x /= length;
    direction.y /= length;
    direction.z /= length;
    return true;
}

bool Gameplay::aiming() const
{
    const AimSettings& aim = settings().aim;
    return aim.enabled && (aim.always_on || (aim.hotkey != 0 && (GetAsyncKeyState(aim.hotkey) & 0x8000) != 0));
}

bool Gameplay::movement_diagnostics(MovementDiagnostics& value) const
{
    std::scoped_lock lock(diagnostics_mutex_);
    if (!diagnostics_valid_)
        return false;
    value = diagnostics_;
    return true;
}

void Gameplay::restore_frame_overrides()
{
    std::scoped_lock lock(frame_overrides_mutex_);
    if (movement_overridden_ && unity_alive(movement_player_))
    {
        Vector3* movement = reinterpret_cast<Vector3*>(reinterpret_cast<char*>(movement_player_) +
                                                       game_offsets::fields::player_movement);
        if (movement->x == overridden_movement_.x && movement->y == overridden_movement_.y &&
            movement->z == overridden_movement_.z)
        {
            *movement = original_movement_;
        }
    }
    movement_player_ = nullptr;
    movement_overridden_ = false;

    if (fov_overridden_)
    {
        float current_fov = 0.0f;
        const bool alive = unity_alive(fov_camera_);
        const bool readable = alive && camera_fov(fov_camera_, current_fov);
        const bool game_replaced_value = readable && std::fabs(current_fov - overridden_fov_) > 0.001f;
        const bool restored = alive && !game_replaced_value && set_camera_fov(fov_camera_, original_fov_);
        if (!alive || game_replaced_value || restored)
        {
            fov_player_ = nullptr;
            fov_camera_ = nullptr;
            fov_overridden_ = false;
        }
    }
}

void Gameplay::before_player_update(il2cpp::Object* player)
{
    il2cpp::Object* local = current_local_player();
    bool owns_override = false;
    {
        std::scoped_lock lock(frame_overrides_mutex_);
        owns_override = (movement_overridden_ && (movement_player_ == player || movement_player_ != local)) ||
                        (fov_overridden_ && (fov_player_ == player || fov_player_ != local));
    }
    if (owns_override)
    {
        restore_frame_overrides();
    }
}

void Gameplay::after_player_update(il2cpp::Object* player)
{
    if (!local_player(player))
    {
        return;
    }

    ready_local_.store(player, std::memory_order_release);
    const AppSettings& config = settings();
    {
        std::scoped_lock lock(diagnostics_mutex_);
        diagnostics_valid_ = false;
        if (config.player.movement_diagnostics && raw_bindings_compatible())
        {
            diagnostics_.intent =
                *reinterpret_cast<Vector3*>(reinterpret_cast<char*>(player) + game_offsets::fields::player_movement);
            diagnostics_.measured_speed = *reinterpret_cast<float*>(reinterpret_cast<char*>(player) +
                                                                    game_offsets::fields::player_measured_speed);
            diagnostics_.ads =
                *reinterpret_cast<bool*>(reinterpret_cast<char*>(player) + game_offsets::fields::player_ads);
            diagnostics_.sprinting =
                *reinterpret_cast<bool*>(reinterpret_cast<char*>(player) + game_offsets::fields::player_sprinting);
            diagnostics_.velocity = {};
            rigidbody_velocity(player_rigidbody(player), diagnostics_.velocity);
            diagnostics_valid_ = true;
        }
    }
    bool cleanup_pending = false;
    {
        std::scoped_lock lock(frame_overrides_mutex_);
        cleanup_pending = movement_overridden_ || fov_overridden_;
    }
    if (cleanup_pending)
    {
        restore_frame_overrides();
    }

    std::scoped_lock lock(frame_overrides_mutex_);
    if (config.movement.high_speed && raw_bindings_compatible() && !movement_overridden_)
    {
        Vector3* movement =
            reinterpret_cast<Vector3*>(reinterpret_cast<char*>(player) + game_offsets::fields::player_movement);
        Vector3 overridden = *movement;
        if (feature_limits::scale_horizontal_movement(overridden.x, overridden.z, config.movement.speed))
        {
            movement_player_ = player;
            original_movement_ = *movement;
            overridden_movement_ = overridden;
            movement_overridden_ = true;
            *movement = overridden;
        }
    }

    if (config.player.custom_fov && !fov_overridden_)
    {
        il2cpp::Object* camera = player_camera(player);
        float fov = 0.0f;
        const float override = feature_limits::camera_fov(config.player.camera_fov);
        if (camera_fov(camera, fov) && set_camera_fov(camera, override))
        {
            fov_player_ = player;
            fov_camera_ = camera;
            original_fov_ = fov;
            overridden_fov_ = override;
            fov_overridden_ = true;
        }
    }
}

il2cpp::Object* Gameplay::target_player() const
{
    return target_player_.load(std::memory_order_acquire);
}

void Gameplay::update_target()
{
    target_player_.store(nullptr, std::memory_order_release);
    if (!aiming())
    {
        locked_target_.store(nullptr, std::memory_order_release);
        locked_target_valid_.store(false, std::memory_order_release);
        return;
    }
    if (il2cpp::Object* locked_target = locked_target_.load(std::memory_order_acquire))
    {
        if (locked_target_valid_.load(std::memory_order_acquire))
            target_player_.store(locked_target, std::memory_order_release);
    }
}

void Gameplay::tick()
{
    AppSettings& config = settings();
    GameplayItems& items = gameplay_items();
    if (!aiming())
    {
        target_player_.store(nullptr, std::memory_order_release);
        locked_target_.store(nullptr, std::memory_order_release);
        locked_target_valid_.store(false, std::memory_order_release);
    }

    if (!any_feature_enabled(config))
    {
        if (il2cpp::ready() && game().ready())
        {
            il2cpp::attach_thread();
            il2cpp::Object* local = current_local_player();
            if (items.resolve())
            {
                items.begin_session(local);
                if (local)
                {
                    if (items.restore_disabled(config.weapons, local))
                        refresh_local_spread(local);
                    items.restore_all();
                }
                else
                {
                    items.begin_session(nullptr);
                }
            }
            restore_frame_overrides();
            restore_movement_state();
            restore_stat_patches(true);
            restore_physics_gravity();
            restore_spawn_protection();
        }
        target_player_.store(nullptr, std::memory_order_release);
        locked_target_.store(nullptr, std::memory_order_release);
        locked_target_valid_.store(false, std::memory_order_release);
        return;
    }
    if (!il2cpp::ready() || !game().ready())
    {
        return;
    }

    il2cpp::attach_thread();

    const bool items_ready = items.resolve();
    il2cpp::Object* local = current_local_player();
    if (!local)
    {
        ready_local_.store(nullptr, std::memory_order_release);
        {
            std::scoped_lock lock(diagnostics_mutex_);
            diagnostics_valid_ = false;
        }
        if (items_ready)
            items.begin_session(nullptr);
        restore_frame_overrides();
        restore_movement_state();
        restore_stat_patches(true);
        restore_physics_gravity();
        restore_spawn_protection();
        target_player_.store(nullptr, std::memory_order_release);
        locked_target_.store(nullptr, std::memory_order_release);
        locked_target_valid_.store(false, std::memory_order_release);
        return;
    }

    if (items_ready)
        items.begin_session(local);
    if (ready_local_.load(std::memory_order_acquire) != local)
    {
        if (patched_shield_player() && patched_shield_player() != local)
            restore_spawn_protection();
        restore_frame_overrides();
        restore_movement_state();
        restore_stat_patches(true);
        restore_physics_gravity();
        target_player_.store(nullptr, std::memory_order_release);
        locked_target_.store(nullptr, std::memory_order_release);
        locked_target_valid_.store(false, std::memory_order_release);
        return;
    }

    if (items_ready && items.restore_disabled(config.weapons, local))
        refresh_local_spread(local);
    apply_player_features(config, local);
    if (items_ready && config.weapons.no_spread)
        items.zero_player_spread(local);
    update_target();

    il2cpp::Array* local_items = items_ready ? items.local_items(local) : nullptr;
    const int total = local_items ? static_cast<int>(local_items->size()) : 0;
    for (int i = 0; i < total; ++i)
    {
        GameplayItem item{};
        if (items.prepare(local_items->at(i), item))
        {
            apply_item_features(config.weapons, items, item);
        }
    }
}

void Gameplay::restore()
{
    target_player_.store(nullptr, std::memory_order_release);
    locked_target_.store(nullptr, std::memory_order_release);
    locked_target_valid_.store(false, std::memory_order_release);
    ready_local_.store(nullptr, std::memory_order_release);
    {
        std::scoped_lock lock(diagnostics_mutex_);
        diagnostics_valid_ = false;
    }
    if (il2cpp::ready() && game().ready())
    {
        il2cpp::attach_thread();
        GameplayItems& items = gameplay_items();
        il2cpp::Object* local = current_local_player();
        WeaponSettings disabled{};
        if (items.resolve())
        {
            items.begin_session(local);
            if (local)
            {
                if (items.restore_disabled(disabled, local))
                    refresh_local_spread(local);
                items.restore_all();
            }
            else
            {
                items.begin_session(nullptr);
            }
        }
        restore_frame_overrides();
        restore_movement_state();
        restore_stat_patches(true);
        restore_physics_gravity();
        restore_spawn_protection();
    }
}

Gameplay& gameplay()
{
    static Gameplay instance;
    return instance;
}
} // namespace hackmatch
