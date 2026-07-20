/*****************************************************************//**
 * @file   waypoint_node.h
 * @brief  路径节点数据结构
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#pragma once
#ifndef WAYPOINT_NODE_H
#define WAYPOINT_NODE_H

#include <glm/vec2.hpp>
#include <vector>

namespace game::data {

    /**
     * @brief 路径节点数据结构。
     * @note 包含节点ID、坐标和指向下一个节点的ID列表。
     */
    struct WaypointNode {
        int mId;                            ///< @brief 节点ID
        glm::vec2 mPosition;                ///< @brief 节点坐标
        std::vector<int> mNextNodeIds;      ///< @brief 指向下一个节点的ID列表

        WaypointNode() = default;

        /**
         * @brief 构造函数
         * @param id 节点ID
         * @param position 节点坐标
         * @param next_node_ids 指向下一个节点的ID列表
         */
        WaypointNode(int id, glm::vec2 position, std::vector<int> next_node_ids)
            : mId(id), mPosition(std::move(position)), mNextNodeIds(std::move(next_node_ids)) {}
    };

} // namespace game::data

#endif // WAYPOINT_NODE_H
