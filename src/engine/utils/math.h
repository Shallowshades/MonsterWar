/*****************************************************************//**
 * @file   math.h
 * @brief  数学计算类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2026.07.14
 *********************************************************************/

#pragma once
#ifndef MATH_H
#define MATH_H

#include <glm/vec2.hpp>

namespace engine::utils {

    /**
     * @brief 自定义矩形结构体，包含位置和大小。
     */
    struct Rect
    {
        glm::vec2 mPosition{};
        glm::vec2 mSize{};

        Rect() = default;
        Rect(glm::vec2 position, glm::vec2 size) : mPosition(position), mSize(size) {}
        Rect(float x, float y, float width, float height) : mPosition(x, y), mSize(width, height) {}
    };

    /**
     * @brief 自定义颜色结构体。
     */
    struct FColor
    {
        float r{};
        float g{};
        float b{};
        float a{};
    };

} // namespace engine::utils
#endif // !MATH_H
