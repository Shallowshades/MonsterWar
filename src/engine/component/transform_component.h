/*****************************************************************//**
 * @file   transform_component.h
 * @brief  变换组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.10.23
 *********************************************************************/

#pragma once
#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include <glm/vec2.hpp>
#include "component.h"

namespace engine::component {
/**
 * @class TransformComponent.
 * @brief 管理GameObject的位置,旋转和缩放.
 */
class TransformComponent final : public Component {
	friend class engine::object::GameObject;
public:
	/**
	* @brief 构造函数.
	*
	* @param position 位置.
	* @param scale 缩放.
	* @param rotation 旋转.
	*/
	TransformComponent(glm::vec2 position = { 0.f, 0.f }, glm::vec2 scale = { 1.f, 1.f }, float rotation = 0.f);

	// 禁止拷贝和移动
	TransformComponent(const TransformComponent&) = delete;						///< @brief 删除拷贝构造
	TransformComponent& operator=(const TransformComponent&) = delete;			///< @brief 删除拷贝赋值构造
	TransformComponent(TransformComponent&&) = delete;							///< @brief 删除移动构造
	TransformComponent& operator=(TransformComponent&&) = delete;				///< @brief 删除移动赋值构造

	const glm::vec2& getPosition() const;										///< @brief 获取位置
	float getRotation() const;													///< @brief 获取旋转
	const glm::vec2& getScale() const;											///< @brief 获取缩放
	void setPosition(const glm::vec2& position);								///< @brief 设置位置
	void setRotation(float rotation);											///< @brief 设置旋转
	void setScale(const glm::vec2& scale);										///< @brief 设置缩放, 应用缩放时应同步更新Sprite偏移量

	void translate(const glm::vec2& offset);									///< @brief 平移

private:
	void update(float, engine::core::Context&) override {};						///< @brief 覆盖纯虚函数, 无需实现
public:
	glm::vec2 mPosition = { 0.f, 0.f };											///< @brief 位置
	glm::vec2 mScale = { 1.f, 1.f };											///< @brief 缩放
	float mRotation = 1.f;														///< @brief 角度制, 单位:度
};
} // engine::component																				
																				
#endif // TRANSFORM_COMPONENT_H
