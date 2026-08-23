/*****************************************************************//**
  * @file   level_clear_scene.h
  * @brief  通关结算场景类
  * @version 1.0
  *
  * @author Shallowshades
  * @date   2026.08.23
  *********************************************************************/

#pragma once
#ifndef LEVEL_CLEAR_SCENE_H
#define LEVEL_CLEAR_SCENE_H

#include "../../engine/scene/scene.h"
#include "../data/ui_config.h"
#include "../data/session_data.h"
#include "../data/game_stats.h"
#include "../data/level_config.h"
#include "../factory/blueprint_manager.h"
#include "../system/fwd.h"
#include <memory>

namespace game::scene {

/**
 * @brief 通关结算场景。
 *
 * 关卡胜利后展示结算界面：奖励积分、可排序的角色表格（升级）、
 * 下一关/保存/返回标题三个按钮。
 * @note 下层 GameScene 仍留在场景栈中（push 压入），本场景只渲染结算 UI。
 */
class LevelClearScene final : public engine::scene::Scene {
    // 允许 ImGui 系统直接访问私有成员与按钮回调
    friend class game::system::DebugUISystem;

    // 场景中共享的数据实例（与 GameScene 共用同一份，由构造函数传入）
    std::shared_ptr<game::factory::BlueprintManager> mBlueprintManager;   ///< @brief 蓝图管理器
    std::shared_ptr<game::data::UIConfig> mUIConfig;                      ///< @brief UI配置
    std::shared_ptr<game::data::LevelConfig> mLevelConfig;                ///< @brief 关卡配置
    std::shared_ptr<game::data::SessionData> mSessionData;                ///< @brief 会话数据

    game::data::GameStats& mGameStats;       ///< @brief 关卡内游戏统计数据（引用下层 GameScene 的成员，用于显示）

    // 目前只需要DebugUI系统
    std::unique_ptr<game::system::DebugUISystem> mDebugUISystem;          ///< @brief 调试UI系统（ImGui调试窗口）

    bool mShowSavePanel{ false };            ///< @brief 是否显示存档面板

public:
    /**
     * @brief 构造函数。
     * @param context 对 Context 实例的引用
     * @param blueprint_manager 蓝图管理器
     * @param ui_config UI配置
     * @param level_config 关卡配置
     * @param session_data 会话数据
     * @param game_stats 关卡内游戏统计数据（本场景只读显示）
     */
    LevelClearScene(engine::core::Context& context,
        std::shared_ptr<game::factory::BlueprintManager> blueprint_manager,
        std::shared_ptr<game::data::UIConfig> ui_config,
        std::shared_ptr<game::data::LevelConfig> level_config,
        std::shared_ptr<game::data::SessionData> session_data,
        game::data::GameStats& game_stats);
    ~LevelClearScene();

    void init() override;
    void render() override;

private:
    // 按钮回调函数
    void onNextLevelClick();     // 进入下一关
    void onBackToTitleClick();   // 返回标题场景
    void onSaveClick();          // 切换存档面板显示
};

}   // namespace game::scene

#endif // LEVEL_CLEAR_SCENE_H
