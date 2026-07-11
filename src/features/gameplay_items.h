#pragma once

#include "il2cpp_api.h"

namespace hackmatch {
struct GameplayItem {
    il2cpp::Object* item = nullptr;
    il2cpp::Object* info = nullptr;
};

class GameplayItems {
public:
    bool resolve();
    il2cpp::Object* local_player();
    il2cpp::Array* local_items(il2cpp::Object* local);

    void begin_session(il2cpp::Object* local);
    bool prepare(il2cpp::Object* item, GameplayItem& out);
    void restore_all();

    void zero_player_spread(il2cpp::Object* local);
    void zero_spread(const GameplayItem& item);
    void infinite_ammo(const GameplayItem& item);
    void instant_reload(const GameplayItem& item);
    void zero_camera_shake(const GameplayItem& item);
    void rapid_fire(const GameplayItem& item);
    void custom_damage(const GameplayItem& item);

private:
    struct Ammo {
        int offset;
        int value;
    };

    struct ShotOriginal {
        il2cpp::Object* shot = nullptr;
        float bullet_spread = 0.0f;
        float ads_bullet_spread = 0.0f;
        float camera_shake = 0.0f;
        float ads_camera_shake = 0.0f;
    };

    struct InfoOriginal {
        il2cpp::Object* info = nullptr;
        il2cpp::Object* primary = nullptr;
        il2cpp::Object* secondary = nullptr;
        float use_delay = 0.0f;
        float reload_time = 0.0f;
        float max_bullet_spread = 0.0f;
        float bullet_spread_decrease = 0.0f;
        float normal_spread = 0.0f;
        float ads_spread = 0.0f;
        float movement_spread_multiplier = 0.0f;
        float ads_movement_spread_multiplier = 0.0f;
        float camera_ads_bob_multiplier = 0.0f;
        int damage_lobby_data = 0;
        ShotOriginal primary_original{};
        ShotOriginal secondary_original{};
    };

    struct ItemSpreadOriginal {
        il2cpp::Object* item = nullptr;
        float spread = 0.0f;
    };

    bool resolve_info_fields(il2cpp::Object* info);
    bool resolve_shot_fields(il2cpp::Object* shot);
    void save_shot(ShotOriginal& original, il2cpp::Object* shot);
    void restore_shot(const ShotOriginal& original);
    InfoOriginal* original_for(il2cpp::Object* info);
    void restore_info(const InfoOriginal& original);
    void discard_all();

    static constexpr int max_originals = 128;
    InfoOriginal originals_[max_originals]{};
    int original_count_ = 0;
    ItemSpreadOriginal item_spread_originals_[max_originals]{};
    int item_spread_original_count_ = 0;
    il2cpp::Object* session_local_ = nullptr;
    float player_spread_original_ = 0.0f;
    bool player_spread_saved_ = false;

    il2cpp::Class* item_class_ = nullptr;
    il2cpp::FieldInfo* local_instance_ = nullptr;
    il2cpp::FieldInfo* player_items_ = nullptr;
    il2cpp::FieldInfo* player_bullet_spread_ = nullptr;
    il2cpp::FieldInfo* item_info_ = nullptr;
    il2cpp::FieldInfo* item_bullet_spread_ = nullptr;
    il2cpp::FieldInfo* item_ammo_ = nullptr;
    il2cpp::FieldInfo* item_total_ammo_ = nullptr;
    il2cpp::FieldInfo* info_use_delay_ = nullptr;
    il2cpp::FieldInfo* info_reload_time_ = nullptr;
    il2cpp::FieldInfo* info_primary_shot_ = nullptr;
    il2cpp::FieldInfo* info_secondary_shot_ = nullptr;
    il2cpp::FieldInfo* info_max_bullet_spread_ = nullptr;
    il2cpp::FieldInfo* info_bullet_spread_decrease_ = nullptr;
    il2cpp::FieldInfo* info_normal_spread_ = nullptr;
    il2cpp::FieldInfo* info_ads_spread_ = nullptr;
    il2cpp::FieldInfo* info_movement_spread_multiplier_ = nullptr;
    il2cpp::FieldInfo* info_ads_movement_spread_multiplier_ = nullptr;
    il2cpp::FieldInfo* info_camera_ads_bob_multiplier_ = nullptr;
    il2cpp::FieldInfo* info_damage_data_ = nullptr;
    il2cpp::FieldInfo* info_damage_lobby_data_ = nullptr;
    il2cpp::FieldInfo* shot_bullet_spread_ = nullptr;
    il2cpp::FieldInfo* shot_ads_bullet_spread_ = nullptr;
    il2cpp::FieldInfo* shot_camera_shake_ = nullptr;
    il2cpp::FieldInfo* shot_ads_camera_shake_ = nullptr;
};

GameplayItems& gameplay_items();
}
