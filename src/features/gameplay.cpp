#include "gameplay.h"

#include "game.h"
#include "game_offsets.h"
#include "feature_limits.h"
#include "gameplay_items.h"
#include "il2cpp_api.h"

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace hackmatch {
namespace {
struct AimTarget {
    il2cpp::Object* player = nullptr;
    Vector3 position{};
    float dot = -1.0f;
    float distance_sq = 0.0f;
    bool found = false;
};

struct ManagedString : il2cpp::Object {
    int length;
    wchar_t chars[1];
};

struct Vector2 {
    float x;
    float y;
};

struct RaycastHit {
    Vector3 point;
    Vector3 normal;
    std::uint32_t face_id;
    float distance;
    Vector2 uv;
    int collider;
};

struct AimHitDistances {
    float wall = 0.0f;
    float player = 0.0f;
    bool has_wall = false;
    bool has_player = false;
    float wall_distances[16]{};
    int wall_count = 0;
};

struct StatDictionaryEntry {
    int hash_code;
    int next;
    il2cpp::Object* key;
    il2cpp::Object* value;
};

struct StatDictionary : il2cpp::Object {
    il2cpp::Array* buckets;
    il2cpp::Array* entries;
    int count;
    int version;
    int free_list;
    int free_count;
};

struct StatPatch {
    il2cpp::Object* stat = nullptr;
    float modifier = 0.0f;
    float offset = 0.0f;
    bool used = false;
};

constexpr int max_stat_patches = 64;

bool any_feature_enabled(const AppSettings& config)
{
    const auto& weapons = config.weapons;
    const auto& movement = config.movement;
    const auto& player = config.player;
    return weapons.no_spread || weapons.infinite_ammo || weapons.instant_reload || weapons.no_camera_shake || config.aim.enabled || weapons.rapid_fire || weapons.custom_damage || player.custom_fov ||
        movement.auto_sprint || movement.no_gravity || movement.custom_gravity || movement.high_speed || player.disable_spawn_protection;
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

template <class T>
bool boxed_value(il2cpp::Object* object, T& out)
{
    if (!object) {
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
    static MethodInfo* object_equals = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Object", "op_Equality", 2);
    if (!object || !object_equals) {
        return false;
    }

    void* args[] = {object, nullptr};
    il2cpp::Object* exception = nullptr;
    bool is_null = true;
    return boxed_value(il2cpp::runtime_invoke(object_equals, nullptr, args, &exception), is_null) && !exception && !is_null;
}

bool position_of(il2cpp::Object* object, Vector3& out)
{
    static MethodInfo* get_transform = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "get_transform", 0);
    static MethodInfo* get_position = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Transform", "get_position", 0);
    if (!get_transform || !get_position || !unity_alive(object)) {
        return false;
    }

    il2cpp::Object* exception = nullptr;
    il2cpp::Object* transform = il2cpp::runtime_invoke(get_transform, object, nullptr, &exception);
    if (exception || !unity_alive(transform)) {
        return false;
    }

    exception = nullptr;
    return boxed_value(il2cpp::runtime_invoke(get_position, transform, nullptr, &exception), out) && !exception;
}

il2cpp::Object* transform_of(il2cpp::Object* object)
{
    static MethodInfo* get_transform = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "get_transform", 0);
    if (!get_transform || !unity_alive(object)) {
        return nullptr;
    }

    il2cpp::Object* exception = nullptr;
    il2cpp::Object* transform = il2cpp::runtime_invoke(get_transform, object, nullptr, &exception);
    return exception || !unity_alive(transform) ? nullptr : transform;
}

bool transform_position(il2cpp::Object* transform, Vector3& out)
{
    static MethodInfo* get_position = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Transform", "get_position", 0);
    if (!transform || !get_position) {
        return false;
    }

    il2cpp::Object* exception = nullptr;
    return boxed_value(il2cpp::runtime_invoke(get_position, transform, nullptr, &exception), out) && !exception;
}

void set_transform_position(il2cpp::Object* transform, const Vector3& position)
{
    static MethodInfo* set_position = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Transform", "set_position", 1);
    if (!transform || !set_position) {
        return;
    }

    Vector3 value = position;
    void* args[] = {&value};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(set_position, transform, args, &exception);
}

il2cpp::Object* component(il2cpp::Object* object, il2cpp::Class* component_class)
{
    static MethodInfo* get_component = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "GetComponent", 1);
    il2cpp::Object* type = il2cpp::type_object(component_class);
    if (!object || !get_component || !type) {
        return nullptr;
    }

    void* args[] = {type};
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* found = il2cpp::runtime_invoke(get_component, object, args, &exception);
    return exception ? nullptr : found;
}

bool camera_aim_state(il2cpp::Object* local, Vector3& position, Vector3& forward)
{
    static il2cpp::FieldInfo* player_camera = il2cpp::field(game().player_controller(), "cam");
    static MethodInfo* get_transform = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "get_transform", 0);
    static MethodInfo* get_position = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Transform", "get_position", 0);
    static MethodInfo* get_forward = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Transform", "get_forward", 0);
    if (!local || !player_camera || !get_transform || !get_position || !get_forward) {
        return false;
    }

    il2cpp::Object* camera = nullptr;
    if (!il2cpp::read_field(local, player_camera, &camera) || !unity_alive(camera)) {
        return false;
    }

    il2cpp::Object* exception = nullptr;
    il2cpp::Object* transform = il2cpp::runtime_invoke(get_transform, camera, nullptr, &exception);
    if (exception || !unity_alive(transform)) {
        return false;
    }

    exception = nullptr;
    if (!boxed_value(il2cpp::runtime_invoke(get_position, transform, nullptr, &exception), position) || exception) {
        return false;
    }
    exception = nullptr;
    return boxed_value(il2cpp::runtime_invoke(get_forward, transform, nullptr, &exception), forward) && !exception && distance_sq(forward, {}) > 0.0001f;
}

