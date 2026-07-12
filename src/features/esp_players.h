#pragma once

#include "il2cpp_api.h"

#include <imgui.h>

#include <mutex>
#include <string>

namespace hackmatch
{
struct Vec3
{
    float x;
    float y;
    float z;
};

struct EspPlayer
{
    il2cpp::Object* object = nullptr;
    il2cpp::Object* material = nullptr;
    std::string name;
    Vec3 center{};
    ImVec2 head{};
    ImVec2 feet{};
    ImVec2 screen_center{};
    float distance = 0.0f;
    int team = -1;
    bool visible = false;
    bool projected = false;
};

class EspPlayers
{
  public:
    bool refresh();
    void reset();

    int count() const;
    const EspPlayer& at(int index) const;
    bool is_team(const EspPlayer& player) const;
    bool is_teammate(il2cpp::Object* player) const;
    bool camera_fov(float& value) const;

  private:
    struct OutlineEntry
    {
        il2cpp::Object* player = nullptr;
        il2cpp::Object* identity = nullptr;
        il2cpp::Object* player_data = nullptr;
        il2cpp::Object* material = nullptr;
        std::string name;
        int team = -1;
    };

    struct Il2CppString : il2cpp::Object
    {
        int length;
        wchar_t chars[1];
    };

    struct TeamEntry
    {
        il2cpp::Object* material = nullptr;
        int team = -1;
    };

    bool resolve();
    bool alive(il2cpp::Object* object);
    std::string utf8(Il2CppString* string);
    il2cpp::Object* player_data_from_identity(il2cpp::Object* identity);
    std::string name_from_identity(il2cpp::Object* identity);
    int team_from_material(il2cpp::Object* material) const;
    void scan_team_info();
    void scan_outlines(il2cpp::Object* local);
    OutlineEntry* outline_for(il2cpp::Object* player);
    Vec3 position(il2cpp::Object* object);
    il2cpp::Object* active_camera();
    bool world_to_screen(il2cpp::Object* camera, const Vec3& world, ImVec2& out, bool& in_front);
    bool scan_players();
    void update_players();
    void publish_relationships();

    static constexpr int max_players = 64;
    static constexpr int max_outline_entries = 128;
    static constexpr int max_team_entries = 16;

    EspPlayer players_[max_players]{};
    OutlineEntry outline_entries_[max_outline_entries]{};
    OutlineEntry relationship_entries_[max_outline_entries]{};
    TeamEntry team_entries_[max_team_entries]{};
    int count_ = 0;
    int outline_count_ = 0;
    int relationship_count_ = 0;
    int team_count_ = 0;
    int local_team_ = -1;
    int relationship_local_team_ = -1;
    il2cpp::Object* local_material_ = nullptr;
    il2cpp::Object* relationship_local_material_ = nullptr;
    il2cpp::Object* local_ = nullptr;
    il2cpp::Object* camera_ = nullptr;
    float camera_fov_ = 0.0f;

    MethodInfo* get_transform_ = nullptr;
    MethodInfo* get_position_ = nullptr;
    MethodInfo* get_main_camera_ = nullptr;
    MethodInfo* get_current_camera_ = nullptr;
    MethodInfo* world_to_screen_ = nullptr;
    MethodInfo* get_field_of_view_ = nullptr;
    MethodInfo* get_shared_material_ = nullptr;
    MethodInfo* object_equals_ = nullptr;
    il2cpp::Class* player_team_outline_class_ = nullptr;
    il2cpp::Class* team_info_manager_class_ = nullptr;
    il2cpp::FieldInfo* local_instance_ = nullptr;
    il2cpp::FieldInfo* player_camera_ = nullptr;
    il2cpp::FieldInfo* outline_player_ = nullptr;
    il2cpp::FieldInfo* outline_identity_ = nullptr;
    il2cpp::FieldInfo* outline_renderer_ = nullptr;
    il2cpp::FieldInfo* team_info_array_ = nullptr;
    il2cpp::FieldInfo* team_id_ = nullptr;
    il2cpp::FieldInfo* team_outline_material_ = nullptr;
    mutable std::mutex relationship_mutex_;
};

EspPlayers& esp_players();
} // namespace hackmatch
