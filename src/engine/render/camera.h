/*****************************************************************//**
 * @file   camera.h
 * @brief  相机类
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.10.20
 *********************************************************************/

#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <optional>
#include <string_view>

#include "../utils/math.h"

namespace engine::component { class TransformComponent; }

namespace engine::render {
/**
 * @brief 相机类负责管理相机位置和视口大小, 并提供坐标转换功能.
 * 包含限制相机移动范围的边界.
 */
class Camera final {
public:
	///< @brief 构造函数
	Camera(const glm::vec2& viewPortSize, const glm::vec2& position = glm::vec2(0), const std::optional<engine::utils::Rect> limitBounds = std::nullopt);

	void update(float deltaTime);										///< @brief 更新相机位置
	void move(const glm::vec2& offset);									///< @brief 移动相机

	glm::vec2 worldToScreen(const glm::vec2& worldPosition) const;		///< @brief 世界坐标转屏幕坐标
	glm::vec2 screenToWorld(const glm::vec2& screenPosition) const;		///< @brief 屏幕坐标转世界坐标

	///< @brief 世界坐标转屏幕坐标, 考虑视差滚动
	glm::vec2 worldToScreenWithParallax(const glm::vec2& worldPosition, const glm::vec2& scrollFactor) const;

	void setPosition(const glm::vec2& position);						///< @brief 设置相机位置
	void setLimitBounds(std::optional<engine::utils::Rect> bounds);				///< @brief 设置限制相机的移动范围
	void setTarget(engine::component::TransformComponent* target);		///< @brief 设置跟随目标变换组件

	const glm::vec2& getPosition() const;								///< @brief 获取相机位置
	std::optional<engine::utils::Rect> getLimitBounds() const;			///< @brief 获取限制相机的移动范围
	glm::vec2 getViewPortSize() const;									///< @brief 获取视口大小
	engine::component::TransformComponent* getTarget() const;			///< @brief 获取跟随目标变换组件

	// 禁用拷贝和移动语义
	Camera(const Camera&) = delete;										///< @brief 删除拷贝构造
	Camera& operator=(const Camera&) = delete;							///< @brief 删除拷贝赋值构造
	Camera(Camera&&) = delete;											///< @brief 删除移动构造
	Camera& operator=(Camera&&) = delete;								///< @brief 删除移动赋值构造

private:
	void clampPosition();												///< @brief 限制相机位置在边界内

private:
	static constexpr std::string_view mLogTag = "Camera";

	glm::vec2 mViewPortSize;											///< @brief 视口大小(屏幕大小)
	glm::vec2 mPosition;												///< @brief 相机左上角的世界坐标
	std::optional<engine::utils::Rect> mLimitBounds;					///< @brief 限制相机的移动范围, 空值表示不限制
	float mSmoothSpeed = 5.f;											///< @brief 相机移动的平滑速度
	engine::component::TransformComponent* mTarget = nullptr;			///< @brief 跟随目标变换组件, 空值表示不跟随
};
}

#endif // CAMERA_H