void choose_target(AimTarget& best, il2cpp::Object* player, const Vector3& camera_position, const Vector3& camera_forward, float min_dot, Vector3 target_position)
{
    const float distance = distance_sq(camera_position, target_position);
    if (distance < 0.0001f) {
        return;
    }

    Vector3 direction{target_position.x - camera_position.x, target_position.y - camera_position.y, target_position.z - camera_position.z};
    const float inv_length = 1.0f / std::sqrt(distance);
    const float dot = (direction.x * camera_forward.x + direction.y * camera_forward.y + direction.z * camera_forward.z) * inv_length;
    if (dot < min_dot) {
        return;
    }

    if (!best.found || dot > best.dot + 0.002f || (dot + 0.002f >= best.dot && distance < best.distance_sq)) {
        best.player = player;
        best.position = target_position;
        best.dot = dot;
        best.distance_sq = distance;
        best.found = true;
    }
}

bool add_wall_distance(AimHitDistances& hits, float distance)
{
    for (int i = 0; i < hits.wall_count; ++i) {
        if (std::fabs(hits.wall_distances[i] - distance) < 0.35f) {
            return false;
        }
    }
    if (hits.wall_count < static_cast<int>(sizeof(hits.wall_distances) / sizeof(hits.wall_distances[0]))) {
        hits.wall_distances[hits.wall_count++] = distance;
    }
    return true;
}

void choose_player_targets(AimTarget& best, const Vector3& camera_position, const Vector3& camera_forward, float min_dot)
{
    static il2cpp::FieldInfo* local_instance = il2cpp::field(game().player_controller(), "LocalInstance");
    il2cpp::Object* local = nullptr;
    il2cpp::Array* players = il2cpp::objects_of_type(game().player_controller());
    if (!players || !local_instance || !il2cpp::read_static_field(local_instance, &local)) {
        return;
    }

    const int total = static_cast<int>(players->size());
    for (int i = 0; i < total; ++i) {
        il2cpp::Object* player = players->at(i);
        Vector3 position{};
        if (player == local || !position_of(player, position)) {
            continue;
        }
        choose_target(best, player, camera_position, camera_forward, min_dot, position);
    }
}

il2cpp::Object* raycast_hit_collider(RaycastHit& hit)
{
    static MethodInfo* get_collider = il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "RaycastHit", "get_collider", 0);
    using GetColliderFn = il2cpp::Object* (*)(RaycastHit, const MethodInfo*);
    static GetColliderFn fn = il2cpp::method_pointer<GetColliderFn>(get_collider);
    il2cpp::Object* collider = fn ? fn(hit, get_collider) : nullptr;
    if (collider || hit.collider == 0) {
        return collider;
    }

    static MethodInfo* find_object = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Object", "FindObjectFromInstanceID", 1);
    void* args[] = {&hit.collider};
    il2cpp::Object* exception = nullptr;
    collider = find_object ? il2cpp::runtime_invoke(find_object, nullptr, args, &exception) : nullptr;
    return exception ? nullptr : collider;
}

