/*****************************************************************//**
 * @file   physics_component.h
 * @brief  物理组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.11.13
 *********************************************************************/

#pragma once
#ifndef PHYSICS_COMPONENT_H
#define PHYSICS_COMPONENT_H

#include <string_view>
#include <glm/vec2.hpp>
#include "component.h"

namespace engine::physics { class PhysicsEngine; }

namespace engine::component {
class TransformComponent;
/**
 * @brief 管理GameObject的物理属性.
 * 
 * 存储速度, 质量, 力和重力设置. 与PhysicsEngine交互.
 */
class PhysicsComponent : public Component {
	friend class engine::object::GameObject;
public:
	/**
	 * @brief 构造函数.
	 * 
	 * @param physicsEngine 指向PhysicsEngine的指针, 不能为nullptr
	 * @param useGravity 物体是否受重力影响, 默认为true
	 * @param mass 物体质量, 默认为1.0
	 */
	PhysicsComponent(engine::physics::PhysicsEngine* physicsEngine, bool useGravity = true, float mass = 1.f);
	~PhysicsComponent();

	// 删除复制/移动操作
	PhysicsComponent(const PhysicsComponent&) = delete;					///< @brief 删除拷贝构造
	PhysicsComponent& operator=(const PhysicsComponent&) = delete;		///< @brief 删除拷贝赋值构造
	PhysicsComponent(PhysicsComponent&&) = delete;						///< @brief 删除移动构造
	PhysicsComponent& operator=(PhysicsComponent&&) = delete;			///< @brief 删除移动赋值构造

	// PhysicsEngine的使用的物理方法
	void addForce(const glm::vec2& force);								///< @brief 添加力
	void clearForce();													///< @brief 清空力
	const glm::vec2& getForce() const;									///< @brief 获取力
	const glm::vec2& getVelocity() const;								///< @brief 获取速度
	TransformComponent* getTransform() const;							///< @brief 获取变换组件指针
	float getMass() const;												///< @brief 获取质量
	bool isEnabled() const;												///< @brief 获取组件是否启用
	bool isUseGravity() const;											///< @brief 获取是否受重力影响
	void setEnabled(bool enabled);										///< @brief 设置组件是否启用
	void setMass(float mass);											///< @brief 设置质量
	void setUseGravity(bool useGravity);								///< @brief 设置组件是否受重力影响
	void setVelocity(const glm::vec2& velocity);						///< @brief 设置速度

	// 碰撞状态访问与修改
	void resetCollisionFlags();											///< @brief 重置所有碰撞标识
	void setCollidedBelow(bool collided);								///< @brief 设置下方碰撞标识
	void setCollidedAbove(bool collided);								///< @brief 设置上方碰撞标识
	void setCollidedLeft(bool collided);								///< @brief 设置左方碰撞标识
	void setCollidedRight(bool collided);								///< @brief 设置右方碰撞标识
	void setCollidedLadder(bool collided);								///< @brief 设置梯子碰撞标识
	void setOnTopLadder(bool collided);									///< @brief 设置是否在梯子顶层

	bool hasCollidedBelow() const;										///< @brief 获取下方碰撞标识
	bool hasCollidedAbove() const;										///< @brief 获取上方碰撞标识
	bool hasCollidedLeft() const;										///< @brief 获取左方碰撞标识
	bool hasCollidedRight() const;										///< @brief 获取右方碰撞标识
	bool hasCollidedLadder() const;										///< @brief 获取梯子碰撞标识
	bool isOnTopLadder() const;											///< @brief 获取是否在梯子顶层

private:
	void init() override;												///< @brief 初始化
	void update(float, engine::core::Context&) override;				///< @brief 更新
	void clean() override;												///< @brief 清理
private:
	static constexpr std::string_view mLogTag = "PhysicsComponent";		///< @brief 日志标识

	engine::physics::PhysicsEngine* mPhysicsEngine = nullptr;			///< @brief 物理引擎的指针
	TransformComponent* mTransform = nullptr;							///< @brief 变换组件的指针
	glm::vec2 mVelocity = glm::vec2(0.f);								///< @brief 当前速度
	glm::vec2 mForce = glm::vec2(0.f);									///< @brief 当前帧受到的力
	float mMass = 1.0f;													///< @brief 物理质量(默认1.0)
	bool mUseGravity = true;											///< @brief 物体是否受重力影响
	bool mEnbled = true;												///< @brief 组件是否激活

	// 碰撞状态标识
	bool mCollidedBelow = false;										///< @brief 底部碰撞
	bool mCollidedAbove = false;										///< @brief 顶部碰撞
	bool mCollidedLeft = false;											///< @brief 左方碰撞
	bool mCollidedRight = false;										///< @brief 右方碰撞
	bool mCollidedLadder = false;										///< @brief 梯子碰撞
	bool mIsOnTopLadder = false;										///< @brief 梯子顶层
};
}

#endif // PHYSICS_COMPONENT_H
