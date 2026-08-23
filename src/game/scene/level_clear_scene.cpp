/*****************************************************************//**
  * @file   level_clear_scene.cpp
  * @brief  通关结算场景实现
  * @version 1.0
  *
  * @author Shallowshades
  * @date   2026.08.23
  *********************************************************************/

#include "level_clear_scene.h"
#include "game_scene.h"
#include "title_scene.h"
#include "../system/debug_ui_system.h"
#include "../../engine/ui/ui_manager.h"
#include "../../engine/core/context.h"
#include "../../engine/core/game_state.h"
#include "../../engine/audio/audio_player.h"
#include "../../engine/utils/events.h"
#include <spdlog/spdlog.h>
#include <entt/entity/registry.hpp>

using namespace entt::literals;

namespace game::scene {

LevelClearScene::LevelClearScene(engine::core::Context& context,
    std::shared_ptr<game::factory::BlueprintManager> blueprint_manager,
    std::shared_ptr<game::data::UIConfig> ui_config,
    std::shared_ptr<game::data::LevelConfig> level_config,
    std::shared_ptr<game::data::SessionData> session_data,
    game::data::GameStats& game_stats)
    : engine::scene::Scene("LevelClearScene", context)
    , mBlueprintManager(blueprint_manager)
    , mUIConfig(ui_config)
    , mLevelConfig(level_config)
    , mSessionData(session_data)
    , mGameStats(game_stats) {
    // 直接在构造函数中初始化调试UI系统（结算场景只需ImGui显示）
    mDebugUISystem = std::make_unique<game::system::DebugUISystem>(mRegistry, mContext);
}

LevelClearScene::~LevelClearScene() = default;

void LevelClearScene::init() {
    if (!mUIConfig || !mLevelConfig || !mSessionData || !mBlueprintManager) {
        spdlog::error("LevelClearScene: ui_config, level_config, session_data 或 blueprint_manager 必须有值");
        return;
    }
    // 进入关卡过关状态（GameScene::render 依此门控调试UI，避免两套窗口叠加冲突）
    mContext.getGameState().setState(engine::core::State::LevelClear);

    // 将共享数据放入注册表上下文，供结算窗口（可排序角色表格/存档面板）直接获取
    mRegistry.ctx().emplace<std::shared_ptr<game::data::SessionData>>(mSessionData);
    mRegistry.ctx().emplace<std::shared_ptr<game::factory::BlueprintManager>>(mBlueprintManager);
    mRegistry.ctx().emplace<std::shared_ptr<game::data::UIConfig>>(mUIConfig);
    mContext.getAudioPlayer().playMusic("win"_hs, 0);    // 播放通关音乐（播放一次）
}

void LevelClearScene::render() {
    engine::scene::Scene::render();
    mDebugUISystem->updateLevelClear(*this);    // 调试UI的显示优先级最高，最后渲染
}

void LevelClearScene::onNextLevelClick() {
    mSessionData->addOneLevel();
    mSessionData->setLevelClear(false);
    // 把共享的蓝图/会话/UI/关卡配置传给新 GameScene，进入下一关
    requestReplaceScene(std::make_unique<game::scene::GameScene>(mContext,
        mBlueprintManager, mSessionData, mUIConfig, mLevelConfig));
}

void LevelClearScene::onBackToTitleClick() {
    requestReplaceScene(std::make_unique<game::scene::TitleScene>(mContext));
}

void LevelClearScene::onSaveClick() {
    mShowSavePanel = !mShowSavePanel;
}

}   // namespace game::scene
