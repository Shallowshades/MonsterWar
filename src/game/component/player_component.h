/*****************************************************************//**
 * @file   player_component.h
 * @brief  玩家组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.22
 *********************************************************************/

#pragma once
#ifndef PLAYER_COMPONENT_H
#define PLAYER_COMPONENT_H

namespace game::component {

    /// @brief 玩家组件，存储出击消耗
    struct PlayerComponent {
        int mCost{};
    };

}   // namespace game::component

#endif // PLAYER_COMPONENT_H
