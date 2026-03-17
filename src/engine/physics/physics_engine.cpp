#include "physics_engine.h"
#include "collision.h"
#include "../component/physics_component.h"
#include "../component/transform_component.h"
#include "../component/collider_component.h"
#include "../component/tilelayer_component.h"
#include "../object/game_object.h"
#include <set>
#include <spdlog/spdlog.h>
#include <glm/common.hpp>

namespace engine::physics {
void PhysicsEngine::setGravity(const glm::vec2& gravity) {
	mGravity = gravity;
}

const glm::vec2& PhysicsEngine::getGravity() const {
	return mGravity;
}

void PhysicsEngine::setMaxSpeed(float maxSpeed) {
	mMaxSpeed = maxSpeed;
}

float PhysicsEngine::getMaxSpeed() const {
	return mMaxSpeed;
}

void PhysicsEngine::setWorldBound(const engine::utils::Rect& worldBounds) {
	mWorldBounds = worldBounds;
}

const std::optional<engine::utils::Rect>& PhysicsEngine::getWorldBounds() const {
	return mWorldBounds;
}

void PhysicsEngine::registerComponent(engine::component::PhysicsComponent* component) {
	mComponents.push_back(component);
	spdlog::trace("{} : 物理组件注册完成", mLogTag.data());
}

void PhysicsEngine::unregisterComponent(engine::component::PhysicsComponent* component) {
	auto iter = std::remove(mComponents.begin(), mComponents.end(), component);
	mComponents.erase(iter, mComponents.end());
	spdlog::trace("{} : 物理组件注册完成", mLogTag.data());
}

void PhysicsEngine::registerCollisionLayer(engine::component::TileLayerComponent* layer) {
	layer->setPhysicsEngine(this);
	mCollisionTileLayers.push_back(layer);
	spdlog::trace("{} : 碰撞瓦片图层注册完成", mLogTag.data());
}

void PhysicsEngine::unregisterCollisionLayer(engine::component::TileLayerComponent* layer) {
	auto iter = std::remove(mCollisionTileLayers.begin(), mCollisionTileLayers.end(), layer);
	mCollisionTileLayers.erase(iter, mCollisionTileLayers.end());
	spdlog::trace("{} : 碰撞瓦片图层注销完成", mLogTag.data());
}

void PhysicsEngine::update(float delta) {
	// 每帧开始时, 清空碰撞对列表和瓦片触发事件列表
	mCollisionPairs.clear();
	mTileTriggerEvents.clear();

	// 遍历所有注册的物理组件
	for (auto* physicsComponent : mComponents) {
		// 检测组件是否有效且组件是否启用
		if (!physicsComponent || !physicsComponent->isEnabled()) {
			continue;
		}

		// 重置碰撞标识
		physicsComponent->resetCollisionFlags();

		// 应用重力 (如果组件受重力影响) : F = m * g
		if (physicsComponent->isUseGravity()) {
			physicsComponent->addForce(physicsComponent->getMass() * mGravity);
		}
		// TODO: 摩擦力, 风力等

		// 更新速度 : v += a * dt, 其中 a = F / m
		physicsComponent->setVelocity(physicsComponent->getVelocity() + (physicsComponent->getForce() / physicsComponent->getMass()) * delta);
		physicsComponent->clearForce();

		//// 更新位置 :  s += v * delta
		//auto* transform = physicsComponent->getTransform();
		//if (transform) {
		//	transform->translate(physicsComponent->getVelocity() * delta);
		//}

		//// 限制最大速度
		//physicsComponent->setVelocity(glm::clamp(physicsComponent->getVelocity(), -mMaxSpeed, mMaxSpeed));

		// 处理对象间的碰撞
		resolveTileCollisions(physicsComponent, delta);

		// 应用世界边界
		applyWorldBounds(physicsComponent);
	}

	// 处理对象间碰撞
	checkObjectCollisions();

	// 检测瓦片触发事件 (检测前已经处理完位移)
	checkTileTriggers();
}

void PhysicsEngine::checkObjectCollisions() {
	// 两层循环遍历所有包含物理组件的游戏对象
	for (size_t i = 0; i < mComponents.size(); ++i) {
		auto* aPC = mComponents[i];
		if (!aPC || !aPC->isEnabled()) {
			continue;
		}

		auto* aObject = aPC->getOwner();
		if (!aObject) {
			continue;
		}

		auto* aCC = aObject->getComponent<engine::component::ColliderComponent>();
		if (!aCC || !aCC->getIsActive()) {
			continue;
		}

		for (size_t j = i + 1; j < mComponents.size(); ++j) {
			auto* bPC = mComponents[j];
			if (!bPC || !bPC->isEnabled()) {
				continue;
			}

			auto* bObject = bPC->getOwner();
			if (!bObject) {
				continue;
			}

			auto* bCC = bObject->getComponent<engine::component::ColliderComponent>();
			if (!bCC || !bCC->getIsActive()) {
				continue;
			}


			// 保护逻辑结束, 开始执行碰撞检测逻辑
			if (collision::checkCollision(*aCC, *bCC)) {
				// 如果是可移动物体与SOLID物体碰撞, 则直接处理位置变化, 不用记录碰撞对
				if (aObject->getTag() != "solid" && bObject->getTag() == "solid") {
					resolveSolidObjectCollisions(aObject, bObject);
				}
				else if (aObject->getTag() == "solid" && bObject->getTag() != "solid") {
					resolveSolidObjectCollisions(bObject, aObject);
				}
				else {
					mCollisionPairs.emplace_back(aObject, bObject);
				}
			}
		}
	}
}

void PhysicsEngine::checkTileTriggers() {
	for (auto pc : mComponents) {
		if (!pc || !pc->isEnabled()) {
			continue;
		}

		auto obj = pc->getOwner();
		if (!obj) {
			continue;
		}

		auto cc = obj->getComponent<engine::component::ColliderComponent>();
		// 如果游戏对象本就是触发器, 则不需要检查瓦片触发事件
		if (!cc || !cc->getIsActive() || cc->getIsTrigger()) {
			continue;
		}

		// 获取物体的世界AABB
		auto worldAABB = cc->getWorldAABB();
		// 使用set来跟踪循环遍历已经触发过瓦片类型, 防止重复添加 (例如, 玩家同时踩到两个刺, 只需要受一次伤害)
		std::set<engine::component::TileType> triggerSet;
		// 遍历所有注册的碰撞瓦片层分别进行检测
		for (auto layer : mCollisionTileLayers) {
			if (!layer) {
				continue;
			}

			auto tileSize = layer->getTileSize();
			constexpr float tolerance = 1.f;
			// 获取瓦片坐标范围
			auto startX = static_cast<int>(floor(worldAABB.position.x / tileSize.x));
			auto endX = static_cast<int>(ceil((worldAABB.position.x + worldAABB.size.x - tolerance) / tileSize.x));
			auto startY = static_cast<int>(floor(worldAABB.position.y / tileSize.y));
			auto endY = static_cast<int>(ceil((worldAABB.position.y + worldAABB.size.y - tolerance) / tileSize.y));
			// 遍历瓦片坐标范围进行检测
			for (int x = startX; x < endX; ++x) {
				for (int y = startY; y < endY; ++y) {
					auto tileType = layer->getTileTypeAt(glm::vec2(x, y));
					// 未来可以添加更多触发器类型的瓦片, 目前只有HAZARD
					if (tileType == engine::component::TileType::HAZARD) {
						triggerSet.insert(tileType);
					}
					// 梯子类型不必记录到事件容器, 物理引擎自己处理
					else if (tileType == engine::component::TileType::LADDER) {
						pc->setCollidedLadder(true);
					}
				}
			}
			// 遍历触发事件集合, 添加到瓦片触发事件
			for (const auto& type : triggerSet) {
				mTileTriggerEvents.emplace_back(obj, type);
				spdlog::trace("{} : 触发事件数组中添加了游戏对象 {} 和 瓦片触发类型: {}", mLogTag.data(), obj->getName(), static_cast<int>(type));
			}
		}
	}
}

void PhysicsEngine::resolveTileCollisions(engine::component::PhysicsComponent* pc, float delta) {
	// 检查组件是否有效
	auto* object = pc->getOwner();
	if (!object) {
		return;
	}

	auto* tc = object->getComponent<engine::component::TransformComponent>();
	auto* cc = object->getComponent<engine::component::ColliderComponent>();
	if (!tc || !cc || cc->getIsTrigger()) {
		return;
	}
	// 使用最小包围盒进行碰撞检测
	auto worldAABB = cc->getWorldAABB();
	auto objectPosition = worldAABB.position;
	auto objectSize = worldAABB.size;
	if (worldAABB.size.x <= 0.f || worldAABB.size.y <= 0.f) {
		return;
	}

	// 检查右边缘和下边缘时, 需要减一像素, 否则会检查到下一行/列的瓦片
	constexpr float tolerance = 1.f;
	auto ds = pc->getVelocity() * delta;
	auto newObjectPosition = objectPosition + ds;

	// 如果碰撞器未激活, 直接让物体正常移动, 然后返回
	if (!cc->getIsActive()) {
		tc->translate(ds);
		auto velocity = pc->getVelocity();
		velocity = glm::clamp(velocity, -mMaxSpeed, mMaxSpeed);
		pc->setVelocity(velocity);
		return;
	}

	// 遍历所有注册的碰撞瓦片层
	for (auto* layer : mCollisionTileLayers) {
		if (!layer) {
			continue;
		}

		auto tileSize = layer->getTileSize();
		// 轴分离碰撞检测: 
		// 先检查X方向是否有碰撞(Y方向使用初始值ObjectPosition.Y)
		// 轴分离碰撞检测：再检查Y方向是否有碰撞 (x方向使用初始值ObjectPosition.X)

		// 向右移动
		if (ds.x > 0.f) {
			// 检测右侧碰撞, 需要分别测试右上和右下角
			auto rightTopX = newObjectPosition.x + objectSize.x;
			auto tileX = static_cast<int>(floor(rightTopX / tileSize.x));
			// y 方向坐标有两个, 右上和右下
			auto tileY = static_cast<int>(floor(objectPosition.y / tileSize.y));
			auto tileTypeTop = layer->getTileTypeAt(glm::vec2(tileX, tileY));
			auto tileYBottom = static_cast<int>(floor((objectPosition.y + objectSize.y - tolerance) / tileSize.y));
			auto tileTypeBottom = layer->getTileTypeAt(glm::vec2(tileX, tileYBottom));

			if (tileTypeTop == engine::component::TileType::SOLID || tileTypeBottom == engine::component::TileType::SOLID) {
				// 撞墙: X 速度归零, X 方向移动到贴着墙的位置
				newObjectPosition.x = tileX * tileSize.x - objectSize.x;
				pc->setVelocity(glm::vec2(0.f, pc->getVelocity().y));
				pc->setCollidedRight(true);
			}
			else {
				// 检测右下角斜坡瓦片
				auto widthRight = newObjectPosition.x + objectSize.x - tileX * tileSize.x;
				auto heightRight = getTileHeightAtWidth(widthRight, tileTypeBottom, tileSize);
				if (heightRight > 0.f) {
					// 如果有碰撞(角点的世界y坐标 < 斜坡地面的世界y坐标), 就让物体贴着斜坡表面
					if (newObjectPosition.y > (tileYBottom + 1) * layer->getTileSize().y - objectSize.y - heightRight) {
						newObjectPosition.y = (tileYBottom + 1) * layer->getTileSize().y - objectSize.y - heightRight;
						pc->setCollidedBelow(true);
					}
				}
			}
		}

		// 向左移动
		else if (ds.x < 0.f) {
			// 检测左侧碰撞, 需要分别测试左上角和左下角
			auto leftTopX = newObjectPosition.x;
			auto tileX = static_cast<int>(floor(leftTopX / tileSize.x));
			// y 方向坐标有两个, 左上和左下
			auto tileY = static_cast<int>(floor(objectPosition.y / tileSize.y));
			auto tileTypeTop = layer->getTileTypeAt(glm::vec2(tileX, tileY));
			auto tileYBottom = static_cast<int>(floor((objectPosition.y + objectSize.y - tolerance) / tileSize.y));
			auto tileTypeBottom = layer->getTileTypeAt(glm::vec2(tileX, tileYBottom));

			if (tileTypeTop == engine::component::TileType::SOLID || tileTypeBottom == engine::component::TileType::SOLID) {
				// 撞墙: X 速度归零, X 方向移动到贴着墙的位置
				newObjectPosition.x = static_cast<float>((tileX + 1) * tileSize.x);
				pc->setVelocity(glm::vec2(0.f, pc->getVelocity().y));
				pc->setCollidedLeft(true);
			}
			else {
				// 检测左下角斜坡瓦片
				auto widthLeft = newObjectPosition.x - tileX * tileSize.x;
				auto heightLeft = getTileHeightAtWidth(widthLeft, tileTypeBottom, tileSize);
				if (heightLeft > 0.f) {
					// 如果有碰撞(角点的世界y坐标 < 斜坡地面的世界y坐标), 就让物体贴着斜坡表面
					if (newObjectPosition.y > (tileYBottom + 1) * layer->getTileSize().y - objectSize.y - heightLeft) {
						newObjectPosition.y = (tileYBottom + 1) * layer->getTileSize().y - objectSize.y - heightLeft;
						pc->setCollidedBelow(true);
					}
				}
			}
		}

		// 向下移动
		if (ds.y > 0.f) {
			// 检测底部碰撞, 需要分别测试左下和右下角
			auto bottomLeftY = newObjectPosition.y + objectSize.y;
			auto tileY = static_cast<int>(floor(bottomLeftY / tileSize.y));
			// X 方向坐标有两个, 左下和右下
			auto tileX = static_cast<int>(floor(objectPosition.x / tileSize.x));
			auto tileTypeLeft = layer->getTileTypeAt(glm::vec2(tileX, tileY));
			auto tileXRight = static_cast<int>(floor((objectPosition.x + objectSize.x - tolerance) / tileSize.x));
			auto tileTypeRight = layer->getTileTypeAt(glm::vec2(tileXRight, tileY));

			if (tileTypeLeft == engine::component::TileType::SOLID || tileTypeRight == engine::component::TileType::SOLID
				|| tileTypeLeft == engine::component::TileType::UNISOLID || tileTypeRight == engine::component::TileType::UNISOLID) {
				// 触底: Y 速度归零, Y 方向移动到贴着墙的位置
				newObjectPosition.y = tileY * tileSize.y - objectSize.y;
				pc->setVelocity(glm::vec2(pc->getVelocity().x, 0.f));
				pc->setCollidedBelow(true);
			}
			// 如果两个角点都位于梯子上, 则判断是不是处于梯子顶层
			else if (tileTypeLeft == engine::component::TileType::LADDER && tileTypeRight == engine::component::TileType::LADDER) {
				// 检测左角点/右角点上方的瓦片类型
				auto tileTypeUpLeft = layer->getTileTypeAt({ tileX, tileY - 1 });
				auto tileTypeUpRight = layer->getTileTypeAt({ tileXRight, tileY - 1 });
				// 如果上方不是梯子, 证明处于梯子顶层
				if (tileTypeUpRight != engine::component::TileType::LADDER && tileTypeUpLeft != engine::component::TileType::LADDER) {
					// 通过是否使用重力来区分是否处于攀爬状态
					if (pc->isUseGravity()) {
						pc->setOnTopLadder(true);
						pc->setCollidedBelow(true);
						newObjectPosition.y = tileY * layer->getTileSize().y - objectSize.y;
						pc->setVelocity({ pc->getVelocity().x, 0.f });
					}
				}
			}
			else {
				// 检测斜坡瓦片(下方两个角点都要检测)
				auto widthLeft = objectPosition.x - tileX * tileSize.x;
				auto widthRight = objectPosition.x + objectSize.x - tileXRight * tileSize.x;
				auto heightLeft = getTileHeightAtWidth(widthLeft, tileTypeLeft, tileSize);
				auto heightRight = getTileHeightAtWidth(widthRight, tileTypeRight, tileSize);
				auto height = glm::max(heightLeft, heightRight);
				if (height > 0.f) {
					if (newObjectPosition.y > (tileY + 1) * layer->getTileSize().y - objectSize.y - height) {
						newObjectPosition.y = (tileY + 1) * layer->getTileSize().y - objectSize.y - height;
						pc->setVelocity(glm::vec2(pc->getVelocity().x, 0.f));
						pc->setCollidedBelow(true);
					}
				}
			}
		}

		// 向上移动
		else if (ds.y < 0.f) {
			// 检测顶部碰撞, 需要分别测试左上角和右上角
			auto topLeftY = newObjectPosition.y;
			auto tileY = static_cast<int>(floor(topLeftY / tileSize.y));
			// X 方向坐标有两个, 左上和右上
			auto tileX = static_cast<int>(floor(objectPosition.x / tileSize.x));
			auto tileTypeLeft = layer->getTileTypeAt(glm::vec2(tileX, tileY));
			auto tileXRight = static_cast<int>(floor((objectPosition.x + objectSize.x - tolerance) / tileSize.x));
			auto tileTypeRight = layer->getTileTypeAt(glm::vec2(tileXRight, tileY));

			if (tileTypeLeft == engine::component::TileType::SOLID || tileTypeRight == engine::component::TileType::SOLID) {
				// 撞顶: Y 速度归零, Y 方向移动到贴着墙的位置
				newObjectPosition.y = static_cast<float>((tileY + 1) * tileSize.y);
				pc->setVelocity(glm::vec2(pc->getVelocity().x, 0.f));
				pc->setCollidedAbove(true);
			}
			else {
				// TODO: 检测斜坡瓦片(上方两个角点都要检测)
				// 上方为斜坡时, 存在会无视碰撞的问题
			}
		}

		// 更新物体位置, 并限制最大速度
		tc->translate(newObjectPosition - objectPosition); // 使用translate方法, 避免直接设置位置, 因为碰撞盒可能有偏移量
		pc->setVelocity(glm::clamp(pc->getVelocity(), -mMaxSpeed, mMaxSpeed));
	}
}

void PhysicsEngine::resolveSolidObjectCollisions(engine::object::GameObject* moveObject, engine::object::GameObject* solidObject) {
	// 组件的有效性已检测
	auto* moveTC = moveObject->getComponent<engine::component::TransformComponent>();
	auto* movePC = moveObject->getComponent<engine::component::PhysicsComponent>();
	auto* moveCC = moveObject->getComponent<engine::component::ColliderComponent>();
	auto* solidCC = solidObject->getComponent<engine::component::ColliderComponent>();

	// TODO: 轴分离碰撞检测
	// 
	auto moveAABB = moveCC->getWorldAABB();
	auto solidAABB = solidCC->getWorldAABB();
	// 使用最小平移向量解决碰撞问题
	auto moveCenter = moveAABB.position + moveAABB.size / 2.f;
	auto solidCenter = solidAABB.position + solidAABB.size / 2.f;
	// 计算两个包围盒的重叠部分
	auto overlap = glm::vec2(moveAABB.size / 2.f + solidAABB.size / 2.f) - glm::abs(moveCenter - solidCenter);
	if (overlap.x < 0.1f && overlap.y < 0.1f) {
		return;
	}

	if (overlap.x < overlap.y) {
		if (moveCenter.x < solidCenter.x) {
			// 移动物体在左边, 让它贴着右边SOLID物体(相当于向左移出重叠部分), y 方向正常移动
			moveTC->translate(glm::vec2(-overlap.x, 0.f));
			// 如果速度为正(向右移动), 则归零(if 判断不可少, 否则可能出现错误吸附)
			if (movePC->getVelocity().x > 0.f) {
				movePC->setVelocity(glm::vec2(0.f, movePC->getVelocity().y));
				movePC->setCollidedRight(true);
			}
		}
		else {
			// 移动物体在右边, 让它贴着左边SOLID物体(相当于向右移出重叠部分), y 方向正常移动
			moveTC->translate(glm::vec2(overlap.x, 0.f));
			if (movePC->getVelocity().x < 0.f) {
				movePC->setVelocity(glm::vec2(0.f, movePC->getVelocity().y));
				movePC->setCollidedLeft(true);
			}
		}
	}
	else {
		if (moveCenter.y < solidCenter.y) {
			// 移动物体在上面, 让它贴着下面SOLID物体(相当于向上移出重叠部分), x 方向正常移动
			moveTC->translate(glm::vec2(0.f, -overlap.y));
			if (movePC->getVelocity().y > 0.f) {
				movePC->setVelocity(glm::vec2(movePC->getVelocity().x, 0.f));
				movePC->setCollidedBelow(true);
			}
		}
		else {
			// 移动物体在下面, 让它贴着上面SOLID物体(相当于向下移出重叠部分), x 方向正常移动
			moveTC->translate(glm::vec2(0.f, overlap.y));
			if (movePC->getVelocity().y < 0.f) {
				movePC->setVelocity(glm::vec2(movePC->getVelocity().x, 0.f));
				movePC->setCollidedAbove(true);
			}
		}
	}
}

void PhysicsEngine::applyWorldBounds(engine::component::PhysicsComponent* pc) {
	if (!pc || !mWorldBounds) {
		return;
	}

	// 只限定左,上,右边界, 不限定下边界, 以碰撞盒作为判断依据
	auto* object = pc->getOwner();
	auto* cc = object->getComponent<engine::component::ColliderComponent>();
	auto* tc = object->getComponent<engine::component::TransformComponent>();
	auto worldAABB = cc->getWorldAABB();
	auto objectPosition = worldAABB.position;
	auto objectSize = worldAABB.size;

	// 检测左边界
	if (objectPosition.x < mWorldBounds->position.x) {
		pc->setVelocity(glm::vec2(0.f, pc->getVelocity().y));
		objectPosition.x = mWorldBounds->position.x;
		pc->setCollidedLeft(true);
	}

	// 检测上边界
	if (objectPosition.y < mWorldBounds->position.y) {
		pc->setVelocity(glm::vec2(pc->getVelocity().x, 0.f));
		objectPosition.y = mWorldBounds->position.y;
		pc->setCollidedAbove(true);
	}

	// 检测右边界
	if (objectPosition.x + objectSize.x > mWorldBounds->position.x + mWorldBounds->size.x) {
		pc->setVelocity(glm::vec2(0.f, pc->getVelocity().y));
		objectPosition.x = mWorldBounds->position.x + mWorldBounds->size.x - objectSize.x;
		pc->setCollidedRight(true);
	}

	// 更新物体位置(使用translate方法, 新位置 - 旧位置)
	tc->translate(objectPosition - worldAABB.position);
}

float PhysicsEngine::getTileHeightAtWidth(float width, engine::component::TileType type, glm::vec2 tileSize) {
	auto relX = glm::clamp(width / tileSize.x, 0.f, 1.f);
	switch (type) {
	case engine::component::TileType::SLOPE_0_1:
		return relX * tileSize.y;
	case engine::component::TileType::SLOPE_0_2:
		return relX * tileSize.y * 0.5f;
	case engine::component::TileType::SLOPE_2_1:
		return relX * tileSize.y * 0.5f + tileSize.y * 0.5f;
	case engine::component::TileType::SLOPE_1_0:
		return (1.f - relX) * tileSize.y;
	case engine::component::TileType::SLOPE_2_0:
		return (1.f - relX) * tileSize.y * 0.5f;
	case engine::component::TileType::SLOPE_1_2:
		return (1.f - relX) * tileSize.y * 0.5f + tileSize.y * 0.5f;
	default:
		return 0.f;		// 默认返回0, 表示没有斜坡
	}
}
}
