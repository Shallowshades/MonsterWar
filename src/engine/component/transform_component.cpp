#include "transform_component.h"
#include "sprite_component.h"
#include "collider_component.h"
#include "../object/game_object.h"

namespace engine::component {
TransformComponent::TransformComponent(glm::vec2 position, glm::vec2 scale, float rotation) 
	: mPosition(position), mScale(scale), mRotation(rotation)
{
}

const glm::vec2& TransformComponent::getPosition() const {
	return mPosition;
}

float engine::component::TransformComponent::getRotation() const {
	return mRotation;
}

const glm::vec2& TransformComponent::getScale() const {
	return mScale;
}

void TransformComponent::setPosition(const glm::vec2& position) {
	mPosition = position;
}

void TransformComponent::setRotation(float rotation) {
	mRotation = rotation;
}

void TransformComponent::setScale(const glm::vec2& scale) {
	mScale = scale;
	if (mOwner) {
		auto spriteComponent = mOwner->getComponent<SpriteComponent>();
		if (spriteComponent) {
			spriteComponent->updateOffset();
		}

		auto colliderComponent = mOwner->getComponent<ColliderComponent>();
		if (colliderComponent) {
			colliderComponent->updateOffset();
		}
	}
}

void TransformComponent::translate(const glm::vec2& offset) {
	mPosition += offset;
}
} // engine::component
