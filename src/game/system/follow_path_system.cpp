/*****************************************************************//**
 * @file   follow_path_system.cpp
 * @brief  路径跟随系统实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#include "follow_path_system.h"
#include "../data/waypoint_node.h"
#include "../component/enemy_component.h"
#include "../../engine/component/velocity_component.h"
#include "../../engine/component/transform_component.h"
#include "../defs/tags.h"
#include "../../engine/utils/math.h"
#include "../defs/events.h"
#include <entt/signal/dispatcher.hpp>
#include <entt/entity/registry.hpp>
#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

namespace game::system {

    void FollowPathSystem::update(entt::registry& registry, entt::dispatcher& dispatcher, std::unordered_map<int, game::data::WaypointNode>& waypoint_nodes) {
        spdlog::trace("FollowPathSystem::update");
        // 筛选依据：速度组件、变换组件、敌人组件
        auto view = registry.view<engine::component::VelocityComponent,
            engine::component::TransformComponent,
            game::component::EnemyComponent>();
        for (auto entity : view) {
            auto& velocity = view.get<engine::component::VelocityComponent>(entity);
            auto& transform = view.get<engine::component::TransformComponent>(entity);
            auto& enemy = view.get<game::component::EnemyComponent>(entity);

            // 获取目标节点
            auto target_node = waypoint_nodes.at(enemy.mTargetWaypointId);

            // 计算当前位置到目标位置的向量
            glm::vec2 direction = target_node.mPosition - transform.mPosition;

            // 如果距离小于阈值，则切换到下一个节点（阈值不要太小，不然敌人速度快的话可能造成震荡）
            if (glm::length(direction) < 5.0f) {
                // 如果下一个节点ID列表为空，代表到达终点。则发送信号并添加删除标记
                auto size = target_node.mNextNodeIds.size();
                if (size == 0) {
                    spdlog::info("到达终点");
                    // 发送信号并添加删除标记
                    dispatcher.enqueue<game::defs::EnemyArriveHomeEvent>(); // 具体做什么，由回调函数决定
                    registry.emplace<game::defs::DeadTag>(entity);          // 用于延迟删除
                    continue;
                }
                // 随机选择下一个节点
                auto target_index = engine::utils::randomInt(0, static_cast<int32_t>(size) - 1);
                enemy.mTargetWaypointId = target_node.mNextNodeIds[target_index];
                // 更新目标节点与方向矢量
                target_node = waypoint_nodes.at(enemy.mTargetWaypointId);
                direction = target_node.mPosition - transform.mPosition;
            }

            // 更新速度组件：velocity = 方向矢量 * speed
            velocity.mVelocity = glm::normalize(direction) * enemy.mSpeed;
        }
    }

}   // namespace game::system
