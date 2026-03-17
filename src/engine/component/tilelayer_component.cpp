#include "tilelayer_component.h"
#include "../object/game_object.h"
#include "../core/context.h"
#include "../render/renderer.h"
#include "../render/camera.h"
#include "../physics/physics_engine.h"
#include <spdlog/spdlog.h>

namespace engine::component {
engine::component::TileLayerComponent::TileLayerComponent(glm::ivec2 tileSize, glm::ivec2 mapSize, std::vector<TileInfo>&& tiles) 
	: mTileSize(tileSize)
	, mMapSize(mapSize)
	, mTiles(std::move(tiles))
{
	if (mTiles.size() != static_cast<size_t>(mMapSize.x * mMapSize.y)) {
		spdlog::error("{} : 地图尺寸与提供的瓦片向量大小不匹配. 瓦片数据将被清除.", mLogTag.data());
		mTiles.clear();
		mMapSize = { 0, 0 };
	}
	spdlog::trace("{} 构造完成", mLogTag.data());
}

const TileInfo* TileLayerComponent::getTileInfoAt(glm::ivec2 position) const {
	if (position.x < 0 || position.x >= mMapSize.x || position.y < 0 || position.y >= mMapSize.y) {
		spdlog::warn("{} : 瓦片坐标越界: ({}, {})", mLogTag.data(), position.x, position.y);
		return nullptr;
	}

	size_t index = static_cast<size_t>(position.y * mMapSize.x + position.x);

	// 瓦片索引不能越界
	if (index < mTiles.size()) {
		return &mTiles[index];
	}

	spdlog::warn("{} 瓦片索引越界: {}", mLogTag.data(), index);
	return nullptr;
}

TileType TileLayerComponent::getTileTypeAt(glm::ivec2 position) const {
	const TileInfo* info = getTileInfoAt(position);
	return info ? info->mType : TileType::EMPTY;
}

TileType TileLayerComponent::getTileTypeAtWordPosition(const glm::vec2& worldPosition) const {
	glm::vec2 relativePosition = worldPosition - mOffset;
	int tileX = static_cast<int>(std::floor(relativePosition.x / mTileSize.x));
	int tileY = static_cast<int>(std::floor(relativePosition.y / mTileSize.y));
	return getTileTypeAt(glm::ivec2(tileX, tileY));
}

glm::ivec2 TileLayerComponent::getTileSize() const {
	return mTileSize;
}

glm::ivec2 TileLayerComponent::getMapSize() const {
	return mMapSize;
}

glm::vec2 TileLayerComponent::getWorldSize() const {
	return glm::vec2(mMapSize.x * mTileSize.x, mMapSize.y * mTileSize.y);
}

const std::vector<TileInfo>& TileLayerComponent::getTiles() const {
	return mTiles;
}

const glm::vec2& TileLayerComponent::getOffset() const {
	return mOffset;
}

bool TileLayerComponent::getIsHidden() const {
	return mIsHidden;
}

void TileLayerComponent::setOffset(const glm::vec2& offset) {
	mOffset = offset;
}

void TileLayerComponent::setHidden(bool hidden) {
	mIsHidden = hidden;
}

void TileLayerComponent::setPhysicsEngine(engine::physics::PhysicsEngine* physicsEngine) {
	mPhysicsEngine = physicsEngine;
}

void TileLayerComponent::init() {
	if (!mOwner) {
		spdlog::warn("{} 的owner未设置.", mLogTag.data());
	}
	spdlog::trace("{} 初始化完成", mLogTag.data());
}

void TileLayerComponent::update(float, engine::core::Context&) {}

void TileLayerComponent::render(engine::core::Context& context) {
	if (mTileSize.x <= 0 || mTileSize.y <= 0) {
		return;
	}
	for (int y = 0; y < mMapSize.y; ++y) {
		for (int x = 0; x < mMapSize.x; ++x) {
			size_t index = static_cast<size_t>(y) * mMapSize.x + x;
			// 检查索引有效性以及瓦片是否需要渲染
			if (index < mTiles.size() && mTiles[index].mType != TileType::EMPTY) {
				const auto& tileInfo = mTiles[index];
				glm::vec2 tileLeftTopPosition = {
					mOffset.x + static_cast<float>(x) * mTileSize.x,
					mOffset.y + static_cast<float>(y) * mTileSize.y,
				};

				// 如果图片大小与瓦片大小不一致, 需要调整y坐标(瓦片层的对齐点时左下角)
				// 绘制起始点为左上角, y 轴向反方向移动源矩阵和瓦片大小的y 轴差值
				if (static_cast<int>(tileInfo.mSprite.getSourceRect()->h) != mTileSize.y) {
					tileLeftTopPosition.y -= (tileInfo.mSprite.getSourceRect()->h - static_cast<float>(mTileSize.y));
				}

				// 执行绘制
				context.getRenderer().drawSprite(context.getCamera(), tileInfo.mSprite, tileLeftTopPosition);
			}
		}
	}
}

void TileLayerComponent::clean() {
	if (mPhysicsEngine) {
		mPhysicsEngine->unregisterCollisionLayer(this);
	}
}
} // engine::component
