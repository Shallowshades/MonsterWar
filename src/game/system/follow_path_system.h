/*****************************************************************//**
 * @file   follow_path_system.h
 * @brief  路径跟随系统
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#pragma once
#ifndef FOLLOW_PATH_SYSTEM_H
#define FOLLOW_PATH_SYSTEM_H

#include "../data/waypoint_node.h"
#include <entt/entity/fwd.hpp>
#include <entt/signal/fwd.hpp>
#include <unordered_map>

namespace game::system {
    /**
     * @brief 路径跟随系统。
     * 根据路径节点更新敌人实体的速度和目标节点。
     */
    class FollowPathSystem {
    public:
        /**
         * @brief 更新路径跟随逻辑
         * @param registry ECS注册表
         * @param dispatcher 事件分发器
         * @param waypoint_nodes 路径节点映射表
         */
        void update(entt::registry& registry,
            entt::dispatcher& dispatcher,
            std::unordered_map<int, game::data::WaypointNode>& waypoint_nodes);
    };

} // namespace game::system

#endif // FOLLOW_PATH_SYSTEM_H
