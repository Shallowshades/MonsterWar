/*****************************************************************//**
 * @file   health_bar_system.h
 * @brief  血量条系统（渲染），用于显示角色的血量条
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.09
 *********************************************************************/

#pragma once
#ifndef GAME_HEALTH_BAR_SYSTEM_H
#define GAME_HEALTH_BAR_SYSTEM_H

#include <entt/entity/fwd.hpp>

namespace engine::render {
    class Renderer;
    class Camera;
}

namespace game::system {

    /**
     * @brief 血量条系统（渲染），用于显示角色的血量条
     */
    class HealthBarSystem {
    public:
        void update(entt::registry& registry, engine::render::Renderer& renderer, engine::render::Camera& camera);
    };

} // namespace game::system

#endif // GAME_HEALTH_BAR_SYSTEM_H
