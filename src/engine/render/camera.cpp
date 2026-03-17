#include "camera.h"
#include "../utils/math.h"
#include "../component/transform_component.h"
#include <spdlog/spdlog.h>

namespace engine::render{
Camera::Camera(const glm::vec2& viewPortSize, const glm::vec2& position, const std::optional<engine::utils::Rect> limitBounds) 
	: mViewPortSize(viewPortSize), mPosition(position), mLimitBounds(limitBounds) 
{
	spdlog::trace("{} 初始化成功, 位置: ({}, {})", mLogTag.data(), std::to_string(mPosition.x), std::to_string(mPosition.y));
}

void Camera::update(float deltaTime) {
	if (!mTarget) {
		return;
	 }

	glm::vec2 targetPosition = mTarget->getPosition();
	glm::vec2 desiredPosition = targetPosition - mViewPortSize / 2.f;

	// 计算当前位置与目标位置的距离
	auto distance = glm::distance(mPosition, desiredPosition);
	constexpr float SNAP_THRESHOLD = 1.f;

	if (distance < SNAP_THRESHOLD) {
		// 如果距离小于阈值, 直接吸附到目标位置
		mPosition = desiredPosition;
	}
	else {
		// 否则, 使用线性插值平滑移动
		// glm::mix(a, b, t) : 在向量a和b之间进行插值, t是插值因子, 范围在0到1之间
		// 公式: (b - a) * t + a; t = 0时结果为a, t = 1时结果为b
		mPosition = glm::mix(mPosition, desiredPosition, mSmoothSpeed * deltaTime);
		mPosition = glm::vec2(glm::round(mPosition.x), glm::round(mPosition.y));
	}

	clampPosition();
}

void Camera::move(const glm::vec2& offset) {
	mPosition += offset;
	clampPosition();
}

glm::vec2 Camera::worldToScreen(const glm::vec2& worldPosition) const {
	return worldPosition - mPosition;
}

glm::vec2 Camera::screenToWorld(const glm::vec2& screenPosition) const {
	return screenPosition + mPosition;
}

glm::vec2 Camera::worldToScreenWithParallax(const glm::vec2& worldPosition, const glm::vec2& scrollFactor) const {
	// 相机位置应用滚动因子
	return worldPosition - mPosition * scrollFactor;
}

void Camera::setPosition(const glm::vec2& position) {
	mPosition = position;
	clampPosition();
}

void Camera::setLimitBounds(std::optional<engine::utils::Rect> bounds) {
	mLimitBounds = bounds;
	clampPosition(); // 设置边界后, 立即应用限制
}

void Camera::setTarget(engine::component::TransformComponent* target) {
	mTarget = target;
}

const glm::vec2& Camera::getPosition() const {
	return mPosition;
}

std::optional<engine::utils::Rect> Camera::getLimitBounds() const {
	return mLimitBounds;
}

glm::vec2 Camera::getViewPortSize() const {
	return mViewPortSize;
}

engine::component::TransformComponent* Camera::getTarget() const {
	return mTarget;
}

void Camera::clampPosition() {
	// 边界检查需要确保相机视图, position 到position+viewPortSize要在limitBounds
	if (mLimitBounds.has_value() && mLimitBounds->size.x > 0 && mLimitBounds->size.y > 0) {
		// 计算允许的相机位置范围
		glm::vec2 minCameraPosition = mLimitBounds->position;
		glm::vec2 maxCameraPosition = mLimitBounds->position + mLimitBounds->size - mViewPortSize;

		// 确保maxCameraPosition不小于minCameraPosition (视口可能比世界还大)
		maxCameraPosition.x = std::max(minCameraPosition.x, maxCameraPosition.x);
		maxCameraPosition.y = std::max(minCameraPosition.y, maxCameraPosition.y);

		mPosition = glm::clamp(mPosition, minCameraPosition, maxCameraPosition);
	}
	// 如果mLimitBounds无效则不进行限制
}
}
