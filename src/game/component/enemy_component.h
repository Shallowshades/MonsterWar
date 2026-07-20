/*****************************************************************//**
 * @file   enemy_component.h
 * @brief  敌人组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#pragma once
#ifndef ENEMY_COMPONENT_H
#define ENEMY_COMPONENT_H

namespace game::component {

    /**
     * @brief 敌人组件，包含目标节点ID和自身速度。
     */
    struct EnemyComponent {
        int mTargetWaypointId;      ///< @brief 当前目标路径节点ID
        float mSpeed;               ///< @brief 移动速度

        /**
         * @brief 构造函数
         * @param target_waypoint_id 目标路径节点ID
         * @param speed 移动速度
         */
        EnemyComponent(int target_waypoint_id, float speed)
            : mTargetWaypointId(target_waypoint_id), mSpeed(speed) {}
    };

}   // namespace game::component

#endif // ENEMY_COMPONENT_H
