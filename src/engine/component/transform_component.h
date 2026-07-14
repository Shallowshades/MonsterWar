/*****************************************************************//**
 * @file   transform_component.h
 * @brief  变换组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.07.14
 *********************************************************************/

#pragma once
#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <glm/vec2.hpp>
#include <utility>

namespace engine::component {

    /**
     * @brief 变换组件，包含位置、缩放和旋转。

     */
    struct TransformComponent {
        glm::vec2 mPosition{};              ///< @brief 位置
        glm::vec2 mScale{ 1.0f };           ///< @brief 缩放
        float mRotation{};                  ///< @brief 旋转

        /**
         * @brief 构造函数
         * @param position 位置
         * @param scale 缩放，默认(1.0f,1.0f)
         * @param rotation 旋转，默认0
         */
        explicit TransformComponent(glm::vec2 position,
            glm::vec2 scale = glm::vec2(1.0f, 1.0f),
            float rotation = 0.0f) :
            mPosition(std::move(position)),
            mScale(std::move(scale)),
            mRotation(rotation) {}
    };
}
																				
#endif // TRANSFORM_COMPONENT_H
