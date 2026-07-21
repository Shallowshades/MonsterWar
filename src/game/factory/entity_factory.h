/*****************************************************************//**
 * @file   entity_factory.h
 * @brief  实体工厂，用于创建不同类型的实体
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.21
 *********************************************************************/

#pragma once
#ifndef ENTITY_FACTORY_H
#define ENTITY_FACTORY_H

#include "../data/entity_blueprint.h"
#include <entt/entity/fwd.hpp>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace game::factory {

class BlueprintManager;

/**
 * @brief 实体工厂，用于创建不同类型的实体
 *
 * 实体工厂通过蓝图管理器获取蓝图数据，并创建不同类型的实体。
 */
class EntityFactory {
private:
    entt::registry& mRegistry;
    BlueprintManager& mBlueprintManager;

public:
    /**
     * @brief 构造函数
     * @param registry ECS注册表
     * @param blueprint_manager 蓝图管理器
     */
    EntityFactory(entt::registry& registry, BlueprintManager& blueprint_manager);

    entt::entity createEnemyUnit(entt::id_type class_id, const glm::vec2& position, int target_waypoint_id, int level = 1, int rarity = 1);
    // TODO: 未来添加其他实体的创建函数

private:
    // --- 组件创建函数 ---
    void addTransformComponent(entt::entity entity, const glm::vec2& position, const glm::vec2& scale = glm::vec2(1.0f), float rotation = 0.0f);
    void addSpriteComponent(entt::entity entity, const data::SpriteBlueprint& sprite, const bool is_flipped = false);
    void addAnimationComponent(entt::entity entity,
        const std::unordered_map<entt::id_type, data::AnimationBlueprint>& animation_blueprints,
        const data::SpriteBlueprint& sprite_blueprint,
        entt::id_type default_animation_id);
    void addStatsComponent(entt::entity entity, const data::StatsBlueprint& stats, int level = 1, int rarity = 1);
    void addEnemyComponent(entt::entity entity, const data::EnemyBlueprint& enemy, int target_waypoint_id);
    void addAudioComponent(entt::entity entity, const data::SoundBlueprint& sounds);
    // TODO: 未来添加其他组件创建函数
};

}   // namespace game::factory

#endif // ENTITY_FACTORY_H
