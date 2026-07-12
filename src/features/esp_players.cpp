#include "esp_players.h"

#include "build_info.h"
#include "game.h"
#include "game_offsets.h"

#include <windows.h>

#include <cmath>

namespace hackmatch
{
namespace
{
template <class T> T unbox(il2cpp::Object* object)
{
    return *reinterpret_cast<T*>(reinterpret_cast<char*>(object) + sizeof(il2cpp::Object));
}
} // namespace

bool EspPlayers::refresh()
{
    if (build_compatibility().status != BuildStatus::Compatible || !game().ready() || !resolve())
    {
        reset();
        return false;
    }

    il2cpp::attach_thread();
    if (!scan_players())
    {
        reset();
        return false;
    }
    update_players();
    return true;
}

void EspPlayers::reset()
{
    count_ = 0;
    outline_count_ = 0;
    team_count_ = 0;
    local_team_ = -1;
    local_material_ = nullptr;
    local_ = nullptr;
    camera_ = nullptr;
    camera_fov_ = 0.0f;
    std::scoped_lock lock(relationship_mutex_);
    relationship_count_ = 0;
    relationship_local_team_ = -1;
    relationship_local_material_ = nullptr;
}

int EspPlayers::count() const
{
    return count_;
}

const EspPlayer& EspPlayers::at(int index) const
{
    return players_[index];
}

bool EspPlayers::is_team(const EspPlayer& player) const
{
    if (player.team >= 0 && local_team_ >= 0)
    {
        return player.team == local_team_;
    }

    return player.material && player.material == local_material_;
}

bool EspPlayers::is_teammate(il2cpp::Object* player) const
{
    std::scoped_lock lock(relationship_mutex_);
    for (int i = 0; player && i < relationship_count_; ++i)
    {
        const OutlineEntry& entry = relationship_entries_[i];
        if (entry.player != player)
            continue;
        if (entry.team >= 0 && relationship_local_team_ >= 0)
            return entry.team == relationship_local_team_;
        return entry.material && relationship_local_material_ && entry.material == relationship_local_material_;
    }
    return false;
}

bool EspPlayers::camera_fov(float& value) const
{
    if (!std::isfinite(camera_fov_) || camera_fov_ <= 0.0f)
        return false;
    value = camera_fov_;
    return true;
}

bool EspPlayers::resolve()
{
    if (!get_transform_ || !get_position_ || !get_main_camera_ || !get_current_camera_ || !world_to_screen_ ||
        !object_equals_)
    {
        get_transform_ = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Component", "get_transform", 0);
        get_position_ = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Transform", "get_position", 0);
        get_main_camera_ = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Camera", "get_main", 0);
        get_current_camera_ = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Camera", "get_current", 0);
        world_to_screen_ = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Camera", "WorldToScreenPoint", 1);
        object_equals_ = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Object", "op_Equality", 2);
        local_instance_ = il2cpp::field(game().player_controller(), "LocalInstance");
        player_camera_ = il2cpp::field(game().player_controller(), "cam");
    }
    if (!get_field_of_view_)
        get_field_of_view_ = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Camera", "get_fieldOfView", 0);
    if (!get_shared_material_)
        get_shared_material_ =
            il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Renderer", "get_sharedMaterial", 0);
    if (!player_team_outline_class_)
        player_team_outline_class_ = il2cpp::klass("Assembly-CSharp", "", "PlayerTeamOutline");
    if (!team_info_manager_class_)
        team_info_manager_class_ = il2cpp::klass("Assembly-CSharp", "", "TeamInfoManager");

    if (player_team_outline_class_ && (!outline_player_ || !outline_identity_ || !outline_renderer_))
    {
        outline_player_ = il2cpp::field(player_team_outline_class_, "player");
        outline_identity_ = il2cpp::field(player_team_outline_class_, "identity");
        outline_renderer_ = il2cpp::field(player_team_outline_class_, "rend");
    }
    if (team_info_manager_class_ && (!team_info_array_ || !team_id_ || !team_outline_material_))
    {
        il2cpp::Class* team_info_class = il2cpp::klass("Assembly-CSharp", "", "TeamInfo");
        team_info_array_ = il2cpp::field(team_info_manager_class_, "info");
        team_id_ = il2cpp::field(team_info_class, "team");
        team_outline_material_ = il2cpp::field(team_info_class, "outlineMaterial");
    }

    return get_transform_ && get_position_ && get_main_camera_ && get_current_camera_ && world_to_screen_ &&
           object_equals_ && local_instance_ && player_camera_;
}

bool EspPlayers::alive(il2cpp::Object* object)
{
    if (!object)
    {
        return false;
    }

    void* args[] = {object, nullptr};
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* boxed = il2cpp::runtime_invoke(object_equals_, nullptr, args, &exception);
    return boxed && !exception ? !unbox<bool>(boxed) : false;
}

std::string EspPlayers::utf8(Il2CppString* string)
{
    if (!string || string->length <= 0 || string->length > 64)
    {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, string->chars, string->length, nullptr, 0, nullptr, nullptr);
    if (size <= 0 || size > 128)
    {
        return {};
    }

    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, string->chars, string->length, result.data(), size, nullptr, nullptr);
    return result;
}

