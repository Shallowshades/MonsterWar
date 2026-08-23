/*****************************************************************//**
 * @file   debug_ui_system.h
 * @brief  调试UI系统
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.21
 *********************************************************************/

#pragma once
#ifndef DEBUG_UI_SYSTEM_H
#define DEBUG_UI_SYSTEM_H

#include <entt/entity/entity.hpp>
#include "../defs/events.h"

namespace engine::core {
    class Context;
}
namespace game::scene {
    class TitleScene;
}

namespace game::system {

/**
 * @brief 调试 UI 系统，负责显示调试 UI。
 *
 * @note 调试UI的主要目的是方便debug，并快速开发UI原型。
 * @note 游戏正式发布时往往会删除，因此不需要过度设计。
 */
class DebugUISystem {
    entt::registry& mRegistry;              ///< @brief ECS注册表（预留：后续课程用其查看实体信息）
    engine::core::Context& mContext;        ///< @brief 上下文，用于获取渲染器/游戏状态
    entt::id_type mHoveredPortrait{ entt::null };   ///< @brief 鼠标悬浮的肖像角色名哈希
    bool mShowDebugUI{ true };              ///< @brief 是否显示调试工具窗口

public:
    DebugUISystem(entt::registry& registry, engine::core::Context& context);
    ~DebugUISystem();

    // ImGui 步骤3: 一轮循环内，ImGui 需要做的操作（逻辑+渲染）
    void update();

    // 标题场景的ImGui更新（直接操作TitleScene的私有成员变量及回调）
    void updateTitle(game::scene::TitleScene& title_scene);

private:
    // 封装开始、结束帧的方法
    void beginFrame();
    void endFrame();

    // 封装每个UI显示模块
    void renderHoveredUnit();     // 显示鼠标悬浮单位的 tooltip
    void renderSelectedUnit();    // 显示鼠标选中单位的角色状态窗口
    void renderHoveredPortrait(); // 显示鼠标悬浮肖像的 tooltip
    void renderInfoUI();          // 显示关卡信息窗口
    void renderSettingUI();       // 显示设置工具窗口
    void renderDebugUI();         // 显示调试工具窗口

    // 标题场景的UI显示模块
    void renderTitleLogo();                               // 显示标题LOGO
    void renderTitleButtons(game::scene::TitleScene& title_scene);  // 显示标题按钮
    void renderUnitInfoUI(bool& show_unit_info);          // 显示角色信息窗口
    void renderLoadPanelUI(bool& show_load_panel);        // 显示读档选择窗口
    void renderSavePanelUI(bool& show_save_panel);        // 显示存档选择窗口
    void renderUnitTable();                               // 显示角色信息表格（可排序）

    // 肖像悬浮事件回调
    void onUIPortraitHoverEnterEvent(const game::defs::UIPortraitHoverEnterEvent& event);
    void onUIPortraitHoverLeaveEvent(const game::defs::UIPortraitHoverLeaveEvent& event);
};

}   // namespace game::system

#endif // DEBUG_UI_SYSTEM_H
