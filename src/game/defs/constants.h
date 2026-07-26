/*****************************************************************//**
 * @file   constants.h
 * @brief  游戏层常量定义
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.22
 *********************************************************************/

#pragma once
#ifndef GAME_CONSTANTS_H
#define GAME_CONSTANTS_H

namespace game::defs {

    constexpr float BLOCK_RADIUS = 40.0f;       ///< @brief 阻挡半径
    constexpr float UNIT_RADIUS = 20.0f;        ///< @brief 角色自身半径（相当于碰撞盒，用于计算攻击范围）

    /// @brief 玩家类型枚举
    enum class PlayerType {
        UNKNOWN,
        MELEE,      ///< @brief 近战型，只能放在近战区域
        RANGED,     ///< @brief 远程型，只能放在远程区域
        MIXED       ///< @brief 混合型，可以放在任意区域（暂不实现，未来可拓展）
    };

}   // namespace game::defs

#endif // GAME_CONSTANTS_H