std::string EspPlayers::name_from_identity(il2cpp::Object* identity)
{
    il2cpp::Object* player_data = player_data_from_identity(identity);
    if (!player_data)
    {
        return {};
    }

    return utf8(*reinterpret_cast<Il2CppString**>(reinterpret_cast<char*>(player_data) +
                                                  game_offsets::fields::player_data_name));
}

il2cpp::Object* EspPlayers::player_data_from_identity(il2cpp::Object* identity)
{
    return identity ? *reinterpret_cast<il2cpp::Object**>(reinterpret_cast<char*>(identity) +
                                                          game_offsets::fields::identity_player_data)
                    : nullptr;
}

int EspPlayers::team_from_material(il2cpp::Object* material) const
{
    for (int i = 0; material && i < team_count_; ++i)
    {
        if (team_entries_[i].material == material)
        {
            return team_entries_[i].team;
        }
    }

    return -1;
}

void EspPlayers::scan_team_info()
{
    team_count_ = 0;
    if (!team_info_manager_class_ || !team_info_array_ || !team_id_ || !team_outline_material_)
    {
        return;
    }

    il2cpp::Array* managers = il2cpp::objects_of_type(team_info_manager_class_);
    const int total = managers ? static_cast<int>(managers->size()) : 0;
    for (int i = 0; i < total && team_count_ < max_team_entries; ++i)
    {
        il2cpp::Array* teams = nullptr;
        il2cpp::Object* manager = managers->at(i);
        if (!alive(manager) || !il2cpp::read_field(manager, team_info_array_, &teams) || !teams)
        {
            continue;
        }

        const int team_total = static_cast<int>(teams->size());
        for (int j = 0; j < team_total && team_count_ < max_team_entries; ++j)
        {
            il2cpp::Object* team = teams->at(j);
            if (!team)
            {
                continue;
            }

            TeamEntry entry{};
            if (il2cpp::read_field(team, team_id_, &entry.team) && entry.team >= 0 &&
                il2cpp::read_field(team, team_outline_material_, &entry.material) && entry.material)
            {
                team_entries_[team_count_++] = entry;
            }
        }
    }
}

void EspPlayers::scan_outlines(il2cpp::Object* local)
{
    outline_count_ = 0;
    local_team_ = -1;
    local_material_ = nullptr;
    scan_team_info();

    if (!player_team_outline_class_ || !outline_player_ || !outline_renderer_)
    {
        publish_relationships();
        return;
    }

    il2cpp::Array* outlines = il2cpp::objects_of_type(player_team_outline_class_);
    const int total = outlines ? static_cast<int>(outlines->size()) : 0;
    for (int i = 0; i < total && outline_count_ < max_outline_entries; ++i)
    {
        il2cpp::Object* outline = outlines->at(i);
        if (!alive(outline))
        {
            continue;
        }

        OutlineEntry entry{};
        il2cpp::Object* renderer = nullptr;
        if (!il2cpp::read_field(outline, outline_player_, &entry.player) || !entry.player)
            continue;
        if (outline_identity_)
        {
            il2cpp::read_field(outline, outline_identity_, &entry.identity);
            entry.player_data = player_data_from_identity(entry.identity);
            entry.name = name_from_identity(entry.identity);
        }
        il2cpp::read_field(outline, outline_renderer_, &renderer);

        if (alive(renderer) && get_shared_material_)
        {
            il2cpp::Object* exception = nullptr;
            entry.material = il2cpp::runtime_invoke(get_shared_material_, renderer, nullptr, &exception);
        }
        entry.team = team_from_material(entry.material);
        outline_entries_[outline_count_++] = entry;
        if (entry.player == local)
        {
            local_team_ = entry.team;
            local_material_ = entry.material;
        }
    }
    publish_relationships();
}

void EspPlayers::publish_relationships()
{
    std::scoped_lock lock(relationship_mutex_);
    relationship_count_ = outline_count_;
    for (int i = 0; i < outline_count_; ++i)
        relationship_entries_[i] = outline_entries_[i];
    relationship_local_team_ = local_team_;
    relationship_local_material_ = local_material_;
}

EspPlayers::OutlineEntry* EspPlayers::outline_for(il2cpp::Object* player)
{
    for (int i = 0; player && i < outline_count_; ++i)
    {
        if (outline_entries_[i].player == player)
        {
            return &outline_entries_[i];
        }
    }

    return nullptr;
}

