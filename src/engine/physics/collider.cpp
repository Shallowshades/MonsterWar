#include "collider.h"

namespace engine::physics {
void Collider::setAABBSize(const glm::vec2& size) {
	mAABBSize = size;
}

const glm::vec2& Collider::getAABBSize() const {
	return mAABBSize;
}

AABBCollider::AABBCollider(const glm::vec2& size) 
	: mSize(size) 
{
	setAABBSize(size);
}

ColliderType AABBCollider::getType() const {
	return ColliderType::AABB;
}

const glm::vec2& AABBCollider::getSize() const {
	return mSize;
}

void AABBCollider::setSize(const glm::vec2& size) {
	mSize = size;
}

CircleCollider::CircleCollider(float radius) 
	: mRadius(radius)
{
	setAABBSize(glm::vec2(mRadius * 2.f, mRadius * 2.f));
}

ColliderType CircleCollider::getType() const {
	return ColliderType::CIRCLE;
}

float CircleCollider::getRadius() const {
	return mRadius;
}

void CircleCollider::setRadius(float radius) {
	mRadius = radius;
}
} // engine::physics
