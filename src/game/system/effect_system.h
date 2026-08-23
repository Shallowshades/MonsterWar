/*****************************************************************//**
 * @file   effect_system.h
 * @brief  特效系统，处理所有特效的创建
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.09
 *********************************************************************/

#pragma once
#ifndef GAME_EFFECT_SYSTEM_H
#define GAME_EFFECT_SYSTEM_H

#include "../defs/events.h"
#include <entt/entity/fwd.hpp>
#include <entt/signal/fwd.hpp>

namespace game::factory {
    class EntityFactory;
}

namespace game::system {

    /**
     * @brief 特效系统，处理所有特效的创建
     *
     * 监听特效事件，通过实体工厂创建对应的特效实体。
     */
    class EffectSystem {
        entt::registry& mRegistry;
        entt::dispatcher& mDispatcher;
        game::factory::EntityFactory& mEntityFactory;

    public:
        EffectSystem(entt::registry& registry, entt::dispatcher& dispatcher, game::factory::EntityFactory& entity_factory);
        ~EffectSystem();

    private:
        // 事件回调函数
        void onEnemyDeadEffectEvent(const game::defs::EnemyDeadEffectEvent& event);   ///< @brief 敌人死亡特效事件
        void onEffectEvent(const game::defs::EffectEvent& event);                     ///< @brief (通用)特效事件
        // TODO: 未来添加其他特效事件回调函数
    };

} // namespace game::system

#endif // GAME_EFFECT_SYSTEM_H
