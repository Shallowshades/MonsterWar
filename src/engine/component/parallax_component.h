/*****************************************************************//**
 * @file   parallax_component.h
 * @brief  视差滚动效果组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.07.14
 *********************************************************************/

#pragma once
#ifndef PARALLAX_COMPONENT_H
#define PARALLAX_COMPONENT_H

#include <glm/vec2.hpp>
#include <utility>

namespace engine::component {
    /**
     * @brief 视差组件，包含滚动速度因子、是否重复和是否可见。（需和Sprite配合使用）
     */
    struct ParallaxComponent {
        glm::vec2 mScrollFactor{};              ///< @brief 滚动速度因子 (0=静止, 1=随相机移动, <1=比相机慢)
        glm::bvec2 mRepeat{ true };             ///< @brief 是否重复
        bool mIsVisible{ true };                ///< @brief 是否可见

        /**
         * @brief 构造函数
         * @param scroll_factor
         * @param repeat 是否重复，默认(true, true)
         * @param is_visible 是否可见，默认true
         */
        ParallaxComponent(glm::vec2 scroll_factor, glm::bvec2 repeat = glm::bvec2(true, true), bool is_visible = true) 
            : mScrollFactor(std::move(scroll_factor)), mRepeat(std::move(repeat)), mIsVisible(is_visible) {}

    };
} // engine::component
#endif // PARALLAX_COMPONENT_H
