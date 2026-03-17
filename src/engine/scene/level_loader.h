/*****************************************************************//**
 * @file   level_loader.h
 * @brief  关卡载入器
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.11.05
 *********************************************************************/

#pragma once
#ifndef LEVEL_LOADER_H
#define LEVEL_LOADER_H

#include <map>
#include <string>
#include <string_view>
#include <optional>
#include <glm/vec2.hpp>
#include <nlohmann/json.hpp>
#include "../utils/math.h"

namespace engine::component { 
	struct TileInfo; 
	enum class TileType;
	class AnimationComponent;
	class AudioComponent;
}

namespace engine::scene {
class Scene;
/**
 * @brief 关卡载入器类.
 */
class LevelLoader final {
public:
	LevelLoader() = default;

	/**
	 * @brief 加载关卡数据到指定的Scene对象中.
	 * 
	 * @param mapPath Tiled JSON地图文件的路径
	 * @param scene 要加载数据的目标 Scene 对象
	 * @return bool 是否加载成功
	 */
	[[nodiscard]] bool loadLevel(std::string_view mapPath, Scene& scene);

private:
	void loadImageLayer(const nlohmann::json& layerJson, Scene& scene);		///< @brief 加载图片图层
	void loadTileLayer(const nlohmann::json& layerJson, Scene& scene);		///< @brief 加载瓦片图层
	void loadObjectLayer(const nlohmann::json& layerJson, Scene& scene);	///< @brief 加载对象图层
	
	/**
	 * @brief 添加动画到指定的AnimationComponent.
	 * 
	 * @param animationJson 动画Json数据(自定义)
	 * @param ac 动画组件指针 (动画添加到此组件)
	 * @param spriteSize 每一帧动画的尺寸
	 */
	void addAnimation(const nlohmann::json& animationJson, engine::component::AnimationComponent* ac, const glm::vec2& spriteSize);

	/**
	 * @brief 添加音效到指定的AudioComponent.
	 *
	 * @param soundJson 音效JSON
	 * @param audioComponent 音频组件指针
	 */
	void addSound(const nlohmann::json& soundJson, engine::component::AudioComponent* audioComponent);

	/**
	 * @brief 获取瓦片属性.
	 * 
	 * @param tileJson 瓦片json数据
	 * @param propertyName 属性名称
	 * @return 属性值, 如果属性不存在则返回 std::nullopt
	 */
	template<typename T>
	std::optional<T> getTileProperty(const nlohmann::json& tileJson, std::string_view propertyName);

	/**
	 * @brief 获取瓦片碰撞器矩形.
	 * @param tileJson 瓦片json数据
	 * @return 碰撞器矩形, 如果碰撞器不存在则返回std::nullopt
	 */
	std::optional<engine::utils::Rect> getColliderRect(const nlohmann::json& tileJson);

	/**
	 * @brief 根据瓦片json对象获取瓦片类型.
	 * 
	 * @param tileJson瓦片json数据
	 * @return 瓦片类型
	 */
	engine::component::TileType getTileType(const nlohmann::json& tileJson);

	/**
	 * @brief 根据(单一图片)图块集中的id获取瓦片类型.
	 * 
	 * @param tilesetJson 图块集json数据
	 * @param localId 图块集中的id
	 * @return 瓦片类型
	 */
	engine::component::TileType getTileTypeById(const nlohmann::json& tilesetJson, int localId);

	/**
	 * @brief 根据全局Id, 获取瓦片信息.
	 * 
	 * @param gid 全局id
	 * @return engine::component::TileInfo 瓦片信息
	 */
	engine::component::TileInfo getTileInfoByGid(int gid);

	/**
	 * @brief 根据全局ID获取瓦片json对象 (用于对象层获取瓦片信息).
	 * 
	 * @param gid 全局 ID
	 * @return 瓦片 json 对象
	 */
	std::optional<nlohmann::json> getTileJsonByGid(int gid) const;

	/**
	 * @brief 加载Tiled tileset 文件 (.tsj).
	 * 
	 * @param tilesetPath Tileset 文件路径
	 * @param firstGid 此tileset的第一个全局id
	 */
	void loadTileset(std::string_view tilesetPath, int firstGid);

	/**
	 * @brief 解析图片路径, 合并地图路径和相对路径. 例如
	 *	- 1. 文件路径: "assets/maps/level1.tmj"
	 *	- 2. 相对路径: "../textures/Layers/back.png"
	 *  - 3. 最终路径: "assets/textures/Layers/back.png"
	 * 
	 * @param relative 相对路径
	 * @param filePath 文件路径
	 * @return std::string 解析后的完整路径
	 */
	std::string resolvePath(std::string_view relativePath, std::string_view filePath);
private:
	static constexpr std::string_view mLogTag = "LevelLoader";				///< @brief 日志标记		

	std::string mMapPath;													///< @brief 地图路径
	glm::ivec2 mMapSize;													///< @brief 地图尺寸(瓦片数量)
	glm::ivec2 mTileSize;													///< @brief 瓦片尺寸(像素)
	std::map<int, nlohmann::json> mTilesetData;								///< @brief 瓦片集数据
};

template<typename T>
inline std::optional<T> scene::LevelLoader::getTileProperty(const nlohmann::json& tileJson, std::string_view propertyName) {
	if (!tileJson.contains("properties")) {
		return std::nullopt;
	}
	const auto& properties = tileJson["properties"];
	for (const auto& property : properties) {
		if (property.contains("name") && property["name"] == std::string(propertyName)) {
			if (property.contains("value")) {
				return property["value"].get<T>();
			}
		}
	}
	return std::nullopt;
}
} // namespace engine::scene

#endif // !LEVEL_LOADER_H

