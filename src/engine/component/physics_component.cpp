#include "physics_component.h"
#include "transform_component.h"
#include "../object/game_object.h"
#include "../physics/physics_engine.h"
#include <spdlog/spdlog.h>

namespace engine::component {
PhysicsComponent::PhysicsComponent(engine::physics::PhysicsEngine* physicsEngine, bool useGravity, float mass)
	: mPhysicsEngine(physicsEngine), mMass(mass >= 0.f ? mass : 1.f), mUseGravity(useGravity)
{
	if (!mPhysicsEngine) {
		spdlog::error("{} : 构造失败, PhysicsEngine指针为空!", mLogTag.data());
	}
	spdlog::trace("{} : 构造物理组件成功, 质量: {}, 使用重力: {}", mLogTag.data(), mMass, mUseGravity);
}

PhysicsComponent::~PhysicsComponent() = default;

void PhysicsComponent::addForce(const glm::vec2& force) {
	mForce += force;
}

void PhysicsComponent::clearForce(){
	mForce = glm::vec2(0.f);
}

const glm::vec2& PhysicsComponent::getForce() const {
	return mForce;
}

const glm::vec2& PhysicsComponent::getVelocity() const {
	return mVelocity;
}

TransformComponent* PhysicsComponent::getTransform() const {
	return mTransform;
}

float PhysicsComponent::getMass() const {
	return mMass;
}

bool PhysicsComponent::isEnabled() const {
	return mEnbled;
}

bool PhysicsComponent::isUseGravity() const {
	return mUseGravity;
}

void PhysicsComponent::setEnabled(bool enabled) {
	mEnbled = enabled;
}

void PhysicsComponent::setMass(float mass) {
	mMass = mass;
}

void PhysicsComponent::setUseGravity(bool useGravity) {
	mUseGravity = useGravity;
}

void PhysicsComponent::setVelocity(const glm::vec2& velocity) {
	mVelocity = velocity;
}

void PhysicsComponent::resetCollisionFlags() {
	mCollidedBelow = false;
	mCollidedAbove = false;
	mCollidedLeft = false;
	mCollidedRight = false;
	mCollidedLadder = false;
	mIsOnTopLadder = false;
}

void PhysicsComponent::setCollidedBelow(bool collided) {
	mCollidedBelow = collided;
}

void PhysicsComponent::setCollidedAbove(bool collided) {
	mCollidedAbove = collided;
}

void PhysicsComponent::setCollidedLeft(bool collided) {
	mCollidedLeft = collided;
}

void PhysicsComponent::setCollidedRight(bool collided) {
	mCollidedRight = collided;
}

void PhysicsComponent::setCollidedLadder(bool collided) {
	mCollidedLadder = collided;
}

void PhysicsComponent::setOnTopLadder(bool collided) {
	mIsOnTopLadder = collided;
}

bool PhysicsComponent::hasCollidedBelow() const {
	return mCollidedBelow;
}

bool PhysicsComponent::hasCollidedAbove() const {
	return mCollidedAbove;
}

bool PhysicsComponent::hasCollidedLeft() const {
	return mCollidedLeft;
}

bool PhysicsComponent::hasCollidedRight() const {
	return mCollidedRight;
}

bool PhysicsComponent::hasCollidedLadder() const {
	return mCollidedLadder;
}

bool PhysicsComponent::isOnTopLadder() const {
	return mIsOnTopLadder;
}

void PhysicsComponent::init() {
	if (!mOwner) {
		spdlog::error("{} : 物理组件初始化前需要一个GameObject作为所有者!", mLogTag.data());
		return;
	}
	if (!mPhysicsEngine) {
		spdlog::error("{} : PhysicsEngine 为空!", mLogTag.data());
		return;
	}
	mTransform = mOwner->getComponent<TransformComponent>();
	if (!mTransform) {
		spdlog::warn("{} 物理组件初始化时未找到TransformComponent组件.", mLogTag.data());
	}
	// 注册到PhysicsEngine
	mPhysicsEngine->registerComponent(this);
	spdlog::trace("{} : 物理组件初始化完成.", mLogTag.data());
}

void PhysicsComponent::update(float, engine::core::Context&) {}

void PhysicsComponent::clean() {
	mPhysicsEngine->unregisterComponent(this);
	spdlog::trace("{} : 物理组件清理完成.", mLogTag.data());
}
}