il2cpp::Object* raycast_hit_transform(RaycastHit& hit)
{
    static MethodInfo* get_transform = il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "RaycastHit", "get_transform", 0);
    using GetTransformFn = il2cpp::Object* (*)(RaycastHit, const MethodInfo*);
    static GetTransformFn fn = il2cpp::method_pointer<GetTransformFn>(get_transform);
    return fn ? fn(hit, get_transform) : nullptr;
}

il2cpp::Object* component_player(il2cpp::Object* component)
{
    static MethodInfo* get_component = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "GetComponent", 1);
    static MethodInfo* get_component_in_parent = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "GetComponentInParent", 1);
    static il2cpp::Object* player_type = il2cpp::type_object(game().player_controller());
    if (!component || !player_type) {
        return nullptr;
    }

    void* args[] = {player_type};
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* player = get_component ? il2cpp::runtime_invoke(get_component, component, args, &exception) : nullptr;
    if (!exception && player) {
        return player;
    }

    exception = nullptr;
    player = get_component_in_parent ? il2cpp::runtime_invoke(get_component_in_parent, component, args, &exception) : nullptr;
    return exception ? nullptr : player;
}

il2cpp::Object* collider_player(il2cpp::Object* collider)
{
    il2cpp::Object* player = component_player(collider);
    if (player) {
        return player;
    }

    static MethodInfo* get_attached_rigidbody = il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Collider", "get_attachedRigidbody", 0);
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* rigidbody = get_attached_rigidbody ? il2cpp::runtime_invoke(get_attached_rigidbody, collider, nullptr, &exception) : nullptr;
    return exception ? nullptr : component_player(rigidbody);
}

bool walls_before_player(const Vector3& origin, const Vector3& direction, float max_distance, il2cpp::Object* local, il2cpp::Object* target, int& walls)
{
    static MethodInfo* raycast_all = il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Physics", "RaycastAll", 5);
    if (!raycast_all || !target) {
        return false;
    }

    Vector3 ray_origin = origin;
    Vector3 ray_direction = direction;
    float distance = max_distance;
    int layer_mask = -1;
    int query_trigger_interaction = 2;
    void* args[] = {&ray_origin, &ray_direction, &distance, &layer_mask, &query_trigger_interaction};
    il2cpp::Object* exception = nullptr;
    il2cpp::Array* hits = reinterpret_cast<il2cpp::Array*>(il2cpp::runtime_invoke(raycast_all, nullptr, args, &exception));
    if (exception || !hits || hits->size() == 0) {
        return false;
    }

    AimHitDistances distances{};
    RaycastHit* data = reinterpret_cast<RaycastHit*>(reinterpret_cast<char*>(hits) + offsetof(il2cpp::Array, vector));
    for (std::uintptr_t i = 0; i < hits->size(); ++i) {
        RaycastHit& hit = data[i];
        if (hit.distance < 1.5f || hit.distance > max_distance) {
            continue;
        }

        il2cpp::Object* collider = raycast_hit_collider(hit);
        il2cpp::Object* player = collider_player(collider);
        if (!player) {
            player = component_player(raycast_hit_transform(hit));
        }
        if (player == local) {
            continue;
        }
        if (player == target) {
            if (!distances.has_player || hit.distance < distances.player) {
                distances.player = hit.distance;
                distances.has_player = true;
            }
            continue;
        }
        if (!player) {
            add_wall_distance(distances, hit.distance);
        }
    }
    if (!distances.has_player) {
        return false;
    }

    walls = 0;
    for (int i = 0; i < distances.wall_count; ++i) {
        if (distances.wall_distances[i] < distances.player) {
            ++walls;
        }
    }
    return true;
}

