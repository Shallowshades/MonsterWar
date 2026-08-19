/*****************************************************************//**
 * @file   unit_prep_component.h
 * @brief  单位准备组件（“准备单位”跟随鼠标的待放置单位）
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.19
 *********************************************************************/

#pragma once
#ifndef UNIT_PREP_COMPONENT_H
#define UNIT_PREP_COMPONENT_H

#include "../defs/constants.h"
#include <entt/entity/entity.hpp>

namespace game::component {

    /// @brief 单位准备组件，存储待放置单位的信息（名称、类型、范围、费用）
    struct UnitPrepComponent {
        entt::id_type mNameId{ entt::null };               ///< @brief 角色名ID
        game::defs::PlayerType mType{ game::defs::PlayerType::UNKNOWN }; ///< @brief 单位类型（近战/远程）
        float mRange{ 0 };                                  ///< @brief 攻击范围（远程单位显示范围圆）
        int mCost{ 0 };                                     ///< @brief 出击费用
    };

}   // namespace game::component

#endif // UNIT_PREP_COMPONENT_H
