#include "game.h"

namespace hackmatch {
bool Game::init()
{
    if (ready_) {
        return true;
    }

    if (!il2cpp::ready()) {
        last_error_ = "il2cpp not ready";
        return false;
    }

    player_controller_ = il2cpp::klass("Assembly-CSharp", "", "PlayerController");
    if (!player_controller_) {
        last_error_ = "PlayerController not found";
        return false;
    }

    player_update_ = il2cpp::method(player_controller_, "Update", 0);
    if (!player_update_) {
        last_error_ = "PlayerController.Update not found";
        return false;
    }

    ready_ = true;
    last_error_ = "ok";
    return true;
}

bool Game::ready() const
{
    return ready_;
}

const char* Game::last_error() const
{
    return last_error_;
}

il2cpp::Class* Game::player_controller() const
{
    return player_controller_;
}

MethodInfo* Game::player_update() const
{
    return player_update_;
}

Game& game()
{
    static Game instance;
    return instance;
}
}
