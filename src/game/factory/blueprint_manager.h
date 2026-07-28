/*****************************************************************//**
 * @file   blueprint_manager.h
 * @brief  蓝图管理器，用于存储和管理所有蓝图数据
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.21
 *********************************************************************/

#pragma once
#ifndef BLUEPRINT_MANAGER_H
#define BLUEPRINT_MANAGER_H

#include "../data/entity_blueprint.h"
#include <string_view>
#include <unordered_map>
#include <entt/entity/fwd.hpp>
#include <nlohmann/json_fwd.hpp>

namespace engine::resource {
    class ResourceManager;
}

namespace game::factory {

    /**
     * @brief 蓝图管理器，用于存储、管理所有蓝图
     *
     * 它从json数据中加载蓝图并保存到容器，并提供获取蓝图的功能。蓝图信息将由实体工厂使用。
     */
    class BlueprintManager {
        friend class EntityFactory;

    private:
        engine::resource::ResourceManager& mResourceManager;

        std::unordered_map<entt::id_type, data::PlayerClassBlueprint> mPlayerClassBlueprints; ///< @brief 玩家职业蓝图
        std::unordered_map<entt::id_type, data::EnemyClassBlueprint> mEnemyClassBlueprints;   ///< @brief 敌人类型蓝图容器
        std::unordered_map<entt::id_type, data::ProjectileBlueprint> mProjectileBlueprints;    ///< @brief 投射物蓝图
        // TODO: 未来添加其他蓝图容器

    public:
        explicit BlueprintManager(engine::resource::ResourceManager& resource_manager);

        [[nodiscard]] bool loadPlayerClassBlueprints(std::string_view player_json_path);    ///< @brief 加载玩家职业蓝图, 返回是否成功
        [[nodiscard]] bool loadEnemyClassBlueprints(std::string_view enemy_json_path);      ///< @brief 加载敌人类型蓝图, 返回是否成功
        [[nodiscard]] bool loadProjectileBlueprints(std::string_view projectile_json_path); ///< @brief 加载投射物蓝图, 返回是否成功
        // TODO: 未来添加其他蓝图加载函数

        const data::PlayerClassBlueprint& getPlayerClassBlueprint(entt::id_type id) const;  ///< @brief 获取指定ID的玩家职业蓝图
        const data::EnemyClassBlueprint& getEnemyClassBlueprint(entt::id_type id) const;    ///< @brief 获取指定ID的敌人类型蓝图
        const data::ProjectileBlueprint& getProjectileBlueprint(entt::id_type id) const;    ///< @brief 获取指定ID的投射物蓝图
        // TODO: 未来添加其他蓝图获取函数

    private:
        // --- 分别针对各个子蓝图进行json解析，并创建对应的蓝图结构体 ---
        [[nodiscard]] entt::id_type parseProjectileID(const nlohmann::json& json);
        [[nodiscard]] data::StatsBlueprint parseStats(const nlohmann::json& json);
        [[nodiscard]] data::SpriteBlueprint parseSprite(const nlohmann::json& json);
        [[nodiscard]] std::unordered_map<entt::id_type, data::AnimationBlueprint> parseAnimationsMap(const nlohmann::json& json);
        [[nodiscard]] data::SoundBlueprint parseSound(const nlohmann::json& json);
        [[nodiscard]] data::PlayerBlueprint parsePlayer(const nlohmann::json& json);
        [[nodiscard]] data::EnemyBlueprint parseEnemy(const nlohmann::json& json);
        [[nodiscard]] data::DisplayInfoBlueprint parseDisplayInfo(const nlohmann::json& json);
    };

}   // namespace game::factory

#endif // BLUEPRINT_MANAGER_H
