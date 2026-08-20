/*****************************************************************//**
 * @file   enemy_spawner.h
 * @brief  敌人生成器
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.20
 *********************************************************************/

#pragma once
#ifndef ENEMY_SPAWNER_H
#define ENEMY_SPAWNER_H

#include <deque>    // 双端队列：两端都可以入队或出队（支持打乱顺序）
#include <entt/entity/fwd.hpp>
#include <entt/signal/fwd.hpp>

namespace game::factory {
    class EntityFactory;
}

namespace game::spawner {

    /**
     * @brief 敌人生成器，根据波次数据生成敌人
     */
    class EnemySpawner {
        entt::registry& mRegistry;                          ///< @brief ECS注册表
        game::factory::EntityFactory& mEntityFactory;       ///< @brief 实体工厂

        float mSpawnTimer{ 0.0f };                          ///< @brief 波次内生成计时器（单位：秒）
        float mSpawnInterval{ 0.0f };                       ///< @brief 波次内生成间隔（单位：秒）
        std::deque<entt::id_type> mEnemyTypes;              ///< @brief 波次内敌人队列（使用双端队列是为了支持随机打乱顺序）

    public:
        /**
         * @brief 构造函数
         * @param registry entt注册表
         * @param entity_factory 实体工厂
         */
        EnemySpawner(entt::registry& registry, game::factory::EntityFactory& entity_factory);
        ~EnemySpawner();

        void update(float delta_time);

    private:
        void spawnEnemy();
    };

}   // namespace game::spawner

#endif // ENEMY_SPAWNER_H
