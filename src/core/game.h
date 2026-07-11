#pragma once

#include "il2cpp_api.h"

namespace hackmatch {
class Game {
public:
    bool init();
    bool ready() const;
    const char* last_error() const;

    il2cpp::Class* player_controller() const;
    MethodInfo* player_update() const;

private:
    il2cpp::Class* player_controller_ = nullptr;
    MethodInfo* player_update_ = nullptr;
    bool ready_ = false;
    const char* last_error_ = "not initialized";
};

Game& game();
}