Vec3 EspPlayers::position(il2cpp::Object* object)
{
    if (!alive(object))
    {
        return {};
    }

    il2cpp::Object* exception = nullptr;
    il2cpp::Object* transform = il2cpp::runtime_invoke(get_transform_, object, nullptr, &exception);
    if (!alive(transform) || exception)
    {
        return {};
    }

    exception = nullptr;
    il2cpp::Object* boxed = il2cpp::runtime_invoke(get_position_, transform, nullptr, &exception);
    return boxed && !exception ? unbox<Vec3>(boxed) : Vec3{};
}

il2cpp::Object* EspPlayers::active_camera()
{
    il2cpp::Object* camera = nullptr;
    il2cpp::Object* exception = nullptr;
    if (alive(local_) && player_camera_)
        il2cpp::read_field(local_, player_camera_, &camera);
    if (!alive(camera))
        camera = il2cpp::runtime_invoke(get_current_camera_, nullptr, nullptr, &exception);
    if (!alive(camera))
    {
        exception = nullptr;
        camera = il2cpp::runtime_invoke(get_main_camera_, nullptr, nullptr, &exception);
    }
    return alive(camera) && !exception ? camera : nullptr;
}

bool EspPlayers::world_to_screen(il2cpp::Object* camera, const Vec3& world, ImVec2& out, bool& in_front)
{
    if (!camera)
        return false;

    void* args[] = {const_cast<Vec3*>(&world)};
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* boxed = il2cpp::runtime_invoke(world_to_screen_, camera, args, &exception);
    if (!boxed || exception)
    {
        return false;
    }

    Vec3 screen = unbox<Vec3>(boxed);
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    out = {screen.x, display.y - screen.y};
    in_front = screen.z > 0.01f;
    if (!in_front)
    {
        out = {display.x - out.x, display.y - out.y};
    }
    return std::isfinite(out.x) && std::isfinite(out.y);
}

bool EspPlayers::scan_players()
{
    local_ = nullptr;
    if (local_instance_)
    {
        il2cpp::read_static_field(local_instance_, &local_);
    }
    if (!alive(local_))
        return false;
    scan_outlines(local_);
    camera_fov_ = 0.0f;
    camera_ = active_camera();
    if (!camera_)
        return false;
    if (get_field_of_view_)
    {
        il2cpp::Object* exception = nullptr;
        il2cpp::Object* boxed = il2cpp::runtime_invoke(get_field_of_view_, camera_, nullptr, &exception);
        if (boxed && !exception)
            camera_fov_ = unbox<float>(boxed);
    }

    il2cpp::Array* players = il2cpp::objects_of_type(game().player_controller());
    if (!players)
    {
        return false;
    }

    count_ = 0;
    const int total = static_cast<int>(players->size());
    for (int i = 0; i < total && count_ < max_players; ++i)
    {
        il2cpp::Object* player = players->at(i);
        if (!alive(player) || player == local_)
        {
            continue;
        }

        OutlineEntry* outline = outline_for(player);
        players_[count_].object = player;
        players_[count_].material = outline ? outline->material : nullptr;
        players_[count_].name = outline ? outline->name : std::string{};
        players_[count_].team = outline ? outline->team : -1;
        ++count_;
    }
    return true;
}

void EspPlayers::update_players()
{
    const Vec3 local_center = position(local_);
    for (int i = 0; i < count_; ++i)
    {
        EspPlayer& player = players_[i];
        player.visible = false;
        player.projected = false;
        if (!alive(player.object))
        {
            continue;
        }

        player.center = position(player.object);
        const float dx = player.center.x - local_center.x;
        const float dy = player.center.y - local_center.y;
        const float dz = player.center.z - local_center.z;
        player.distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        Vec3 head = {player.center.x, player.center.y + 1.05f, player.center.z};
        Vec3 feet = {player.center.x, player.center.y - 1.05f, player.center.z};
        bool head_front = false;
        bool feet_front = false;
        bool center_front = false;
        const bool head_projected = world_to_screen(camera_, head, player.head, head_front);
        const bool feet_projected = world_to_screen(camera_, feet, player.feet, feet_front);
        const bool center_projected = world_to_screen(camera_, player.center, player.screen_center, center_front);
        player.projected = head_projected && feet_projected && center_projected;
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const float height = player.feet.y - player.head.y;
        const float half_width = std::max(10.0f, height) * 0.225f;
        const float center_x = (player.head.x + player.feet.x) * 0.5f;
        player.visible = player.projected && head_front && feet_front && center_front &&
                         player.feet.y > player.head.y && center_x + half_width >= 0.0f &&
                         center_x - half_width <= display.x && player.feet.y >= 0.0f && player.head.y <= display.y;
    }
}

EspPlayers& esp_players()
{
    static EspPlayers instance;
    return instance;
}
} // namespace hackmatch
