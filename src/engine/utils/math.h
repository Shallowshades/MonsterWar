/*****************************************************************//**
 * @file   math.h
 * @brief  数学计算类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.10.20
 *********************************************************************/

#pragma once
#ifndef MATH_H
#define MATH_H

#include <glm/glm.hpp>
#include <cmath>

namespace engine::utils {
/**
 * @brief 矩形.
 */
struct Rect {
	glm::vec2 position;
	glm::vec2 size;
};

/**
 * @brief 颜色.
 */
struct FColor {
	float r;
	float g;
	float b;
	float a;
};
} // engine::utils

#endif // !MATH_H
