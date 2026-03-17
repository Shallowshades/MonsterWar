/*****************************************************************//**
 * @file   health_component.h
 * @brief  生命组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.12.01
 *********************************************************************/

#pragma once
#ifndef HEALTH_COMPONENT_H
#define HEALTH_COMPONENT_H

#include <string_view>
#include "component.h"

namespace engine::component {
/**
 * @brief 管理游戏对象的生命值, 处理伤害, 治疗, 并提供无敌帧功能.
 */
class HealthComponent final : public engine::component::Component {
	friend class engine::object::GameObject;
public:
	/**
	 * @brief 构造函数.
	 * 
	 * @param maxHealth 最大生命值, 默认为 1
	 * @param invincibilityDuration 无敌状态持续时间, 默认为 2.0 秒
	 */
	explicit HealthComponent(int maxHealth = 1, float invincibilityDuration = 2.f);
	~HealthComponent() override = default;												///< @brief 析构函数

	// 禁用拷贝和移动语义
	HealthComponent(const HealthComponent&) = delete;									///< @brief 删除拷贝构造
	HealthComponent& operator=(const HealthComponent&) = delete;						///< @brief 删除拷贝赋值构造
	HealthComponent(HealthComponent&&) = delete;										///< @brief 删除移动构造
	HealthComponent& operator=(HealthComponent&&) = delete;								///< @brief 删除移动赋值构造

	bool takeDamage(int damageAmount);													///< @brief 伤害游戏对象
	int heal(int healAmount);															///< @brief 治疗游戏对象, 返回治疗后的生命值

	bool isAlive() const;																///< @brief 检查游戏对象是否存活
	bool isInvincible() const;															///< @brief 检查游戏对象是否无敌
	int getCurrentHealth() const;														///< @brief 获取当前生命值
	int getMaxHealth() const;															///< @brief 获取最大生命值
	void setCurrentHealth(int currentHealth);											///< @brief 设置当前生命
	void setMaxHealth(int maxHealth);													///< @brief 设置最大生命
	void setInvincible(float duration);													///< @brief 设置游戏对象无敌状态
	void setInvincibleDuration(float duration);											///< @brief 设置游戏对象无敌状态持续时间

protected:
	// 核心循环函数
	void update(float, engine::core::Context&) override;								///< @brief 更新

private:
	static constexpr std::string_view mLogTag = "HealthComponent";						///< @brief 日志标识
	int mMaxHealth = 1;																	///< @brief 最大生命值
	int mCurrentHealth = 1;																///< @brief 当前生命值
	bool mIsInvincible = false;															///< @brief 是否处于无敌状态
	float mInvincibilityDuration = 2.f;													///< @brief 受伤后无敌的总时长
	float mInvincibilityTimer = 0.f;													///< @brief 无敌时间计时器
};
} // namespace engine::component

#endif // !HEALTH_COMPONENT_H
