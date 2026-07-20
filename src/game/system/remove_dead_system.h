/*****************************************************************//**
 * @file   remove_dead_system.h
 * @brief  清理死亡实体的系统
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#pragma once
#ifndef REMOVE_DEAD_SYSTEM_H
#define REMOVE_DEAD_SYSTEM_H

#include <entt/entity/fwd.hpp>

namespace game::system {

    /**
     * @brief 清理死亡实体的系统
     */
    class RemoveDeadSystem {
    public:
        /**
         * @brief 更新清理逻辑
         * @param registry ECS注册表
         */
        void update(entt::registry& registry);
    };

} // namespace game::system

#endif // REMOVE_DEAD_SYSTEM_H
