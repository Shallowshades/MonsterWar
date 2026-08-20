/*****************************************************************//**
 * @file   enemy_spawner.cpp
 * @brief  敌人生成器实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.20
 *********************************************************************/

#include "enemy_spawner.h"
#include "../data/level_config.h"
#include "../data/level_data.h"
#include "../data/waypoint_node.h"
#include "../factory/entity_factory.h"
#include "../../engine/utils/math.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <spdlog/spdlog.h>

namespace game::spawner {

    EnemySpawner::EnemySpawner(entt::registry& registry, game::factory::EntityFactory& entity_factory)
        : mRegistry(registry), mEntityFactory(entity_factory) {
    }

    EnemySpawner::~EnemySpawner() {
    }

    void EnemySpawner::update(float delta_time) {
        auto& waves = mRegistry.ctx().get<game::data::Waves&>();
        // 如果"关卡波次队列"不为空，则考虑添加敌人
        if (!waves.mWaves.empty()) {
            waves.mNextWaveCountDown -= delta_time;
            // 如果已经到了新的一波，则弹出并载入敌人波次队列
            if (waves.mNextWaveCountDown <= 0.0f) {
                auto& wave = waves.mWaves.front();
                // 更新下一波次倒数计时器
                waves.mNextWaveCountDown = wave.mNextWaveInterval;
                // 更新本波次敌人生成间隔与生成计时器
                mSpawnInterval = wave.mSpawnInterval;
                mSpawnTimer = 0.0f;
                // 先把所有敌人依次加入"当前波次队列"
                for (auto& enemy_type : wave.mEnemyTypes) {
                    auto [class_id, count] = enemy_type;
                    for (int i = 0; i < count; ++i) {
                        mEnemyTypes.push_back(class_id);
                    }
                }
                // 打乱队列，确保敌人生成顺序随机
                engine::utils::shuffle(mEnemyTypes.begin(), mEnemyTypes.end());

                // 本波次数据处理完毕，弹出关卡波次队列头
                waves.mWaves.pop();
                spdlog::info("开始新一波敌人生成");
            }
        }

        // 如果"当前波次队列"不为空，则按"敌人生成间隔"生成敌人
        if (!mEnemyTypes.empty()) {
            mSpawnTimer += delta_time;
            if (mSpawnTimer >= mSpawnInterval) {
                mSpawnTimer = 0.0f;
                spawnEnemy();       // 生成一个敌人
            }
        }
    }

    void EnemySpawner::spawnEnemy() {
        // 获取上下文数据
        auto& start_points = mRegistry.ctx().get<std::vector<int>&>();
        auto& waypoint_nodes = mRegistry.ctx().get<std::unordered_map<int, game::data::WaypointNode>&>();
        auto& level_config = mRegistry.ctx().get<std::shared_ptr<game::data::LevelConfig>&>();
        auto& level_number = mRegistry.ctx().get<int&>();

        // 随机选择起点
        auto random_index = engine::utils::randomInt(0, static_cast<int>(start_points.size()) - 1);
        auto start_index = start_points[random_index];
        auto position = waypoint_nodes[start_index].mPosition;
        auto level = level_config->getEnemyLevel(level_number);
        auto rarity = level_config->getEnemyRarity(level_number);

        // 弹出敌人类型
        auto enemy_type = mEnemyTypes.front();
        mEnemyTypes.pop_front();

        // 创建敌人
        mEntityFactory.createEnemyUnit(enemy_type, position, start_index, level, rarity);
        spdlog::info("创建敌人: 位置: {}, {}", position.x, position.y);
    }

}   // namespace game::spawner