void choose_allowed_target(AimTarget& best, il2cpp::Object* local, il2cpp::Object* player, const Vector3& origin, const Vector3& camera_forward, float min_dot, Vector3 target_position, int max_walls)
{
    const float distance = distance_sq(origin, target_position);
    if (distance < 0.0001f) {
        return;
    }

    Vector3 direction{target_position.x - origin.x, target_position.y - origin.y, target_position.z - origin.z};
    const float length = std::sqrt(distance);
    direction.x /= length;
    direction.y /= length;
    direction.z /= length;

    const float dot = direction.x * camera_forward.x + direction.y * camera_forward.y + direction.z * camera_forward.z;
    if (dot < min_dot) {
        return;
    }

    int walls = 0;
    if (!walls_before_player(origin, direction, length + 2.0f, local, player, walls) || walls > max_walls) {
        return;
    }
    choose_target(best, player, origin, camera_forward, min_dot, target_position);
}

void choose_shot_targets(AimTarget& best, const Vector3& origin, const Vector3& camera_forward, float min_dot, int max_walls)
{
    static il2cpp::FieldInfo* local_instance = il2cpp::field(game().player_controller(), "LocalInstance");
    il2cpp::Object* local = nullptr;
    il2cpp::Array* players = il2cpp::objects_of_type(game().player_controller());
    if (!players || !local_instance || !il2cpp::read_static_field(local_instance, &local)) {
        return;
    }

    const int total = static_cast<int>(players->size());
    for (int i = 0; i < total; ++i) {
        il2cpp::Object* player = players->at(i);
        Vector3 position{};
        if (player == local || !position_of(player, position)) {
            continue;
        }
        choose_allowed_target(best, local, player, origin, camera_forward, min_dot, {position.x, position.y + 1.0f, position.z}, max_walls);
        choose_allowed_target(best, local, player, origin, camera_forward, min_dot, {position.x, position.y + 1.8f, position.z}, max_walls);
        choose_allowed_target(best, local, player, origin, camera_forward, min_dot, position, max_walls);
    }
}

