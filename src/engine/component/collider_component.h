/*****************************************************************//**
 * @file   collider_component.h
 * @brief  碰撞组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.11.14
 *********************************************************************/

#pragma once
#ifndef COLLIDER_COMPONENT_H
#define COLLIDER_COMPONENT_H

#include "component.h"
#include "../physics/collider.h"
#include "../utils/math.h"
#include "../utils/alignment.h"
#include <memory>
#include <string_view>

namespace engine::component { class TransformComponent; }

namespace engine::component {
/**
 * @brief 碰撞器组件.
 * 
 * 持有Collider对象(如AABBCollider)并提供获取世界坐标系下碰撞形状的方法.
 */
class ColliderComponent final : public Component {
	friend class engine::object::GameObject;
public:
	/**
	 * @brief 构造函数.
	 * 
	 * @param collider 指向碰撞体的独立指针, 所有权将被转移
	 * @param alignment 初始的对齐锚点
	 * @param isTrigger 此碰撞器是否为触发器
	 * @param isActive 此碰撞器是否激活
	 */
	explicit ColliderComponent(std::unique_ptr<engine::physics::Collider> collider, engine::utils::Alignment alignment = engine::utils::Alignment::NONE, bool isTrigger = false, bool isActive = true);

	/**
	 * @brief 根据当前的对齐锚点和碰撞体尺寸计算偏移量.
	 * 
	 * @note 需要用到变换组件缩放大小, 因此变换组件更新缩放大小时也要调用此方法
	 */
	void updateOffset();

	TransformComponent* getTransform() const;							///< @brief 获取变换组件
	const engine::physics::Collider* getCollider() const;					///< @brief 获取碰撞体对象
	const glm::vec2& getOffset() const;										///< @brief 获取当前计算出的偏移量
	engine::utils::Alignment getAlignment() const;							///< @brief 获取设置的对齐锚点
	engine::utils::Rect getWorldAABB() const;								///< @brief 获取世界坐标系下的最小轴对齐包围盒(AABB)
	bool getIsTrigger() const;												///< @brief 获取此碰撞器是否为触发器
	bool getIsActive() const;												///< @brief 获取此碰撞器是否激活

	void setAlignment(engine::utils::Alignment anchor);						///< @brief 设置新的对齐方式并重新计算偏移量
	void setOffset(const glm::vec2& offset);								///< @brief 设置偏移量
	void setTrigger(bool isTrigger);										///< @brief 设置此碰撞器是否为触发器
	void setActive(bool isActive);											///< @brief 设置此碰撞器是否激活

private:
	void init() override;													///< @brief 初始化
	void update(float, engine::core::Context&) override;					///< @brief 更新
private:
	static constexpr std::string_view mLogTag = "ColliderComponent";		///< @brief 日志标识
	TransformComponent* mTransform = nullptr;								///< @brief 变换组件指针
	std::unique_ptr<engine::physics::Collider> mCollider;					///< @brief 碰撞器对象
	glm::vec2 mOffset = glm::vec2(0.f);										///< @brief 碰撞盒(最小包围盒的)左上角相对于变换原点的偏移量
	engine::utils::Alignment mAlignment = engine::utils::Alignment::NONE;	///< @brief 对齐方式
	bool mIsTrigger = false;												///< @brief 是否触发器
	bool mIsActive = true;													///< @brief 是否激活
};
}

#endif // COLLIDER_COMPONENT_H
