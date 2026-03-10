#include <engine/input_system.h>

namespace engine {

void InputSystem::pollInput(const bool headless) {
    inputMask_ = 0;
    if (headless || replayPlaybackMode_) {
        return;
    }

    const Uint8* keyboard = SDL_GetKeyboardState(nullptr);
    if (keyboard[SDL_SCANCODE_A] || keyboard[SDL_SCANCODE_LEFT]) inputMask_ |= InputMoveLeft;
    if (keyboard[SDL_SCANCODE_D] || keyboard[SDL_SCANCODE_RIGHT]) inputMask_ |= InputMoveRight;
    if (keyboard[SDL_SCANCODE_W] || keyboard[SDL_SCANCODE_UP]) inputMask_ |= InputMoveUp;
    if (keyboard[SDL_SCANCODE_S] || keyboard[SDL_SCANCODE_DOWN]) inputMask_ |= InputMoveDown;
    if (keyboard[SDL_SCANCODE_LSHIFT] || keyboard[SDL_SCANCODE_RSHIFT]) inputMask_ |= InputDefensiveSpecial;
}

void InputSystem::processEvent(const SDL_Event& event) {
    if (event.type == SDL_CONTROLLERBUTTONDOWN && event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) {
        inputMask_ |= InputDefensiveSpecial;
    }

    if (!upgradeNavCallback_) {
        return;
    }

    if (event.type == SDL_KEYDOWN) {
        if (event.key.repeat != 0) {
            return;
        }
        if (event.key.keysym.sym == SDLK_u) {
            upgradeNavCallback_(UpgradeNavAction::ToggleScreen);
        }
        if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a) upgradeNavCallback_(UpgradeNavAction::MoveLeft);
        if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d) upgradeNavCallback_(UpgradeNavAction::MoveRight);
        if (event.key.keysym.sym == SDLK_1) upgradeNavCallback_(UpgradeNavAction::SelectSlot1);
        if (event.key.keysym.sym == SDLK_2) upgradeNavCallback_(UpgradeNavAction::SelectSlot2);
        if (event.key.keysym.sym == SDLK_3) upgradeNavCallback_(UpgradeNavAction::SelectSlot3);
        if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) upgradeNavCallback_(UpgradeNavAction::Confirm);
        if (event.key.keysym.sym == SDLK_x) upgradeNavCallback_(UpgradeNavAction::Reroll);
    }
    if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT) upgradeNavCallback_(UpgradeNavAction::MoveLeft);
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) upgradeNavCallback_(UpgradeNavAction::MoveRight);
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) upgradeNavCallback_(UpgradeNavAction::Confirm);
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_X) upgradeNavCallback_(UpgradeNavAction::Reroll);
    }
}

std::uint32_t InputSystem::inputMask() const { return inputMask_; }

void InputSystem::setReplayPlayback(ReplayPlayer* player) {
    replayPlayer_ = player;
    replayPlaybackMode_ = replayPlayer_ != nullptr;
}

void InputSystem::recordTo(ReplayRecorder* recorder) { replayRecorder_ = recorder; }

void InputSystem::setUpgradeNavCallback(UpgradeNavCallback cb) { upgradeNavCallback_ = std::move(cb); }

std::uint32_t InputSystem::effectiveInputMask(const std::uint64_t tick) const {
    if (replayPlaybackMode_ && replayPlayer_) {
        return replayPlayer_->inputForTick(tick);
    }
    return inputMask_;
}

void InputSystem::clearTransient() {
    if (replayRecorder_) {
        replayRecorder_->recordTickInput(inputMask_);
    }
    inputMask_ &= ~InputDefensiveSpecial;
}

} // namespace engine