float delta_time()
{
    static MethodInfo* get_delta_time = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Time", "get_deltaTime", 0);
    il2cpp::Object* exception = nullptr;
    float value = 0.0f;
    if (boxed_value(il2cpp::runtime_invoke(get_delta_time, nullptr, nullptr, &exception), value) && !exception && value > 0.0f) {
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
    static MethodInfo* method = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Rigidbody", "set_useGravity", 1);
    return method;
}

MethodInfo* set_detect_collisions_method()
{
    static MethodInfo* method = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Rigidbody", "set_detectCollisions", 1);
    return method;
}

void set_rigidbody_bool(il2cpp::Object* rigidbody, MethodInfo* method, bool value)
{
    if (!rigidbody || !method) {
        return;
    }

    void* args[] = {&value};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(method, rigidbody, args, &exception);
}

void set_rigidbody_velocity(il2cpp::Object* rigidbody, const Vector3& velocity)
{
    static MethodInfo* set_velocity = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Rigidbody", "set_velocity", 1);
    if (!rigidbody || !set_velocity) {
        return;
    }

    Vector3 value = velocity;
    void* args[] = {&value};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(set_velocity, rigidbody, args, &exception);
}

bool rigidbody_velocity(il2cpp::Object* rigidbody, Vector3& velocity)
{
    static MethodInfo* get_velocity = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Rigidbody", "get_velocity", 0);
    if (!rigidbody || !get_velocity) {
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

void restore_movement_state()
{
    il2cpp::Object*& rigidbody = movement_rigidbody();
    if (!unity_alive(rigidbody)) {
        rigidbody = nullptr;
        return;
    }

    set_rigidbody_bool(rigidbody, set_use_gravity_method(), true);
    set_rigidbody_bool(rigidbody, set_detect_collisions_method(), true);
    rigidbody = nullptr;
}

bool stat_fields(il2cpp::Object* stat, il2cpp::FieldInfo*& default_value, il2cpp::FieldInfo*& modifier, il2cpp::FieldInfo*& offset)
{
    if (!stat) {
        return false;
    }

    static il2cpp::Class* stat_class = nullptr;
    static il2cpp::FieldInfo* default_value_field = nullptr;
    static il2cpp::FieldInfo* modifier_field = nullptr;
    static il2cpp::FieldInfo* offset_field = nullptr;
    if (stat->klass != stat_class) {
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
    if (!stat) {
        return nullptr;
    }

    StatPatch* patches = stat_patches();
    for (int i = 0; i < max_stat_patches; ++i) {
        if (patches[i].stat == stat) {
            return &patches[i];
        }
    }

    il2cpp::FieldInfo* default_value = nullptr;
    il2cpp::FieldInfo* modifier = nullptr;
    il2cpp::FieldInfo* offset = nullptr;
    if (!stat_fields(stat, default_value, modifier, offset)) {
        return nullptr;
    }

    for (int i = 0; i < max_stat_patches; ++i) {
        if (!patches[i].stat) {
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
    for (int i = 0; i < max_stat_patches; ++i) {
        patches[i].used = false;
    }
}

void restore_stat_patches(bool all)
{
    StatPatch* patches = stat_patches();
    for (int i = 0; i < max_stat_patches; ++i) {
        StatPatch* patch = &patches[i];
        if (!patch || !patch->stat || (!all && patch->used)) {
            continue;
        }

        il2cpp::FieldInfo* default_value = nullptr;
        il2cpp::FieldInfo* modifier = nullptr;
        il2cpp::FieldInfo* offset = nullptr;
        if (stat_fields(patch->stat, default_value, modifier, offset)) {
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
    if (!string || !needle || string->length <= 0) {
        return false;
    }

    for (int i = 0; i < string->length; ++i) {
        int j = 0;
        while (needle[j] && i + j < string->length && lower_ascii(string->chars[i + j]) == needle[j]) {
            ++j;
        }
        if (!needle[j]) {
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
    if (!patch || !stat_fields(stat, default_value_field, modifier_field, offset_field)) {
        return;
    }

    float default_value = 0.0f;
    if (!il2cpp::read_field(stat, default_value_field, &default_value)) {
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
    return local ? *reinterpret_cast<StatDictionary**>(reinterpret_cast<char*>(local) + game_offsets::fields::player_stats) : nullptr;
}

void apply_controller_stat_patches(const MovementSettings& movement, il2cpp::Object* local)
{
    begin_stat_patches();

    StatDictionary* stats = player_stats(local);
    if (!stats || !stats->entries || stats->count <= 0) {
        restore_stat_patches(false);
        return;
    }

    StatDictionaryEntry* entries = reinterpret_cast<StatDictionaryEntry*>(reinterpret_cast<char*>(stats->entries) + offsetof(il2cpp::Array, vector));
    const int total = static_cast<int>(stats->entries->size());
    for (int i = 0; i < stats->count && i < total; ++i) {
        StatDictionaryEntry& entry = entries[i];
        if (entry.hash_code < 0 || !entry.key || !entry.value) {
            continue;
        }
        if ((movement.no_gravity || movement.custom_gravity) && gravity_stat(entry.key)) {
            set_stat_value(entry.value, movement.no_gravity ? 0.0f : std::fabs(feature_limits::gravity(movement.gravity)));
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
    static MethodInfo* get_gravity = il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Physics", "get_gravity", 0);
    static MethodInfo* set_gravity = il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Physics", "set_gravity", 1);
    if (!get_gravity || !set_gravity) {
        return;
    }

    bool& patched = physics_gravity_patched();
    if (!patched) {
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
    if (!physics_gravity_patched()) {
        return;
    }

    static MethodInfo* set_gravity = il2cpp::method("UnityEngine.PhysicsModule", "UnityEngine", "Physics", "set_gravity", 1);
    if (set_gravity) {
        Vector3 gravity = original_physics_gravity();
        void* args[] = {&gravity};
        il2cpp::Object* exception = nullptr;
        il2cpp::runtime_invoke(set_gravity, nullptr, args, &exception);
    }
    physics_gravity_patched() = false;
}

bool key_down(int key)
{
    return (GetAsyncKeyState(key) & 0x8000) != 0;
}

void apply_high_speed_movement(const MovementSettings& movement, il2cpp::Object* local)
{
    float input_x = 0.0f;
    float input_z = 0.0f;
    if (key_down('W')) {
        input_z += 1.0f;
    }
    if (key_down('S')) {
        input_z -= 1.0f;
    }
    if (key_down('D')) {
        input_x += 1.0f;
    }
    if (key_down('A')) {
        input_x -= 1.0f;
    }
    if (input_x == 0.0f && input_z == 0.0f) {
        return;
    }

    Vector3 camera_position{};
    Vector3 forward{};
    if (!camera_aim_state(local, camera_position, forward)) {
        return;
    }

    forward.y = 0.0f;
    const float forward_sq = forward.x * forward.x + forward.z * forward.z;
    if (forward_sq < 0.0001f) {
        return;
    }
    const float inv_forward = 1.0f / std::sqrt(forward_sq);
    forward.x *= inv_forward;
    forward.z *= inv_forward;

    const Vector3 right{forward.z, 0.0f, -forward.x};
    Vector3 move{right.x * input_x + forward.x * input_z, 0.0f, right.z * input_x + forward.z * input_z};
    const float move_sq = move.x * move.x + move.z * move.z;
    if (move_sq < 0.0001f) {
        return;
    }
    const float distance = feature_limits::speed(movement.speed) * delta_time() / std::sqrt(move_sq);
    move.x *= distance;
    move.z *= distance;

    il2cpp::Object* transform = transform_of(local);
    Vector3 position{};
    if (!transform_position(transform, position)) {
        return;
    }

    position.x += move.x;
    position.z += move.z;
    set_transform_position(transform, position);
}

bool local_player(il2cpp::Object* player)
{
    static il2cpp::FieldInfo* local_instance = il2cpp::field(game().player_controller(), "LocalInstance");
    il2cpp::Object* local = nullptr;
    return player && local_instance && il2cpp::read_static_field(local_instance, &local) && player == local;
}

void apply_movement_features(const MovementSettings& movement, il2cpp::Object* local)
{
    if (!any_movement_feature_enabled(movement)) {
        restore_movement_state();
        restore_stat_patches(true);
        restore_physics_gravity();
        return;
    }

    apply_controller_stat_patches(movement, local);
    if (movement.no_gravity) {
        set_physics_gravity({0.0f, 0.0f, 0.0f});
    } else if (movement.custom_gravity) {
        set_physics_gravity({0.0f, feature_limits::gravity(movement.gravity), 0.0f});
    } else {
        restore_physics_gravity();
    }

    il2cpp::Object* rigidbody = player_rigidbody(local);
    if (!rigidbody) {
        return;
    }
    movement_rigidbody() = rigidbody;

    set_rigidbody_bool(rigidbody, set_use_gravity_method(), !movement.no_gravity);

    Vector3 velocity{};
    if (!rigidbody_velocity(rigidbody, velocity)) {
        return;
    }
    if (movement.custom_gravity && !movement.no_gravity) {
        velocity.y += feature_limits::gravity(movement.gravity) * delta_time();
        set_rigidbody_velocity(rigidbody, velocity);
    }
}

void apply_spawn_protection_disable(il2cpp::Object* local)
{
    static il2cpp::FieldInfo* shield_object = il2cpp::field(game().player_controller(), "shieldObject");
    static MethodInfo* set_active = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "GameObject", "SetActive", 1);
    il2cpp::Object* shield = nullptr;
    if (!shield_object || !set_active || !il2cpp::read_field(local, shield_object, &shield) || !shield) {
        return;
    }

    bool inactive = false;
    void* args[] = {&inactive};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(set_active, shield, args, &exception);
}

void apply_camera_fov(const PlayerSettings& player, il2cpp::Object* local)
{
    if (!player.custom_fov) {
        return;
    }

    static MethodInfo* get_main_camera = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Camera", "get_main", 0);
    static MethodInfo* set_field_of_view = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Camera", "set_fieldOfView", 1);
    static il2cpp::FieldInfo* player_camera = il2cpp::field(game().player_controller(), "cam");
    if (!set_field_of_view) {
        return;
    }

    il2cpp::Object* camera = nullptr;
    if (player_camera) {
        il2cpp::read_field(local, player_camera, &camera);
    }
    if (!camera && get_main_camera) {
        il2cpp::Object* exception = nullptr;
        camera = il2cpp::runtime_invoke(get_main_camera, nullptr, nullptr, &exception);
        if (exception) {
            camera = nullptr;
        }
    }
    if (!camera) {
        return;
    }

    float fov = feature_limits::camera_fov(player.camera_fov);
    void* args[] = {&fov};
    il2cpp::Object* exception = nullptr;
    il2cpp::runtime_invoke(set_field_of_view, camera, args, &exception);
}

void apply_player_features(const AppSettings& config, GameplayItems& items, il2cpp::Object* local)
{
    apply_movement_features(config.movement, local);
    if (config.player.disable_spawn_protection) {
        apply_spawn_protection_disable(local);
    }
    if (config.weapons.no_spread) {
        items.zero_player_spread(local);
    }
    apply_camera_fov(config.player, local);
}

void apply_item_features(const WeaponSettings& weapons, GameplayItems& items, const GameplayItem& item)
{
    if (weapons.no_spread) {
        items.zero_spread(item);
    }
    if (weapons.infinite_ammo) {
        items.infinite_ammo(item);
    }
    if (weapons.instant_reload) {
        items.instant_reload(item);
    }
    if (weapons.no_camera_shake) {
        items.zero_camera_shake(item);
    }
    if (weapons.rapid_fire) {
        items.rapid_fire(item);
    }
    if (weapons.custom_damage) {
        items.custom_damage(item);
    }
}
}

bool Gameplay::redirect_shot(Vector3 origin, Vector3& direction, float& target_distance)
{
    const AimSettings& aim = settings().aim;
    target_player_ = nullptr;
    if (!aiming() || !il2cpp::ready() || !game().ready() || distance_sq(direction, {}) < 0.0001f) {
        return false;
    }

    AimTarget best{};
    choose_shot_targets(best, origin, direction, aim_min_dot(aim), aim.wallbang ? 2 : 0);
    if (!best.found) {
        return false;
    }

    target_player_ = best.player;
    direction = {best.position.x - origin.x, best.position.y - origin.y, best.position.z - origin.z};
    const float length = std::sqrt(distance_sq(direction, {}));
    direction.x /= length;
    direction.y /= length;
    direction.z /= length;
    target_distance = length;
    return true;
}

bool Gameplay::aiming() const
{
    const AimSettings& aim = settings().aim;
    return aim.enabled && (aim.always_on || (aim.hotkey != 0 && (GetAsyncKeyState(aim.hotkey) & 0x8000) != 0));
}

void Gameplay::after_player_update(il2cpp::Object* player)
{
    if (local_player(player)) {
        ready_local_.store(player, std::memory_order_release);
    }
    if (settings().movement.high_speed && local_player(player)) {
        apply_high_speed_movement(settings().movement, player);
    }
}

il2cpp::Object* Gameplay::target_player() const
{
    return target_player_;
}

void Gameplay::update_target(il2cpp::Object* local)
{
    target_player_ = nullptr;
    if (!aiming()) {
        return;
    }

    Vector3 camera_position{};
    Vector3 camera_forward{};
    if (!camera_aim_state(local, camera_position, camera_forward)) {
        return;
    }

    AimTarget best{};
    choose_player_targets(best, camera_position, camera_forward, aim_min_dot(settings().aim));
    if (best.found) {
        target_player_ = best.player;
    }
}

void Gameplay::tick()
{
    AppSettings& config = settings();
    GameplayItems& items = gameplay_items();
    if (!aiming()) {
        target_player_ = nullptr;
    }

    if (!any_feature_enabled(config)) {
        if (il2cpp::ready() && game().ready()) {
            il2cpp::attach_thread();
            restore_movement_state();
            restore_stat_patches(true);
            restore_physics_gravity();
        }
        items.restore_all();
        target_player_ = nullptr;
        return;
    }
    if (!il2cpp::ready() || !game().ready() || !items.resolve()) {
        return;
    }

    il2cpp::attach_thread();

    il2cpp::Object* local = items.local_player();
    if (!local) {
        ready_local_.store(nullptr, std::memory_order_release);
        items.begin_session(nullptr);
        restore_movement_state();
        restore_stat_patches(true);
        restore_physics_gravity();
        target_player_ = nullptr;
        return;
    }

    items.begin_session(local);
    if (ready_local_.load(std::memory_order_acquire) != local) {
        return;
    }

    apply_player_features(config, items, local);
    update_target(local);

    il2cpp::Array* local_items = items.local_items(local);
    const int total = local_items ? static_cast<int>(local_items->size()) : 0;
    for (int i = 0; i < total; ++i) {
        GameplayItem item{};
        if (items.prepare(local_items->at(i), item)) {
            apply_item_features(config.weapons, items, item);
        }
    }
}

void Gameplay::restore()
{
    target_player_ = nullptr;
    ready_local_.store(nullptr, std::memory_order_release);
    if (il2cpp::ready() && game().ready()) {
        il2cpp::attach_thread();
        restore_movement_state();
        restore_stat_patches(true);
        restore_physics_gravity();
    }
    gameplay_items().restore_all();
}

Gameplay& gameplay()
{
    static Gameplay instance;
    return instance;
}
}
