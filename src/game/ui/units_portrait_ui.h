/*****************************************************************//**
 * @file   units_portrait_ui.h
 * @brief  单位肖像UI
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.19
 *********************************************************************/

#pragma once
#ifndef UNITS_PORTRAIT_UI_H
#define UNITS_PORTRAIT_UI_H

#include "../defs/events.h"
#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>

namespace engine::core {
    class Context;
}
namespace engine::ui {
    class UIPanel;
    class UIManager;
}

namespace game::ui {

/**
 * @brief 单位肖像UI
 *
 * 负责管理画面下方单位肖像UI的创建、更新和排列。
 * @note 数据（UIConfig/SessionData/BlueprintManager/GameStats）从 registry.ctx() 获取。
 */
class UnitsPortraitUI {
    // --- 构造函数传入的外部组件引用 ---
    entt::registry& mRegistry;
    engine::ui::UIManager& mUIManager;
    engine::core::Context& mContext;

    engine::ui::UIPanel* mAnchorPanel{ nullptr };   ///< @brief 保存单位肖像UI的根面板(非拥有指针)，方便使用

public:
    UnitsPortraitUI(entt::registry& registry,
        engine::ui::UIManager& ui_manager,
        engine::core::Context& context);
    ~UnitsPortraitUI();

    void update(float delta_time);

    engine::ui::UIPanel* getAnchorPanel() const { return mAnchorPanel; }

private:
    void updatePortraitCover();         ///< @brief 更新肖像遮盖（费用不足以出击时显示灰色遮盖）
    void createUnitsPortraitUI();       ///< @brief 创建单位肖像UI
    void arrangeUnitsPortraitUI();      ///< @brief 排列单位肖像UI（肖像增/减时调用）

    void movePortraitPanelRight(float delta_time);      ///< @brief 向右移动单位肖像UI
    void movePortraitPanelLeft(float delta_time);       ///< @brief 向左移动单位肖像UI

    // 事件回调函数
    void onRemoveUIPortraitEvent(const game::defs::RemoveUIPortraitEvent& event);
};

}   // namespace game::ui

#endif // UNITS_PORTRAIT_UI_H
