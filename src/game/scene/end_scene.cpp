/*****************************************************************//**
  * @file   end_scene.cpp
  * @brief  游戏结束场景实现
  * @version 1.0
  *
  * @author Shallowshades
  * @date   2026.08.23
  *********************************************************************/

#include "end_scene.h"
#include "title_scene.h"
#include "../../engine/core/context.h"
#include "../../engine/core/game_state.h"
#include "../../engine/audio/audio_player.h"
#include "../../engine/utils/events.h"
#include "../system/debug_ui_system.h"
#include <spdlog/spdlog.h>
#include <entt/entity/registry.hpp>

using namespace entt::literals;

namespace game::scene {

EndScene::EndScene(engine::core::Context& context, bool is_win)
    : engine::scene::Scene("EndScene", context)
    , mIsWin(is_win) {
    mDebugUISystem = std::make_unique<game::system::DebugUISystem>(mRegistry, mContext);
}

EndScene::~EndScene() = default;

void EndScene::init() {
    if (mIsWin) {
        mContext.getAudioPlayer().playMusic("win"_hs, 0);
    } else {
        mContext.getAudioPlayer().playMusic("lose"_hs, 0);
    }
    // 进入游戏结束状态（GameScene::render 依此门控调试UI，避免两套窗口叠加冲突）
    mContext.getGameState().setState(engine::core::State::GameOver);
}

void EndScene::render() {
    engine::scene::Scene::render();
    mDebugUISystem->updateEnd(*this);    // 调试UI的显示优先级最高，最后渲染
}

void EndScene::onBackToTitleClick() {
    requestReplaceScene(std::make_unique<game::scene::TitleScene>(mContext));
}

void EndScene::onQuitClick() {
    quit();
}

}   // namespace game::scene
