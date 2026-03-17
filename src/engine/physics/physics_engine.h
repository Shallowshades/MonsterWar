/*****************************************************************//**
 * @file   physics_engine.h
 * @brief  物理引擎
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2025.11.13
 *********************************************************************/

#pragma once
#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

#include <string>
#include <vector>
#include <utility>
#include <optional>
#include <glm/vec2.hpp>
#include "../utils/math.h"

namespace engine::component { 
	class PhysicsComponent; 
	class TileLayerComponent;
	enum class TileType;
}
namespace engine::object { class GameObject; }

namespace engine::physics {
/**
* @brief 负责管理和模拟物理行为及碰撞检测.
*/
class PhysicsEngine {
public:
	PhysicsEngine() = default;															///< @brief 默认构造

	// 禁止拷贝和移动
	PhysicsEngine(const PhysicsEngine&) = delete;										///< @brief 删除拷贝构造
	PhysicsEngine& operator=(const PhysicsEngine&) = delete;							///< @brief 删除拷贝赋值构造
	PhysicsEngine(PhysicsEngine&&) = delete;											///< @brief 删除移动构造
	PhysicsEngine& operator=(PhysicsEngine&&) = delete;									///< @brief 删除移动赋值构造

	void setGravity(const glm::vec2& gravity);											///< @brief 设置重力
	const glm::vec2& getGravity() const;												///< @brief 获取重力
	void setMaxSpeed(float maxSpeed);													///< @brief 设置最大速度
	float getMaxSpeed() const;															///< @brief 获取最大速度
	void setWorldBound(const engine::utils::Rect& worldBounds);							///< @brief 设置世界边界
	const std::optional<engine::utils::Rect>& getWorldBounds() const;					///< @brief 获取世界边界
	const auto& getCollisionPairs() const { return mCollisionPairs; }					///< @brief 获取本帧检测到的所有游戏对象碰撞对
	const auto& getTileTriggerEvents() const { return mTileTriggerEvents; }					///< @brief 获取本帧检测到的所有游戏对象碰撞对

	void registerComponent(engine::component::PhysicsComponent* component);				///< @brief 注册组件
	void unregisterComponent(engine::component::PhysicsComponent* component);			///< @brief 移除组件

	void registerCollisionLayer(engine::component::TileLayerComponent* layer);			///< @brief 注册用于碰撞检测的瓦片组件
	void unregisterCollisionLayer(engine::component::TileLayerComponent* layer);		///< @brief 注销用于碰撞检测的瓦片组件

	void update(float delta);															///< @brief 更新
	
private:
	void checkObjectCollisions();														///< @brief 检测并处理对象之间的碰撞, 并记录需要游戏逻辑处理的碰撞对
	void checkTileTriggers();															///< @brief 检测所有游戏对象与瓦片层的触发器类型瓦片碰撞, 并记录触发事件. (位移处理完毕后再调用)
	void resolveTileCollisions(engine::component::PhysicsComponent* pc, float delta);	///< @brief 检测并处理游戏对象和瓦片层之间的碰撞
	
	///< @brief 处理移动物体与SOLID物体的碰撞
	void resolveSolidObjectCollisions(engine::object::GameObject* moveObject, engine::object::GameObject* solidObject);
	void applyWorldBounds(engine::component::PhysicsComponent* pc);

	/**
	 * @brief 根据瓦片类型和指定宽度X坐标, 计算瓦片上对应Y坐标.
	 * 
	 * @param width 从瓦片左侧起算的宽度
	 * @param type 瓦片类型
	 * @param tileSize 瓦片尺寸
	 * @return 瓦片上对应高度(从瓦片下侧起算)
	 */
	float getTileHeightAtWidth(float width, engine::component::TileType type, glm::vec2 tileSize);
private:
	static constexpr std::string_view mLogTag = "PhysicsEngine";						///< @brief 日志标识
																						
	std::vector<engine::component::PhysicsComponent*> mComponents;						///< @brief 在物理系统中注册的组件数组
	std::vector<engine::component::TileLayerComponent*> mCollisionTileLayers;			///< @brief 注册的碰撞瓦片图层容器
	glm::vec2 mGravity = { 0.f, 980.f };												///< @brief 重力加速度
	float mMaxSpeed = 500.f;															///< @brief 最大速度限值
	std::optional<engine::utils::Rect> mWorldBounds;									///< @brief 世界边界, 用于限制物体移动范围

	// @brief 存储本帧发生的GameObject碰撞对(每次update开始时清空)
	std::vector<std::pair<engine::object::GameObject*, engine::object::GameObject*>> mCollisionPairs;
	// @brief 存储本帧发生发生的瓦片触发事件 (游戏对象指针, 触发瓦片类型, 每次更新时清空)
	std::vector<std::pair<engine::object::GameObject*, engine::component::TileType>> mTileTriggerEvents;
};
}

#endif // PHYSICS_ENGINE_H
