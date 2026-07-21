/*****************************************************************//**
 * @file   blocked_by_component.h
 * @brief  被阻挡组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.22
 *********************************************************************/

#pragma once
#ifndef BLOCKED_BY_COMPONENT_H
#define BLOCKED_BY_COMPONENT_H

#include <entt/entity/entity.hpp>

namespace game::component {

    /// @brief 被阻挡组件，存储自身被哪个阻挡者阻挡（敌方单位用）
    struct BlockedByComponent {
        entt::entity mEntity{ entt::null };
    };

}   // namespace game::component

#endif // BLOCKED_BY_COMPONENT_H
