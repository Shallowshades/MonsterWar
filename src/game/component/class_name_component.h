/*****************************************************************//**
 * @file   class_name_component.h
 * @brief  职业名称组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.21
 *********************************************************************/

#pragma once
#ifndef CLASS_NAME_COMPONENT_H
#define CLASS_NAME_COMPONENT_H

#include <entt/entity/entity.hpp>
#include <string>

namespace game::component {

/**
 * @brief 职业名称组件。
 * 用于存储角色职业（例如战士、法师、弓箭手）或
 * 敌人类型（例如史莱姆、狼、哥布林）的ID和名称。
 */
struct ClassNameComponent {
    entt::id_type mClassId{ entt::null };
    std::string mClassName;                 ///< @brief 名称，主要用于显示
};

}   // namespace game::component

#endif // CLASS_NAME_COMPONENT_H
