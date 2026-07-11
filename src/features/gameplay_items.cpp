#include "gameplay_items.h"

#include "game.h"

namespace hackmatch {
namespace {
constexpr int damage_rifle = 1;
constexpr int damage_lobby_sniper = 0x21;

template <class T>
bool read(il2cpp::Object* object, il2cpp::FieldInfo* field, T& value)
{
    return il2cpp::read_field(object, field, &value);
}

template <class T>
void write(il2cpp::Object* object, il2cpp::FieldInfo* field, const T& value)
{
    il2cpp::write_field(object, field, &value);
}
}

bool GameplayItems::resolve()
{
    if (local_instance_ && player_items_ && item_info_) {
        return true;
    }

    item_class_ = il2cpp::klass("Assembly-CSharp", "", "Item");
    local_instance_ = il2cpp::field(game().player_controller(), "LocalInstance");
    player_items_ = il2cpp::field(game().player_controller(), "items");
    player_bullet_spread_ = il2cpp::field(game().player_controller(), "bulletSpread");
    item_info_ = il2cpp::field(item_class_, "info");
    item_bullet_spread_ = il2cpp::field(item_class_, "bulletSpread");
    item_ammo_ = il2cpp::field(item_class_, "ammo");
    item_total_ammo_ = il2cpp::field(item_class_, "totalAmmo");
    return local_instance_ && player_items_ && item_info_;
}

il2cpp::Object* GameplayItems::local_player()
{
    il2cpp::Object* local = nullptr;
    return il2cpp::read_static_field(local_instance_, &local) ? local : nullptr;
}

il2cpp::Array* GameplayItems::local_items(il2cpp::Object* local)
{
    il2cpp::Array* items = nullptr;
    return read(local, player_items_, items) ? items : nullptr;
}

bool GameplayItems::prepare(il2cpp::Object* item, GameplayItem& out)
{
    if (!item) {
        return false;
    }

    il2cpp::Object* info = nullptr;
    if (!read(item, item_info_, info) || !info || !resolve_info_fields(info)) {
        return false;
    }

    InfoOriginal* original = original_for(info);
    if (!original) {
        return false;
    }

    il2cpp::Object* primary = nullptr;
    il2cpp::Object* secondary = nullptr;
    read(info, info_primary_shot_, primary);
    read(info, info_secondary_shot_, secondary);
    if (primary != original->primary_original.shot) save_shot(original->primary_original, primary);
    if (secondary != original->secondary_original.shot) save_shot(original->secondary_original, secondary);
    out = {item, info};
    return true;
}

void GameplayItems::begin_session(il2cpp::Object* local)
{
    if (local != session_local_) {
        discard_all();
        session_local_ = local;
    }
}

bool GameplayItems::resolve_info_fields(il2cpp::Object* info)
{
    if (!info) {
        return false;
    }
    if (info_use_delay_ && info_primary_shot_) {
        return true;
    }

    info_use_delay_ = il2cpp::field(info->klass, "useDelay");
    info_reload_time_ = il2cpp::field(info->klass, "reloadTime");
    info_primary_shot_ = il2cpp::field(info->klass, "primaryShotInfo");
    info_secondary_shot_ = il2cpp::field(info->klass, "secondaryShotInfo");
    info_max_bullet_spread_ = il2cpp::field(info->klass, "maxBulletSpread");
    info_bullet_spread_decrease_ = il2cpp::field(info->klass, "bulletSpreadDecrease");
    info_normal_spread_ = il2cpp::field(info->klass, "normalSpread");
    info_ads_spread_ = il2cpp::field(info->klass, "adsSpread");
    info_movement_spread_multiplier_ = il2cpp::field(info->klass, "movementSpreadMultiplier");
    info_ads_movement_spread_multiplier_ = il2cpp::field(info->klass, "adsMovementSpreadMultiplier");
    info_camera_ads_bob_multiplier_ = il2cpp::field(info->klass, "cameraADSBobMultiplier");
    info_damage_data_ = il2cpp::field(info->klass, "damageData");
    info_damage_lobby_data_ = il2cpp::field(info->klass, "damageLobbyData");
    return info_use_delay_ && info_primary_shot_;
}

bool GameplayItems::resolve_shot_fields(il2cpp::Object* shot)
{
    if (!shot) {
        return false;
    }
    if (shot_bullet_spread_) {
        return true;
    }

    shot_bullet_spread_ = il2cpp::field(shot->klass, "bulletSpread");
    shot_ads_bullet_spread_ = il2cpp::field(shot->klass, "adsBulletSpread");
    shot_camera_shake_ = il2cpp::field(shot->klass, "cameraShake");
    shot_ads_camera_shake_ = il2cpp::field(shot->klass, "adsCameraShake");
    return shot_bullet_spread_;
}

void GameplayItems::save_shot(ShotOriginal& original, il2cpp::Object* shot)
{
    original.shot = shot;
    if (!resolve_shot_fields(shot)) {
        return;
    }

    read(shot, shot_bullet_spread_, original.bullet_spread);
    read(shot, shot_ads_bullet_spread_, original.ads_bullet_spread);
    read(shot, shot_camera_shake_, original.camera_shake);
    read(shot, shot_ads_camera_shake_, original.ads_camera_shake);
}

void GameplayItems::restore_shot(const ShotOriginal& original)
{
    if (!original.shot || !resolve_shot_fields(original.shot)) {
        return;
    }

    write(original.shot, shot_bullet_spread_, original.bullet_spread);
    write(original.shot, shot_ads_bullet_spread_, original.ads_bullet_spread);
    write(original.shot, shot_camera_shake_, original.camera_shake);
    write(original.shot, shot_ads_camera_shake_, original.ads_camera_shake);
}

GameplayItems::InfoOriginal* GameplayItems::original_for(il2cpp::Object* info)
{
    for (int i = 0; i < original_count_; ++i) {
        if (originals_[i].info == info) {
            return &originals_[i];
        }
    }
    if (!info || original_count_ >= max_originals || !resolve_info_fields(info)) {
        return nullptr;
    }

    InfoOriginal& original = originals_[original_count_++];
    original = {};
    original.info = info;
    read(info, info_primary_shot_, original.primary);
    read(info, info_secondary_shot_, original.secondary);
    read(info, info_use_delay_, original.use_delay);
    read(info, info_reload_time_, original.reload_time);
    read(info, info_max_bullet_spread_, original.max_bullet_spread);
    read(info, info_bullet_spread_decrease_, original.bullet_spread_decrease);
    read(info, info_normal_spread_, original.normal_spread);
    read(info, info_ads_spread_, original.ads_spread);
    read(info, info_movement_spread_multiplier_, original.movement_spread_multiplier);
    read(info, info_ads_movement_spread_multiplier_, original.ads_movement_spread_multiplier);
    read(info, info_camera_ads_bob_multiplier_, original.camera_ads_bob_multiplier);
    read(info, info_damage_lobby_data_, original.damage_lobby_data);
    save_shot(original.primary_original, original.primary);
    save_shot(original.secondary_original, original.secondary);
    return &original;
}

void GameplayItems::restore_info(const InfoOriginal& original)
{
    il2cpp::Object* info = original.info;
    if (!info || !resolve_info_fields(info)) {
        return;
    }

    write(info, info_use_delay_, original.use_delay);
    write(info, info_reload_time_, original.reload_time);
    write(info, info_max_bullet_spread_, original.max_bullet_spread);
    write(info, info_bullet_spread_decrease_, original.bullet_spread_decrease);
    write(info, info_normal_spread_, original.normal_spread);
    write(info, info_ads_spread_, original.ads_spread);
    write(info, info_movement_spread_multiplier_, original.movement_spread_multiplier);
    write(info, info_ads_movement_spread_multiplier_, original.ads_movement_spread_multiplier);
    write(info, info_camera_ads_bob_multiplier_, original.camera_ads_bob_multiplier);
    write(info, info_damage_lobby_data_, original.damage_lobby_data);
    restore_shot(original.primary_original);
    restore_shot(original.secondary_original);
}

void GameplayItems::restore_all()
{
    if (player_spread_saved_ && session_local_ && player_bullet_spread_) {
        write(session_local_, player_bullet_spread_, player_spread_original_);
    }
    for (int i = 0; i < item_spread_original_count_; ++i) {
        if (item_spread_originals_[i].item && item_bullet_spread_) {
            write(item_spread_originals_[i].item, item_bullet_spread_, item_spread_originals_[i].spread);
        }
    }
    for (int i = 0; i < original_count_; ++i) {
        restore_info(originals_[i]);
    }
    discard_all();
}

void GameplayItems::discard_all()
{
    original_count_ = 0;
    item_spread_original_count_ = 0;
    player_spread_saved_ = false;
}

void GameplayItems::zero_player_spread(il2cpp::Object* local)
{
    if (player_bullet_spread_) {
        if (!player_spread_saved_) {
            player_spread_saved_ = read(local, player_bullet_spread_, player_spread_original_);
        }
        const float zero = 0.0f;
        write(local, player_bullet_spread_, zero);
    }
}

void GameplayItems::zero_spread(const GameplayItem& item)
{
    const float zero = 0.0f;
    bool saved = false;
    for (int i = 0; i < item_spread_original_count_; ++i) {
        saved = item_spread_originals_[i].item == item.item;
        if (saved) break;
    }
    if (!saved && item_spread_original_count_ < max_originals) {
        ItemSpreadOriginal original{item.item};
        if (read(item.item, item_bullet_spread_, original.spread)) {
            item_spread_originals_[item_spread_original_count_++] = original;
        }
    }
    write(item.item, item_bullet_spread_, zero);
    write(item.info, info_max_bullet_spread_, zero);
    write(item.info, info_bullet_spread_decrease_, zero);
    write(item.info, info_normal_spread_, zero);
    write(item.info, info_ads_spread_, zero);
    write(item.info, info_movement_spread_multiplier_, zero);
    write(item.info, info_ads_movement_spread_multiplier_, zero);

    il2cpp::Object* primary = nullptr;
    il2cpp::Object* secondary = nullptr;
    read(item.info, info_primary_shot_, primary);
    read(item.info, info_secondary_shot_, secondary);
    il2cpp::Object* shots[] = {primary, secondary};
    for (il2cpp::Object* shot : shots) {
        if (!resolve_shot_fields(shot)) {
            continue;
        }
        write(shot, shot_bullet_spread_, zero);
        write(shot, shot_ads_bullet_spread_, zero);
    }
}

void GameplayItems::infinite_ammo(const GameplayItem& item)
{
    const Ammo ammo{0, 9999};
    write(item.item, item_ammo_, ammo);
    write(item.item, item_total_ammo_, ammo);
}

void GameplayItems::instant_reload(const GameplayItem& item)
{
    const float zero = 0.0f;
    write(item.info, info_reload_time_, zero);
}

void GameplayItems::zero_camera_shake(const GameplayItem& item)
{
    const float zero = 0.0f;
    write(item.info, info_camera_ads_bob_multiplier_, zero);

    il2cpp::Object* primary = nullptr;
    il2cpp::Object* secondary = nullptr;
    read(item.info, info_primary_shot_, primary);
    read(item.info, info_secondary_shot_, secondary);
    il2cpp::Object* shots[] = {primary, secondary};
    for (il2cpp::Object* shot : shots) {
        if (!resolve_shot_fields(shot)) {
            continue;
        }
        write(shot, shot_camera_shake_, zero);
        write(shot, shot_ads_camera_shake_, zero);
    }
}

void GameplayItems::rapid_fire(const GameplayItem& item)
{
    int damage_data = 0;
    read(item.info, info_damage_data_, damage_data);
    const float delay = damage_data == damage_rifle ? 0.05f : 0.3f;
    write(item.info, info_use_delay_, delay);
}

void GameplayItems::custom_damage(const GameplayItem& item)
{
    write(item.info, info_damage_lobby_data_, damage_lobby_sniper);
}

GameplayItems& gameplay_items()
{
    static GameplayItems instance;
    return instance;
}
}
