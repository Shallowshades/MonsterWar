/*****************************************************************//**
 * @file   tilelayer_component.h
 * @brief  瓦片层
 * @version 1.0
 *
 * @author ShallowShades
 * @date   2025.11.06
 *********************************************************************/

#pragma once
#ifndef TILElAYER_COMPONENT_H
#define TILElAYER_COMPONENT_H

#include "../render/sprite.h"
#include "component.h"
#include <vector>
#include <glm/vec2.hpp>

namespace engine::render { class Sprite; }
namespace engine::core { class Context; }
namespace engine::physics { class PhysicsEngine; }
namespace engine::component {
/**
 * @brief 定义瓦片的类型, 用于游戏逻辑(例如碰撞).
 */
enum class TileType {
	EMPTY,								///< @brief 空白瓦片
	NORMAL,								///< @brief 普通瓦片
	SOLID,								///< @brief 静止可碰撞瓦片
	UNISOLID,							///< @brief 单向静止可碰撞瓦片
	SLOPE_0_1,							///< @brief 斜坡瓦片, 高度: 左0 右1
	SLOPE_1_0,							///< @brief 斜坡瓦片, 高度: 左1 右0
	SLOPE_0_2,							///< @brief 斜坡瓦片, 高度: 左0 右1/2
	SLOPE_2_1,							///< @brief 斜坡瓦片, 高度: 左1/2 右1
	SLOPE_1_2,							///< @brief 斜坡瓦片, 高度: 左1 右1/2
	SLOPE_2_0,							///< @brief 斜坡瓦片, 高度: 左1/2 右0
	HAZARD,								///< @brief 危险瓦片 (例如火焰, 尖刺)
	LADDER,								///< @brief 梯子瓦片
	// TODO: 更多类型
};

/**
 * @brief 包含单个瓦片的渲染和逻辑信息.
 */
struct TileInfo {
	render::Sprite mSprite;				///@brief 瓦片的视觉表示
	TileType mType;						///@brief 瓦片的逻辑类型
	TileInfo(engine::render::Sprite sprite = render::Sprite(), TileType type = TileType::EMPTY) 
		: mSprite(sprite), mType(type) { }
};

/**
 * @brief 管理和渲染瓦片地图层.
 */
class TileLayerComponent final : public Component {
	friend class engine::object::GameObject;
public:
	TileLayerComponent() = default;										///@brief 默认构造

	/**
	 * @brief 构造函数.
	 * 
	 * @param tileSize 单个瓦片尺寸(像素)
	 * @param mapSize 地图尺寸(瓦片数)
	 * @param tiles 初始化瓦片数据的容器(会被移动)
	 */
	TileLayerComponent(glm::ivec2 tileSize, glm::ivec2 mapSize, std::vector<TileInfo>&& tiles);

	/**
	 * @brief 根据瓦片坐标获取瓦片信息.
	 * 
	 * @param position 瓦片坐标 (0 <= x <= mapSize.x, 0 <= y <= mapSize.y)
	 * @return const TileInfo* 指向瓦片信息的指针, 如果坐标无效则返回nullptr
	 */
	const TileInfo* getTileInfoAt(glm::ivec2 position) const;					

	/**
	 * @brief 根据瓦片坐标获取瓦片类型.
	 * 
	 * @param position 瓦片坐标(0 <= x < mapSize.x, 0 <= y < mapSize.y)
	 * @return TileType瓦片类型, 如果坐标无效则返回 TileType::EMPTY
	 */
	TileType getTileTypeAt(glm::ivec2 position) const;

	/**
	 * @brief 根据世界坐标获取瓦片类型.
	 * 
	 * @param worldPosition 世界坐标
	 * @return TileType 瓦片类型, 如果坐标无效则返回 TileType::EMPTY
	 */
	TileType getTileTypeAtWordPosition(const glm::vec2& worldPosition) const;

	glm::ivec2 getTileSize() const;											///< @brief 获取单个瓦片尺寸
	glm::ivec2 getMapSize() const;											///< @brief 获取地图尺寸
	glm::vec2 getWorldSize() const;											///< @brief 获取世界尺寸
	const std::vector<TileInfo>& getTiles() const;							///< @brief 获取瓦片容器
	const glm::vec2& getOffset() const;										///< @brief 瓦片偏移
	bool getIsHidden() const;												///< @brief 获取是否隐藏

	void setOffset(const glm::vec2& offset);								///< @brief 设置瓦片偏移量
	void setHidden(bool hidden);											///< @brief 设置是否隐藏
	void setPhysicsEngine(engine::physics::PhysicsEngine* physicsEngine);	///< @brief 设置物理引擎指针

protected:
	void init() override;													///< @brief 初始化
	void update(float, engine::core::Context&) override;					///< @brief 更新
	void render(engine::core::Context& context) override;					///< @brief 渲染
	void clean() override;													///< @brief 清理

private:
	static constexpr std::string_view mLogTag = "TileLayerComponent";		///< @brief 日志标识

	glm::ivec2 mTileSize;													///< @brief 单个瓦片的尺寸(像素)
	glm::ivec2 mMapSize;													///< @brief 地图尺寸(瓦片数)
	std::vector<TileInfo> mTiles;											///< @brief 存储所有瓦片信息(行主序, index = y * mMapWidth + x)
	glm::vec2 mOffset = glm::vec2(0.f);										///< @brief 瓦片层在世界中的偏移量(瓦片层无需缩放和旋转, 所以不需要Transform组件)
	bool mIsHidden = false;													///< @brief 是否隐藏(不渲染)
	engine::physics::PhysicsEngine* mPhysicsEngine = nullptr;				///< @brief 物理引擎指针, clean 函数要取消注册
};
} // engine::component

#endif // TILElAYER_COMPONENT_H
