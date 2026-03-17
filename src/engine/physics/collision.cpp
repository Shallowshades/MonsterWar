#include "collision.h"
#include "../component/collider_component.h"
#include "../component/transform_component.h"

namespace engine::physics::collision {
bool checkCollision(const engine::component::ColliderComponent& a, const engine::component::ColliderComponent& b) {
	// 获取两个碰撞盒及对应Transform信息
	auto aCollider = a.getCollider();
	auto bCollider = b.getCollider();
	auto aTransform = a.getTransform();
	auto bTransform = b.getTransform();

	// 先计算最小包围盒是否碰撞, 如果没有碰撞, 那一定是返回false(不考虑AABB的旋转)
	auto aSize = aCollider->getAABBSize() * aTransform->getScale();
	auto bSize = bCollider->getAABBSize() * bTransform->getScale();
	auto aPosition = aTransform->getPosition() + a.getOffset();
	auto bPosition = bTransform->getPosition() + b.getOffset();
	if (!checkAABBOverlap(aPosition, aSize, bPosition, bSize)) {
		return false;
	}

	// 如果最小包围盒有碰撞, 再进行更细致的判断
	// AABB vs AABB
	if (aCollider->getType() == engine::physics::ColliderType::AABB && bCollider->getType() == engine::physics::ColliderType::AABB) {
		return true;
	}

	// Circle vs Circle
	else if (aCollider->getType() == engine::physics::ColliderType::CIRCLE && bCollider->getType() == engine::physics::ColliderType::CIRCLE) {
		auto aCenter = aPosition + 0.5f * aSize;
		auto bCenter = bPosition + 0.5f * bSize;
		auto aRadius = aSize.x * 0.5f;
		auto bRadius = bSize.x * 0.5f;
		return checkCircleOverlap(aCenter, aRadius, bCenter, bRadius);
	}

	// AABB vs Circle
	else if (aCollider->getType() == engine::physics::ColliderType::AABB && bCollider->getType() == engine::physics::ColliderType::CIRCLE) {
		auto bCenter = bPosition + 0.5f * bSize;
		auto bRadius = bSize.x * 0.5f;
		auto nearestPoint = glm::clamp(bCenter, aPosition, aPosition + aSize);
		return checkPointInCircle(nearestPoint, bCenter, bRadius);
	}

	// Circle vs AABB
	else if (aCollider->getType() == engine::physics::ColliderType::CIRCLE && bCollider->getType() == engine::physics::ColliderType::AABB) {
		auto aCenter = aPosition + 0.5f * aSize;
		auto aRadius = aSize.x * 0.5f;
		auto nearestPoint = glm::clamp(aCenter, bPosition, bPosition + bSize);
		return checkPointInCircle(nearestPoint, aCenter, aRadius);
	}

	return false;
}

bool checkCircleOverlap(const glm::vec2 & aCenter, const float aRadius, const glm::vec2 & bCenter, const float bRadius) {
	return (glm::length(aCenter - bCenter) < aRadius + bRadius);
}

bool checkAABBOverlap(const glm::vec2& aPosition, const glm::vec2& aSize, const glm::vec2& bPosition, const glm::vec2& bSize) {
	if (aPosition.x + aSize.x <= bPosition.x || aPosition.x >= bPosition.x + bSize.x ||
		aPosition.y + aSize.y <= bPosition.y || aPosition.y >= bPosition.y + bSize.y) {
		return false;
	}
	
	return true;
}

bool checkRectOverlap(const engine::utils::Rect & a, const engine::utils::Rect & b) {
	return checkAABBOverlap(a.position, a.size, b.position, b.size);
}

bool checkPointInCircle(const glm::vec2 & point, const glm::vec2 & center, const float radius) {
	return (glm::length(point - center) < radius);
}
}
