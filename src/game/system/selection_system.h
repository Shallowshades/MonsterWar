/*****************************************************************//**
 * @file   selection_system.h
 * @brief  选择单位系统
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.21
 *********************************************************************/

#pragma once
#ifndef SELECTION_SYSTEM_H
#define SELECTION_SYSTEM_H

#include <entt/entity/fwd.hpp>

namespace engine::core {
    class Context;
}

namespace game::system {

/**
 * @brief 选择单位系统
 * @note 处理玩家角色的"选中单位"操作以及"鼠标悬浮单位"的操作。
 */
class SelectionSystem {
    entt::registry& mRegistry;              ///< @brief ECS注册表
    engine::core::Context& mContext;        ///< @brief 上下文，用于获取输入管理器

public:
    SelectionSystem(entt::registry& registry, engine::core::Context& context);
    ~SelectionSystem();

    void update();  ///< @brief 每帧判断是否有鼠标悬浮单位

private:
    void clearCurrentSelection();   ///< @brief 清除当前选中单位（移除范围标签、置空）

    // 输入控制回调函数
    bool onMouseLeftClick();    ///< @brief 鼠标左键点击时，如果有悬浮单位且是玩家，则选中该单位
    bool onMouseRightClick();   ///< @brief 鼠标右键点击时，清除当前选中单位（返回false允许穿透）
};

}   // namespace game::system

#endif // SELECTION_SYSTEM_H
