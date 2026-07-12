# Compact decompiler reconstructions

These excerpts retain only data flow used by production features. Names are descriptive where metadata did not provide one; omitted branches remain part of the original functions.

## Update and FixedUpdate

```c
void PlayerController_Update(PlayerController* self) {
    if (!self->identity->IsOwner()) {
        self->spread_168 = ComputeRemoteSpread(self, clamp(self->measuredSpeed_280, 0, 5));
        return;
    }

    self->camera_40->fieldOfView = Lerp(self->camera_40->fieldOfView, GameFovTarget(self), deltaTime * 15);
    Vector3 input = ReadNormalizedMovementInput();
    Vector3 target = input * NetworkMovementScale() * ReadMovementStat(self, self->sprint_164);
    self->movement_11C = SmoothDamp(self->movement_11C, target, &self->movementVelocity_128, self->movementSmooth_E8);
    ProcessSprintAdsInventoryWeaponsAndInteraction(self);
}

void PlayerController_FixedUpdate(PlayerController* self) {
    Vector3 position = self->transform->position;
    self->measuredSpeed_280 = magnitude(position - self->previousPosition_274) / fixedDeltaTime;
    self->previousPosition_274 = position;
    if (!self->identity->IsOwner()) return;

    Vector3 destination = self->rigidbody_180->position
        + self->transform->TransformDirection(self->movement_11C) * fixedDeltaTime
        + self->impulse_21C * fixedDeltaTime + self->contactDelta_154;
    if (!Physics_Raycast(position, direction(position, destination), distance(position, destination), self->collisionMask_18))
        self->rigidbody_180->MovePosition(destination);
}
```

## Spread and crosshair

```c
void ComputeWeaponSpread(PlayerController* self) {
    float speed = clamp(magnitude(self->rigidbody_180->velocity), 0, 5);
    Item* item = checked_get(self->items_60, self->selectedItem_108);
    ItemInfo* info = item->info_18;
    float base = self->ads_161 ? info->adsBase_6C : info->hipBase_68;
    float multiplier = self->ads_161 ? info->adsMultiplier_74 : info->hipMultiplier_70;
    self->spread_168 = (speed + base + item->spread_30) * multiplier;
}

void UpdateCrosshairSpreadLayout(PlayerController* self) {
    if (self->ads_161) return;
    uint32_t style = self->crosshairStyle_208;
    float distance = self->spread_168 + 2.5f;
    set_x(checked_get(self->crosshairLeft_98, style), -distance);
    set_x(checked_get(self->crosshairRight_A0, style), distance);
    set_y(checked_get(self->crosshairUp_A8, style), distance);
    set_y(checked_get(self->crosshairDown_B0, style), -distance);
}
```

## Primary-shot scope

```c
void FirePrimaryShot(PlayerController* self, ShotInfo* shot) {
    Item* item = checked_get(self->items_60, self->selectedItem_108);
    ApplyAnimationAudioAndSpread(self, item, shot);
    for (int pellet = 0; pellet < shot->pelletCount_4C; ++pellet) {
        Ray ray = BuildCameraRayWithSpread(self->camera_40, item, shot);
        RaycastHit[] hits = Physics_RaycastAll(ray.origin, ray.direction, item->info_18->mask_B0);
        SortFilterAndAggregateHits(hits);
    }
    PreserveOriginalNetworkAmmoRecoilAndFeedback(self, item, shot);
}
```

## Local shield pair

```c
void HidePlayerStatusUI(PlayerController* self) {
    require(self->identity_188);
    MaybeFadeOwnedCanvas();
    self->shieldState_10C = false;
    require(self->shieldObject_38)->SetActive(false);
}
```

The production feature intentionally mirrors only the final two local writes and restores their exact pre-enable values.
