/*****************************************************************//**
 * @file   place_occupied_component.h
 * @brief  放置点被占用组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.19
 *********************************************************************/

#pragma once
#ifndef PLACE_OCCUPIED_COMPONENT_H
#define PLACE_OCCUPIED_COMPONENT_H

#include <entt/entity/entity.hpp>

namespace game::component {

    /// @brief “位置被占用”组件，记录占用该放置点的单位实体，一个放置点只能放置一个单位
    struct PlaceOccupiedComponent {
        entt::entity mEntity{ entt::null };    ///< @brief 占用该放置点的单位实体
    };

}   // namespace game::component

#endif // PLACE_OCCUPIED_COMPONENT_H
