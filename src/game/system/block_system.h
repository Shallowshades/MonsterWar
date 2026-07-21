/*****************************************************************//**
 * @file   block_system.h
 * @brief  阻挡系统
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.22
 *********************************************************************/

#pragma once
#ifndef BLOCK_SYSTEM_H
#define BLOCK_SYSTEM_H

#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>

namespace game::system {

    /**
     * @brief 阻挡系统
     * 用于判断敌人是否被阻挡，并更新阻挡相关组件。
     */
    class BlockSystem {
    public:
        void update(entt::registry& registry, entt::dispatcher& dispatcher);
    };

}   // namespace game::system

#endif // BLOCK_SYSTEM_H
