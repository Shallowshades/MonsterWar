#include "health_component.h"
#include "../object/game_object.h"
#include <spdlog/spdlog.h>
#include <glm/common.hpp>

namespace engine::component {
HealthComponent::HealthComponent(int maxHealth, float invincibilityDuration) 
	: mMaxHealth(glm::max(1, maxHealth))
	, mCurrentHealth(maxHealth)
	, mInvincibilityDuration(invincibilityDuration)
{
}

bool HealthComponent::takeDamage(int damageAmount) {
	// 伤害量错误或者已经死亡
	if (damageAmount <= 0 || !isAlive()) {
		return false;
	}

	if (mIsInvincible) {
		spdlog::debug("{} : 游戏对象 '{}' 处于无敌状态, 免疫了{}点伤害.", mLogTag.data(), (mOwner ? mOwner->getName() : "unknown"), damageAmount);
		return false;
	}

	// 造成伤害
	mCurrentHealth -= damageAmount;
	mCurrentHealth = std::max(0, mCurrentHealth);

	// 如果受伤但没死, 并且设置无敌时间, 则触发无敌
	if (isAlive() && mInvincibilityDuration > 0.f) {
		setInvincible(mInvincibilityDuration);
	}
	spdlog::debug("{} : 游戏对象 '{}' 收到了 {} 点伤害, 当前生命值: {}/{}", mLogTag.data(), (mOwner ? mOwner->getName() : "unknown"), damageAmount, mCurrentHealth, mMaxHealth);
	return true;
}

int HealthComponent::heal(int healAmount) {
	// 治疗量错误或者已经死亡
	if (healAmount <= 0 || !isAlive()) {
		return mCurrentHealth;
	}

	mCurrentHealth += healAmount;
	mCurrentHealth = std::min(mMaxHealth, mCurrentHealth);
	spdlog::debug("{} : 游戏对象 '{}' 治疗了 {} 点生命, 当前生命值: {}/{}", mLogTag.data(), (mOwner ? mOwner->getName() : "unknown"), healAmount, mCurrentHealth, mMaxHealth);
	return mCurrentHealth;
}

bool HealthComponent::isAlive() const {
	return mCurrentHealth > 0;
}

bool HealthComponent::isInvincible() const {
	return mIsInvincible;
}

int HealthComponent::getCurrentHealth() const {
	return mCurrentHealth;
}

int HealthComponent::getMaxHealth() const {
	return mMaxHealth;
}

void HealthComponent::setCurrentHealth(int currentHealth) {
	mCurrentHealth = std::max(0, std::min(currentHealth, mMaxHealth));
}

void HealthComponent::setMaxHealth(int maxHealth) {
	mMaxHealth = std::max(1, maxHealth);
	mCurrentHealth = std::min(mCurrentHealth, mMaxHealth);
}

void HealthComponent::setInvincible(float duration) {
	if (duration > 0.f) {
		mIsInvincible = true;
		mInvincibilityTimer = duration;
		spdlog::debug("{} : 游戏对象 '{}' 进入无敌状态, 持续 {} 秒.", mLogTag.data(), (mOwner ? mOwner->getName() : "unknown"), duration);
	}
	else {
		// 如果持续时间为 0 或 负数, 则立即取消无敌
		mIsInvincible = false;
		mInvincibilityTimer = 0.f;
		spdlog::debug("{} : 游戏对象 '{}' 无敌状态被手动移除.", mLogTag.data(), (mOwner ? mOwner->getName() : "unknown"));
	}
}

void HealthComponent::setInvincibleDuration(float duration) {
	mInvincibilityDuration = duration;
}

void HealthComponent::update(float delta, engine::core::Context&) {
	// 更新无敌状态计时器
	if (mIsInvincible) {
		mInvincibilityTimer -= delta;
		if (mInvincibilityTimer <= 0.f) {
			mIsInvincible = false;
			mInvincibilityTimer = 0.f;
		}
	}
}
} // namespace engine::component
