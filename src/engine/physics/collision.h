/*****************************************************************//**
 * @file   collision.h
 * @brief  碰撞
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2025.11.14
 *********************************************************************/

#pragma once
#ifndef COLLISION_H
#define COLLISION_H

#include "../utils/math.h"

namespace engine::component { class ColliderComponent; }

namespace engine::physics::collision {
bool checkCollision(const engine::component::ColliderComponent& a, const engine::component::ColliderComponent& b);
bool checkCircleOverlap(const glm::vec2 & aCenter, const float aRadius, const glm::vec2 & bCenter, const float bRadius);
bool checkAABBOverlap(const glm::vec2& aPosition, const glm::vec2& aSize, const glm::vec2& bPosition, const glm::vec2& bSize);
bool checkRectOverlap(const engine::utils::Rect& a, const engine::utils::Rect& b);
bool checkPointInCircle(const glm::vec2& point, const glm::vec2& center, const float radius);
}

#endif // COLLISION_H