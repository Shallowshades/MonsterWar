#include "collider_component.h"
#include "transform_component.h"
#include "../object/game_object.h"
#include "../physics/collider.h"
#include <spdlog/spdlog.h>

namespace engine::component {
ColliderComponent::ColliderComponent(std::unique_ptr<engine::physics::Collider> collider, engine::utils::Alignment alignment, bool isTrigger, bool isActive)
	: mCollider(std::move(collider))
	, mAlignment(alignment)
	, mIsTrigger(isTrigger)
	, mIsActive(isActive)
{
	if (!mCollider) {
		spdlog::error("{} : 初始化失败, 碰撞体指针为空", mLogTag.data());
	}
}

void ColliderComponent::updateOffset() {
	if (!mCollider) {
		return;
	}

	// 获取碰撞盒的最小包围盒尺寸
	auto colliderSize = mCollider->getAABBSize();

	// 如果尺寸无效, 偏移量为零
	if (colliderSize.x <= 0.f || colliderSize.y <= 0.f) {
		mOffset = glm::vec2(0.f);
		return;
	}

	auto scale = mTransform->getScale();
	// 根据对齐锚点计算AABB左上角相对于变换组件中心的偏移量
	switch (mAlignment) {
	case engine::utils::Alignment::TOP_LEFT:
		mOffset = glm::vec2(0.f, 0.f) * scale;
		break;
	case engine::utils::Alignment::TOP_CENTER:
		mOffset = glm::vec2(-colliderSize.x / 2.f, 0.f) * scale;
		break;
	case engine::utils::Alignment::TOP_RIGHT:
		mOffset = glm::vec2(-colliderSize.x, 0.f) * scale;
		break;
	case engine::utils::Alignment::CENTER_LEFT:
		mOffset = glm::vec2(0.f, -colliderSize.y / 2.f) * scale;
		break;
	case engine::utils::Alignment::CENTER:
		mOffset = glm::vec2(-colliderSize.x / 2.f, -colliderSize.y / 2.f) * scale;
		break;
	case engine::utils::Alignment::CENTER_RIGHT:
		mOffset = glm::vec2(-colliderSize.x, -colliderSize.y / 2.f) * scale;
		break;
	case engine::utils::Alignment::BOTTOM_LEFT:
		mOffset = glm::vec2(0.f, -colliderSize.y) * scale;
		break;
	case engine::utils::Alignment::BOTTOM_CENTER:
		mOffset = glm::vec2(-colliderSize.x / 2.f, -colliderSize.y) * scale;
		break;
	case engine::utils::Alignment::BOTTOM_RIGHT:
		mOffset = glm::vec2(-colliderSize.x, -colliderSize.y) * scale;
		break;
	default:
		break;
	}
}

TransformComponent* ColliderComponent::getTransform() const {
	return mTransform;
}

const engine::physics::Collider* ColliderComponent::getCollider() const {
	return mCollider.get();
}

const glm::vec2& ColliderComponent::getOffset() const {
	return mOffset;
}

engine::utils::Alignment ColliderComponent::getAlignment() const {
	return mAlignment;
}

engine::utils::Rect ColliderComponent::getWorldAABB() const {
	if (!mTransform || !mCollider) {
		return engine::utils::Rect{ glm::vec2(0.f), glm::vec2(0.f) };
	}

	// 计算最小包围盒的左上角坐标
	const glm::vec2 topLeftPosition = mTransform->getPosition() + mOffset;
	// 计算最小包围盒的尺寸
	const glm::vec2 baseSize = mCollider->getAABBSize();
	const glm::vec2 scale = mTransform->getScale();
	glm::vec2 scaledSize = baseSize * scale;
	return engine::utils::Rect{ topLeftPosition, scaledSize };
}

bool ColliderComponent::getIsTrigger() const {
	return mIsTrigger;
}

bool ColliderComponent::getIsActive() const {
	return mIsActive;
}

void ColliderComponent::setAlignment(engine::utils::Alignment anchor) {
	mAlignment = anchor;
	// 重新计算偏移量, 确保变换组件和碰撞体有效
	if (mTransform && mCollider) {
		updateOffset();
	}
}

void ColliderComponent::setOffset(const glm::vec2& offset) {
	mOffset = offset;
}

void ColliderComponent::setTrigger(bool isTrigger) {
	mIsTrigger = isTrigger;
}

void ColliderComponent::setActive(bool isActive) {
	mIsActive = isActive;
}

void ColliderComponent::init() {
	if (!mOwner) {
		spdlog::error("{} : 该组件的所有者指针为空", mLogTag.data());
		return;
	}

	mTransform = mOwner->getComponent<TransformComponent>();
	if (!mTransform) {
		spdlog::error("{} : 需要一个在同一个游戏对象上的变换组件", mLogTag.data());
		return;
	}

	// 获取变换组件后计算初始偏移量
	updateOffset();
}

void ColliderComponent::update(float, engine::core::Context&) {}
}
