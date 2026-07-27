/*****************************************************************//**
 * @file   events.h
 * @brief  游戏层事件定义
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#pragma once
#ifndef GAME_EVENTS_H
#define GAME_EVENTS_H

#include <entt/entity/entity.hpp>

namespace game::defs {

    struct EnemyArriveHomeEvent {};         ///< @brief 敌人到达基地的事件

    /// @brief 攻击（命中）事件
    struct AttackEvent {
        entt::entity mAttacker{ entt::null }; ///< @brief 攻击者
        entt::entity mTarget{ entt::null };   ///< @brief 目标
        float mDamage{};                    ///< @brief 原始伤害
    };

    /// @brief 治疗（命中）事件
    struct HealEvent {
        entt::entity mHealer{ entt::null };   ///< @brief 治疗者
        entt::entity mTarget{ entt::null };   ///< @brief 目标
        float mAmount{};                    ///< @brief 治疗量
    };

}   // namespace game::defs

#endif // GAME_EVENTS_H
