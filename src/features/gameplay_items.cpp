#include "gameplay_items.h"

#include "game.h"

namespace hackmatch
{
namespace
{
constexpr int damage_rifle = 1;

template <class T> bool read(il2cpp::Object* object, il2cpp::FieldInfo* field, T& value)
{
    return il2cpp::read_field(object, field, &value);
}

template <class T> void write(il2cpp::Object* object, il2cpp::FieldInfo* field, const T& value)
{
    il2cpp::write_field(object, field, &value);
}
} // namespace

bool GameplayItems::resolve()
{
    if (local_instance_ && player_items_ && item_info_ && object_equals_)
    {
        return true;
    }

    item_class_ = il2cpp::klass("Assembly-CSharp", "", "Item");
    object_equals_ = il2cpp::method("UnityEngine.CoreModule", "UnityEngine", "Object", "op_Equality", 2);
    local_instance_ = il2cpp::field(game().player_controller(), "LocalInstance");
    player_items_ = il2cpp::field(game().player_controller(), "items");
    player_bullet_spread_ = il2cpp::field(game().player_controller(), "bulletSpread");
    item_info_ = il2cpp::field(item_class_, "info");
    item_bullet_spread_ = il2cpp::field(item_class_, "bulletSpread");
    item_ammo_ = il2cpp::field(item_class_, "ammo");
    item_total_ammo_ = il2cpp::field(item_class_, "totalAmmo");
    return local_instance_ && player_items_ && item_info_ && object_equals_;
}

bool GameplayItems::alive(il2cpp::Object* object)
{
    if (!object || !object_equals_)
        return false;
    void* args[] = {object, nullptr};
    il2cpp::Object* exception = nullptr;
    il2cpp::Object* boxed = il2cpp::runtime_invoke(object_equals_, nullptr, args, &exception);
    return boxed && !exception && !*reinterpret_cast<bool*>(reinterpret_cast<char*>(boxed) + sizeof(il2cpp::Object));
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
    if (!item)
    {
        return false;
    }

    il2cpp::Object* info = nullptr;
    if (!read(item, item_info_, info) || !info || !resolve_info_fields(info))
    {
        return false;
    }

    out = {item, info};
    return true;
}

void GameplayItems::begin_session(il2cpp::Object* local)
{
    if (local != session_local_)
    {
        (void)restore_spread(alive(session_local_) ? session_local_ : nullptr);
        restore_reload();
        restore_camera_shake();
        restore_rapid_fire();
        restore_ammo();
        discard_all();
        session_local_ = local;
    }
}

bool GameplayItems::resolve_info_fields(il2cpp::Object* info)
{
    if (!info)
    {
        return false;
    }
    if (info_use_delay_ && info_primary_shot_)
    {
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
    return info_use_delay_ && info_primary_shot_;
}

bool GameplayItems::resolve_shot_fields(il2cpp::Object* shot)
{
    if (!shot)
    {
        return false;
    }
    if (shot_bullet_spread_)
    {
        return true;
    }

    shot_bullet_spread_ = il2cpp::field(shot->klass, "bulletSpread");
    shot_ads_bullet_spread_ = il2cpp::field(shot->klass, "adsBulletSpread");
    shot_camera_shake_ = il2cpp::field(shot->klass, "cameraShake");
    shot_ads_camera_shake_ = il2cpp::field(shot->klass, "adsCameraShake");
    return shot_bullet_spread_;
}

GameplayItems::ShotOriginal* GameplayItems::shot_original_for(il2cpp::Object* shot)
{
    for (int i = 0; i < shot_original_count_; ++i)
    {
        if (shot_originals_[i].shot == shot)
            return &shot_originals_[i];
    }
    if (!resolve_shot_fields(shot) || shot_original_count_ >= max_originals)
        return nullptr;
    ShotOriginal& original = shot_originals_[shot_original_count_++];
    original = {};
    original.shot = shot;
    return &original;
}

void GameplayItems::patch_shot_spread(il2cpp::Object* shot)
{
    ShotOriginal* original = shot_original_for(shot);
    if (!original)
        return;
    if (!original->spread_saved)
    {
        original->spread_saved = read(shot, shot_bullet_spread_, original->bullet_spread) &&
                                 read(shot, shot_ads_bullet_spread_, original->ads_bullet_spread);
    }
    if (!original->spread_saved)
        return;
    const float zero = 0.0f;
    write(shot, shot_bullet_spread_, zero);
    write(shot, shot_ads_bullet_spread_, zero);
    original->spread_patched = true;
}

void GameplayItems::patch_shot_shake(il2cpp::Object* shot)
{
    ShotOriginal* original = shot_original_for(shot);
    if (!original)
        return;
    if (!original->shake_saved)
    {
        original->shake_saved = read(shot, shot_camera_shake_, original->camera_shake) &&
                                read(shot, shot_ads_camera_shake_, original->ads_camera_shake);
    }
    if (!original->shake_saved)
        return;
    const float zero = 0.0f;
    write(shot, shot_camera_shake_, zero);
    write(shot, shot_ads_camera_shake_, zero);
    original->shake_patched = true;
}

void GameplayItems::restore_shot_spread(const ShotOriginal& original)
{
    if (!original.shot || !resolve_shot_fields(original.shot))
    {
        return;
    }

    write(original.shot, shot_bullet_spread_, original.bullet_spread);
    write(original.shot, shot_ads_bullet_spread_, original.ads_bullet_spread);
}

void GameplayItems::restore_shot_shake(const ShotOriginal& original)
{
    if (!original.shot || !resolve_shot_fields(original.shot))
    {
        return;
    }

    write(original.shot, shot_camera_shake_, original.camera_shake);
    write(original.shot, shot_ads_camera_shake_, original.ads_camera_shake);
}

GameplayItems::InfoOriginal* GameplayItems::original_for(il2cpp::Object* info)
{
    for (int i = 0; i < original_count_; ++i)
    {
        if (originals_[i].info == info)
        {
            return &originals_[i];
        }
    }
    if (!info || original_count_ >= max_originals || !resolve_info_fields(info))
    {
        return nullptr;
    }

    InfoOriginal& original = originals_[original_count_++];
    original = {};
    original.info = info;
    return &original;
}

bool GameplayItems::restore_spread(il2cpp::Object* local)
{
    bool restored = player_spread_saved_ || item_spread_original_count_ > 0;
    if (player_spread_saved_ && local && player_bullet_spread_)
    {
        write(local, player_bullet_spread_, player_spread_original_);
    }
    for (int i = 0; i < item_spread_original_count_; ++i)
    {
        if (alive(item_spread_originals_[i].item) && item_bullet_spread_)
        {
            write(item_spread_originals_[i].item, item_bullet_spread_, item_spread_originals_[i].spread);
        }
    }
    for (int i = 0; i < original_count_; ++i)
    {
        InfoOriginal& original = originals_[i];
        restored = restored || original.spread_patched;
        if (!original.spread_patched || !original.info || !resolve_info_fields(original.info))
            continue;
        write(original.info, info_max_bullet_spread_, original.max_bullet_spread);
        write(original.info, info_bullet_spread_decrease_, original.bullet_spread_decrease);
        write(original.info, info_normal_spread_, original.normal_spread);
        write(original.info, info_ads_spread_, original.ads_spread);
        write(original.info, info_movement_spread_multiplier_, original.movement_spread_multiplier);
        write(original.info, info_ads_movement_spread_multiplier_, original.ads_movement_spread_multiplier);
        original.spread_patched = false;
        original.spread_saved = false;
    }
    for (int i = 0; i < shot_original_count_; ++i)
    {
        ShotOriginal& original = shot_originals_[i];
        restored = restored || original.spread_patched;
        if (original.spread_patched)
            restore_shot_spread(original);
        original.spread_patched = false;
        original.spread_saved = false;
    }
    item_spread_original_count_ = 0;
    player_spread_saved_ = false;
    return restored;
}

void GameplayItems::restore_reload()
{
    for (int i = 0; i < original_count_; ++i)
    {
        InfoOriginal& original = originals_[i];
        if (original.reload_patched && original.info && resolve_info_fields(original.info))
        {
            write(original.info, info_reload_time_, original.reload_time);
            original.reload_patched = false;
            original.reload_saved = false;
        }
    }
}

void GameplayItems::restore_camera_shake()
{
    for (int i = 0; i < original_count_; ++i)
    {
        InfoOriginal& original = originals_[i];
        if (!original.shake_patched || !original.info || !resolve_info_fields(original.info))
            continue;
        write(original.info, info_camera_ads_bob_multiplier_, original.camera_ads_bob_multiplier);
        original.shake_patched = false;
        original.shake_saved = false;
    }
    for (int i = 0; i < shot_original_count_; ++i)
    {
        ShotOriginal& original = shot_originals_[i];
        if (original.shake_patched)
            restore_shot_shake(original);
        original.shake_patched = false;
        original.shake_saved = false;
    }
}

void GameplayItems::restore_rapid_fire()
{
    for (int i = 0; i < original_count_; ++i)
    {
        InfoOriginal& original = originals_[i];
        if (original.rapid_fire_patched && original.info && resolve_info_fields(original.info))
        {
            write(original.info, info_use_delay_, original.use_delay);
            original.rapid_fire_patched = false;
            original.rapid_fire_saved = false;
        }
    }
}

void GameplayItems::restore_ammo()
{
    for (int i = 0; i < ammo_original_count_; ++i)
    {
        const AmmoOriginal& original = ammo_originals_[i];
        if (!alive(original.item))
            continue;
        write(original.item, item_ammo_, original.ammo);
        write(original.item, item_total_ammo_, original.total_ammo);
    }
    ammo_original_count_ = 0;
}

bool GameplayItems::restore_disabled(const WeaponSettings& settings, il2cpp::Object* local)
{
    const bool spread_restored = !settings.no_spread && restore_spread(local);
    if (!settings.instant_reload)
        restore_reload();
    if (!settings.no_camera_shake)
        restore_camera_shake();
    if (!settings.rapid_fire)
        restore_rapid_fire();
    if (!settings.infinite_ammo)
        restore_ammo();
    return spread_restored;
}

void GameplayItems::restore_all()
{
    (void)restore_spread(session_local_);
    restore_reload();
    restore_camera_shake();
    restore_rapid_fire();
    restore_ammo();
    discard_all();
}

void GameplayItems::discard_all()
{
    original_count_ = 0;
    shot_original_count_ = 0;
    item_spread_original_count_ = 0;
    ammo_original_count_ = 0;
    player_spread_saved_ = false;
}

void GameplayItems::zero_player_spread(il2cpp::Object* local)
{
    if (player_bullet_spread_)
    {
        if (!player_spread_saved_)
        {
            player_spread_saved_ = read(local, player_bullet_spread_, player_spread_original_);
        }
        if (!player_spread_saved_)
            return;
        const float zero = 0.0f;
        write(local, player_bullet_spread_, zero);
    }
}

void GameplayItems::zero_spread(const GameplayItem& item)
{
    InfoOriginal* info_original = original_for(item.info);
    if (!info_original)
        return;
    if (!info_original->spread_saved)
    {
        info_original->spread_saved =
            read(item.info, info_max_bullet_spread_, info_original->max_bullet_spread) &&
            read(item.info, info_bullet_spread_decrease_, info_original->bullet_spread_decrease) &&
            read(item.info, info_normal_spread_, info_original->normal_spread) &&
            read(item.info, info_ads_spread_, info_original->ads_spread) &&
            read(item.info, info_movement_spread_multiplier_, info_original->movement_spread_multiplier) &&
            read(item.info, info_ads_movement_spread_multiplier_, info_original->ads_movement_spread_multiplier);
    }
    if (!info_original->spread_saved)
        return;
    const float zero = 0.0f;
    bool saved = false;
    for (int i = 0; i < item_spread_original_count_; ++i)
    {
        saved = item_spread_originals_[i].item == item.item;
        if (saved)
            break;
    }
    if (!saved && item_spread_original_count_ < max_originals)
    {
        ItemSpreadOriginal original{item.item};
        if (read(item.item, item_bullet_spread_, original.spread))
        {
            item_spread_originals_[item_spread_original_count_++] = original;
            saved = true;
        }
    }
    if (saved)
        write(item.item, item_bullet_spread_, zero);
    write(item.info, info_max_bullet_spread_, zero);
    write(item.info, info_bullet_spread_decrease_, zero);
    write(item.info, info_normal_spread_, zero);
    write(item.info, info_ads_spread_, zero);
    write(item.info, info_movement_spread_multiplier_, zero);
    write(item.info, info_ads_movement_spread_multiplier_, zero);
    info_original->spread_patched = true;

    il2cpp::Object* primary = nullptr;
    il2cpp::Object* secondary = nullptr;
    read(item.info, info_primary_shot_, primary);
    read(item.info, info_secondary_shot_, secondary);
    il2cpp::Object* shots[] = {primary, secondary};
    for (il2cpp::Object* shot : shots)
    {
        patch_shot_spread(shot);
    }
}

void GameplayItems::infinite_ammo(const GameplayItem& item)
{
    if (!item_ammo_)
        item_ammo_ = il2cpp::field(item.item->klass, "ammo");
    if (!item_total_ammo_)
        item_total_ammo_ = il2cpp::field(item.item->klass, "totalAmmo");
    if (!item_ammo_ || !item_total_ammo_)
        return;
    bool saved = false;
    for (int i = 0; i < ammo_original_count_; ++i)
    {
        if (ammo_originals_[i].item == item.item)
        {
            saved = true;
            break;
        }
    }
    if (!saved && ammo_original_count_ < max_originals)
    {
        AmmoOriginal original{};
        original.item = item.item;
        if (read(item.item, item_ammo_, original.ammo) && read(item.item, item_total_ammo_, original.total_ammo))
        {
            ammo_originals_[ammo_original_count_++] = original;
            saved = true;
        }
    }
    if (!saved)
        return;
    const Ammo unlimited{0, 9999};
    write(item.item, item_ammo_, unlimited);
    write(item.item, item_total_ammo_, unlimited);
}

void GameplayItems::instant_reload(const GameplayItem& item, float reload_time)
{
    InfoOriginal* original = original_for(item.info);
    if (!original)
        return;
    if (!original->reload_saved)
        original->reload_saved = read(item.info, info_reload_time_, original->reload_time);
    if (!original->reload_saved)
        return;
    original->reload_patched = true;
    write(item.info, info_reload_time_, reload_time);
}

void GameplayItems::zero_camera_shake(const GameplayItem& item)
{
    InfoOriginal* original = original_for(item.info);
    if (!original)
        return;
    if (!original->shake_saved)
    {
        original->shake_saved = read(item.info, info_camera_ads_bob_multiplier_, original->camera_ads_bob_multiplier);
    }
    if (!original->shake_saved)
        return;
    original->shake_patched = true;
    const float zero = 0.0f;
    write(item.info, info_camera_ads_bob_multiplier_, zero);

    il2cpp::Object* primary = nullptr;
    il2cpp::Object* secondary = nullptr;
    read(item.info, info_primary_shot_, primary);
    read(item.info, info_secondary_shot_, secondary);
    il2cpp::Object* shots[] = {primary, secondary};
    for (il2cpp::Object* shot : shots)
    {
        patch_shot_shake(shot);
    }
}

void GameplayItems::rapid_fire(const GameplayItem& item)
{
    InfoOriginal* original = original_for(item.info);
    if (!original)
        return;
    if (!original->rapid_fire_saved)
        original->rapid_fire_saved = read(item.info, info_use_delay_, original->use_delay);
    if (!original->rapid_fire_saved)
        return;
    original->rapid_fire_patched = true;
    int damage_data = 0;
    read(item.info, info_damage_data_, damage_data);
    const float delay = damage_data == damage_rifle ? 0.05f : 0.3f;
    write(item.info, info_use_delay_, delay);
}

GameplayItems& gameplay_items()
{
    static GameplayItems instance;
    return instance;
}
} // namespace hackmatch
