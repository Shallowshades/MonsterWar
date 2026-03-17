/*****************************************************************//**
 * @file   collider.h
 * @brief  碰撞体
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2025.11.14
 *********************************************************************/

#pragma once
#ifndef COLLIDER_H
#define COLLIDER_H

#include <string_view>
#include <glm/vec2.hpp>

namespace engine::physics {
/**
* @brief 定义不同类型的碰撞器
*/
enum class ColliderType {
	NONE,
	AABB,
	CIRCLE,
	// TODO: more
};

/**
 * @brief 碰撞器的抽象基类.
 * 
 * 所有具体的碰撞器都应该继承此类
 */
class Collider {
public:
	virtual ~Collider() = default;									///< @brief 默认构造函数
	virtual ColliderType getType() const = 0;						///< @brief 纯虚函数, 获取碰撞器类型

	void setAABBSize(const glm::vec2& size);						///< @brief 设置最小包围盒的尺寸(宽度和高度)
	const glm::vec2& getAABBSize() const;							///< @brief 获取最小包围盒的尺寸(宽度和高度)
protected:
	glm::vec2 mAABBSize = glm::vec2(0.f);							///< @brief 覆盖Collider的最小包围盒的尺寸（宽度和高度）。
};

/**
 * @brief 轴对齐包围盒(Axis-Aligned Bounding Box) 碰撞器.
 */
class AABBCollider final : public Collider {
public:
	explicit AABBCollider(const glm::vec2& size);					///< @brief 构造函数
	~AABBCollider() override = default;								///< @brief 重写析构

	ColliderType getType() const override;							///< @brief 获取碰撞盒类型
	const glm::vec2& getSize() const;								///< @brief 获取包围盒的寸尺
	void setSize(const glm::vec2& size);							///< @brief 设置包围盒尺寸
private:
	glm::vec2 mSize = glm::vec2(0.f);								///< @brief 包围盒的尺寸(和基类的AABBSize相同)
};

/**
 * @brief 圆形碰撞器.
 */
class CircleCollider final : public Collider {
public:
	explicit CircleCollider(float radius);							///< @brief 构造函数
	~CircleCollider() override = default;							///< @brief 重写析构

	ColliderType getType() const override;							///< @brief 获取碰撞盒类型
	float getRadius() const;										///< @brief 获取半径
	void setRadius(float radius);									///< @brief 设置半径
private:
	float mRadius = 0.f;											///< @brief 圆形碰撞盒半径
};
}

#endif // COLLIDER_H
