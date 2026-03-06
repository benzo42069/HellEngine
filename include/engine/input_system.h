#pragma once

#include <engine/replay.h>

#include <SDL.h>

#include <cstdint>
#include <functional>

namespace engine {

enum class UpgradeNavAction {
    MoveLeft,
    MoveRight,
    SelectSlot1,
    SelectSlot2,
    SelectSlot3,
    Confirm,
    Reroll,
    ToggleScreen,
};

class InputSystem {
  public:
    using UpgradeNavCallback = std::function<void(UpgradeNavAction)>;

    void pollInput(bool headless);
    void processEvent(const SDL_Event& event);
    [[nodiscard]] std::uint32_t inputMask() const;
    void setReplayPlayback(ReplayPlayer* player);
    void recordTo(ReplayRecorder* recorder);
    void setUpgradeNavCallback(UpgradeNavCallback cb);
    [[nodiscard]] std::uint32_t effectiveInputMask(std::uint64_t tick) const;
    void clearTransient();

  private:
    std::uint32_t inputMask_ {0};
    ReplayPlayer* replayPlayer_ {nullptr};
    ReplayRecorder* replayRecorder_ {nullptr};
    UpgradeNavCallback upgradeNavCallback_ {};
    bool replayPlaybackMode_ {false};
};

} // namespace engine
