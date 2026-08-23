/*****************************************************************//**
 * @file   skill_system.h
 * @brief  技能系统
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.23
 *********************************************************************/

#pragma once
#ifndef GAME_SKILL_SYSTEM_H
#define GAME_SKILL_SYSTEM_H

#include "../defs/events.h"
#include <entt/signal/fwd.hpp>
#include <entt/entity/fwd.hpp>

namespace game::factory {
    class EntityFactory;
}

namespace game::system {

    /**
     * @brief 技能系统
     * @note 用于管理技能的施放与显示、Buff增删等操作
     */
    class SkillSystem {
        entt::registry& mRegistry;
        entt::dispatcher& mDispatcher;
        game::factory::EntityFactory& mEntityFactory;

    public:
        SkillSystem(entt::registry& registry, entt::dispatcher& dispatcher, game::factory::EntityFactory& entity_factory);
        ~SkillSystem();

    private:
        // 事件回调函数
        void onSkillReadyEvent(const game::defs::SkillReadyEvent& event);        ///< @brief 技能准备就绪
        void onSkillActiveEvent(const game::defs::SkillActiveEvent& event);      ///< @brief 技能激活
        void onSkillDurationEndEvent(const game::defs::SkillDurationEndEvent& event);  ///< @brief 技能持续结束
        void onRemoveUnitEvent(const game::defs::RemovePlayerUnitEvent& event);  ///< @brief 单位移除（回收技能显示实体）

        // Buff增删函数
        void addBuff(entt::entity entity, entt::id_type skill_id);
        void removeBuff(entt::entity entity, entt::id_type skill_id);
    };

}   // namespace game::system

#endif // GAME_SKILL_SYSTEM_H
