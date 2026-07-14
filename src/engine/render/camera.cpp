#include "camera.h"
#include <spdlog/spdlog.h>
#include <glm/common.hpp>

namespace engine::render
{
	Camera::Camera(glm::vec2 viewport_size, glm::vec2 position, std::optional<engine::utils::Rect> limit_bounds)
		: mViewportSize(std::move(viewport_size)), mPosition(std::move(position)), mLimitBounds(std::move(limit_bounds)) {
		spdlog::trace("Camera 初始化成功，位置: {},{}", mPosition.x, mPosition.y);
	}

	void Camera::setPosition(glm::vec2 position) {
		mPosition = std::move(position);
		clampPosition();
	}

	void Camera::move(const glm::vec2& offset)
	{
		mPosition += offset;
		clampPosition();
	}

	void Camera::setLimitBounds(std::optional<engine::utils::Rect> limit_bounds)
	{
		mLimitBounds = std::move(limit_bounds);
		clampPosition(); // 设置边界后，立即应用限制
	}

	const glm::vec2& Camera::getPosition() const {
		return mPosition;
	}

	void Camera::clampPosition()
	{
		// 边界检查需要确保相机视图（position 到 position + viewport_size）在 limit_bounds 内
		if (mLimitBounds.has_value() && mLimitBounds->mSize.x > 0 && mLimitBounds->mSize.y > 0) {
			// 计算允许的相机位置范围
			glm::vec2 min_cam_pos = mLimitBounds->mPosition;
			glm::vec2 max_cam_pos = mLimitBounds->mPosition + mLimitBounds->mSize - mViewportSize;

			// 确保 max_cam_pos 不小于 min_cam_pos (视口可能比世界还大)
			max_cam_pos.x = std::max(min_cam_pos.x, max_cam_pos.x);
			max_cam_pos.y = std::max(min_cam_pos.y, max_cam_pos.y);

			mPosition = glm::clamp(mPosition, min_cam_pos, max_cam_pos);
		}
		// 如果 limit_bounds 无效则不进行限制
	}

	glm::vec2 Camera::worldToScreen(const glm::vec2& world_pos) const {
		// 将世界坐标减去相机左上角位置
		return world_pos - mPosition;
	}

	glm::vec2 Camera::worldToScreenWithParallax(const glm::vec2& world_pos, const glm::vec2& scroll_factor) const
	{
		// 相机位置应用滚动因子
		return world_pos - mPosition * scroll_factor;
	}

	glm::vec2 Camera::screenToWorld(const glm::vec2& screen_pos) const
	{
		// 将屏幕坐标加上相机左上角位置
		return screen_pos + mPosition;
	}

	glm::vec2 Camera::getViewportSize() const {
		return mViewportSize;
	}

	std::optional<engine::utils::Rect> Camera::getLimitBounds() const {
		return mLimitBounds;
	}
}