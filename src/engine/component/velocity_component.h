/*****************************************************************//**
 * @file   transform_component.h
 * @brief  速度组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.14
 *********************************************************************/

#pragma once
#include <glm/vec2.hpp>

namespace engine::component {

    /**
     * @brief 速度组件。
     */
    struct VelocityComponent {
        glm::vec2 mVelocity{};      ///< @brief 速度
    };

}