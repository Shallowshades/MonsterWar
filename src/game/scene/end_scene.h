/*****************************************************************//**
  * @file   end_scene.h
  * @brief  游戏结束场景类
  * @version 1.0
  *
  * @author Shallowshades
  * @date   2026.08.23
  *********************************************************************/

#pragma once
#ifndef END_SCENE_H
#define END_SCENE_H

#include "../../engine/scene/scene.h"
#include "../system/fwd.h"
#include <memory>

namespace game::scene {

/**
 * @brief 游戏结束场景。
 *
 * 游戏胜利（通关最后一关）或失败（基地被摧毁）时展示结束画面：
 * 胜利/失败大字 + 返回标题/退出游戏两个按钮。
 * @note 下层 GameScene 仍留在场景栈中（push 压入），本场景只渲染结束 UI。
 */
class EndScene final : public engine::scene::Scene {
    // 允许 ImGui 系统直接访问私有成员与按钮回调
    friend class game::system::DebugUISystem;

    // 目前只需要DebugUI系统
    std::unique_ptr<game::system::DebugUISystem> mDebugUISystem;   ///< @brief 调试UI系统（ImGui调试窗口）

    bool mIsWin{ false };       ///< @brief 是否获胜（决定胜利/失败文本与音乐）

public:
    /**
     * @brief 构造函数。
     * @param context 对 Context 实例的引用
     * @param is_win 是否获胜
     */
    EndScene(engine::core::Context& context, bool is_win = false);
    ~EndScene();

    bool init() override;
    void render() override;

private:
    // 按钮回调函数
    void onBackToTitleClick();   // 返回标题场景
    void onQuitClick();          // 退出游戏
};

}   // namespace game::scene

#endif // END_SCENE_H
