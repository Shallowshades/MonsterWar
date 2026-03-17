#include "level_loader.h"
#include "../component/parallax_component.h"
#include "../component/transform_component.h"
#include "../component/tilelayer_component.h"
#include "../component/sprite_component.h"
#include "../component/collider_component.h"
#include "../component/physics_component.h"
#include "../component/animation_component.h"
#include "../component/health_component.h"
#include "../component/audio_component.h"
#include "../object/game_object.h"
#include "../scene/scene.h"
#include "../core/context.h"
#include "../resource/resource_manager.h"
#include "../render/sprite.h"
#include "../render/animation.h"
#include "../utils/math.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <spdlog/spdlog.h>
#include <glm/vec2.hpp>
#include <filesystem>

namespace engine::scene {
bool LevelLoader::loadLevel(std::string_view mapPath, Scene& scene) {
	// 1. 加载json文件
	auto path = std::filesystem::path(mapPath);
	std::ifstream file(path);
	if (!file.is_open()) {
		spdlog::error("{} : 无法打开关卡文件: {}", mLogTag.data(), mapPath.data());
		return false;
	}

	// 2. 解析json数据
	nlohmann::json data;
	try {
		file >> data;
	}
	catch (const nlohmann::json::parse_error& e) {
		spdlog::error("{} : 解析json数据失败: {}", mLogTag.data(), e.what());
		return false;
	}
	
	// 3.获取基本地图信息 (名称, 地图尺寸, 瓦片尺寸)
	mMapPath = std::string(mapPath);
	mMapSize = glm::ivec2(data.value("width", 0), data.value("height", 0));
	mTileSize = glm::ivec2(data.value("tilewidth", 0), data.value("tileheight", 0));

	// 4.加载tileset数据
	if (data.contains("tilesets") && data["tilesets"].is_array()) {
		for (const auto& tileset : data["tilesets"]) {
			if (!tileset.contains("source") || !tileset["source"].is_string() || !tileset.contains("firstgid") || !tileset["firstgid"].is_number_integer()) {
				spdlog::error("{} : tilesets 对象中缺少有效 'source' 或 'firstgid' 字段", mLogTag.data());
				continue;
			}
			auto tilesetPath = resolvePath(tileset["source"].get<std::string>(), mMapPath);
			int firstGid = tileset["firstgid"];
			loadTileset(tilesetPath, firstGid);
		}
	}

	// 5.加载图层数据
	if (!data.contains("layers") || !data["layers"].is_array()) {
		spdlog::error("{} : 地图文件 {} 缺少或者无效的 'layers' 数组.", mLogTag.data(), mMapPath);
		return false;
	}
	for (const auto& layerData : data["layers"]) {
		// 获取各图层对象中的类型type字段
		std::string layerType = layerData.value("type", "none");
		if (!layerData.value("visible", true)) {
			spdlog::info("{} : 图层 '{}' 不可见, 跳过加载.", mLogTag.data(), layerData.value("name", "Unnamed"));
			continue;
		}

		// 根据图层类型决定加载方法
		if (layerType == "imagelayer") {
			loadImageLayer(layerData, scene);
		}
		else if (layerType == "tilelayer") {
			loadTileLayer(layerData, scene);
		}
		else if (layerType == "objectgroup") {
			loadObjectLayer(layerData, scene);
		}
		else {
			spdlog::warn("{} : 不支持的图层类型: {}", mLogTag.data(), layerType);
		}
	}

	spdlog::info("{} : 关卡加载器完成: {}", mLogTag.data(), mMapPath);
	return true;
}

void LevelLoader::loadImageLayer(const nlohmann::json& layerJson, Scene& scene) {
	// 获取纹理相对路径 (会自动处理'\/'符号)
	std::string imagePath = layerJson.value("image", ""); // json.value() 返回的是一个临时对象
	if (imagePath.empty()) {
		spdlog::error("{} : 图层 '{}' 缺少 'image' 属性.", mLogTag.data(), layerJson.value("name", "Unnamed"));
		return;
	}

	// 解析图片路径
	auto textureId = resolvePath(imagePath, mMapPath);
	// 获取图层偏移量
	const glm::vec2 offset = glm::vec2(layerJson.value("offsetx", 0.f), layerJson.value("offsety", 0.f));
	// 获取视差因子及重复标志
	const glm::vec2 scrollFactor = glm::vec2(layerJson.value("parallaxx", 1.f), layerJson.value("parallaxy", 1.f));
	const glm::bvec2 repeat = glm::bvec2(layerJson.value("repeatx", false), layerJson.value("repeaty", false));
	// 获取图层名称
	std::string layerName = layerJson.value("name", "Unnamed");

	/// TODO: 待获取其他属性
	
	// 创建游戏对象并添加Transform, Parallax 组件
	auto gameObject = std::make_unique<engine::object::GameObject>(layerName);
	gameObject->addComponent<engine::component::TransformComponent>(offset);
	gameObject->addComponent<engine::component::ParallaxComponent>(textureId, scrollFactor, repeat);

	// 添加到场景中
	scene.addGameObject(std::move(gameObject));
	spdlog::info("{} : 加载图层: '{}' 完成", mLogTag.data(), layerName);
}

void LevelLoader::loadTileLayer(const nlohmann::json& layerJson, Scene& scene) {
	if (!layerJson.contains("data") || !layerJson["data"].is_array()) {
		spdlog::error("{} 图层 '{}' 缺少 'data' 属性.", mLogTag.data(), layerJson.value("name", "Unnamed"));
		return;
	}
	// 准备 TileInfo Vector (瓦片数量 = 地图宽度 * 地图高度)
	std::vector<engine::component::TileInfo> tiles;
	tiles.reserve(mMapSize.x * mMapSize.y);

	// 获取图层数据 (瓦片id列表)
	const auto& data = layerJson["data"];

	// 根据gid获取必要信息, 并依次填充 TileInfo Vector
	for (const auto& gid : data) {
		tiles.push_back(getTileInfoByGid(gid));
	}

	// 获取图层名称
	std::string layerName = layerJson.value("name", "Unnamed");
	// 创建游戏对象
	auto gameObject = std::make_unique<engine::object::GameObject>(layerName);
	// 添加TileLayer组件
	gameObject->addComponent<engine::component::TileLayerComponent>(mTileSize, mMapSize, std::move(tiles));
	// 添加到场景
	scene.addGameObject(std::move(gameObject));
	spdlog::info("{} : 加载瓦片图层 : '{}' 完成", mLogTag.data(), layerName);
}

void LevelLoader::loadObjectLayer(const nlohmann::json& layerJson, Scene& scene) {
	if (!layerJson.contains("objects") || !layerJson["objects"].is_array()) {
		spdlog::error("{} 对象图层 '{}' 缺少 'objects' 属性", mLogTag.data(), layerJson.value("name", "Unnamed"));
		return;
	}

	// 获取对象数据
	const auto& objects = layerJson["objects"];
	// 遍历对象数据
	for (const auto& object : objects) {
		// 获取对象id
		auto gid = object.value("gid", 0);
		// gid为0,代表不存在,则代表自己绘制的形状(碰撞盒,触发器等)
		if (gid == 0) {
			// 非矩形对象会有额外标识
			if (object.value("point", false)) {
				continue;	// TODO: 点对象的处理方式
			} else if (object.value("ellipse", false)) {
				continue;	// TODO: 椭圆对象的处理方式
			} else if (object.value("polygon", false)) {
				continue;	// TODO: 多边形对象的处理方式
			}
			// 没有这些标识则默认是矩形对象
			else {
				// 创建游戏对象并添加变换组件
				std::string objectName = object.value("name", "Unnamed");
				auto gameObject = std::make_unique<engine::object::GameObject>(objectName);
				// 获取变换组件相关信息
				auto position = glm::vec2(object.value("x", 0.f), object.value("y", 0.f));
				auto dstSize = glm::vec2(object.value("width", 0.f), object.value("height", 0.f));
				auto rotation = object.value("rotation", 0.f);
				// 添加变换组件, 缩放为设定为1.0f
				gameObject->addComponent<engine::component::TransformComponent>(position, glm::vec2(1.f), rotation);
				// 添加碰撞组件和物理组件
				// 碰撞盒大小与dstSize相同
				auto collider = std::make_unique<engine::physics::AABBCollider>(dstSize);
				auto* cc = gameObject->addComponent<engine::component::ColliderComponent>(std::move(collider));
				// 自定义形状通常是trigger类型, 除非显示指定 (因此默认为真)
				cc->setTrigger(object.value("trigger", true));
				gameObject->addComponent<engine::component::PhysicsComponent>(&scene.getContext().getPhysicsEngine(), false);
				// 获取标签信息并设置
				if (auto tag = getTileProperty<std::string>(object, "tag"); tag) {
					gameObject->setTag(tag.value());
				}
				// 添加到场景
				scene.addGameObject(std::move(gameObject));
				spdlog::info("{} : 加载对象: '{}' 完成 (类型: 自定义形状)", mLogTag.data(), objectName);
			}
		}
		else {
			// gid 存在, 则按照图片解析流程
			auto tileInfo = getTileInfoByGid(gid);
			if (tileInfo.mSprite.getTextureId().empty()) {
				spdlog::error("{} : gid 为 {} 的瓦片没有图像纹理.", mLogTag.data(), gid);
				continue;
			}
			// 获取Transform相关信息
			auto position = glm::vec2(object.value("x", 0.f), object.value("y", 0.f));
			auto dstSize = glm::vec2(object.value("width", 0.f), object.value("height", 0.f));
			// 对象层对齐点位为左下角, SDL 绘制点为左上角, 需处理
			position = glm::vec2(position.x, position.y - dstSize.y);

			auto rotation = object.value("rotation", 0.f);
			auto srcRect = tileInfo.mSprite.getSourceRect();
			if (!srcRect) {
				spdlog::error("{} : gid 为 {} 的瓦片没有源矩阵.", mLogTag.data(), gid);
				continue;
			}
			auto srcSize = glm::vec2(srcRect->w, srcRect->h);
			auto scale = dstSize / srcSize;

			// 获取对象名称
			std::string objectName = object.value("name", "Unnamed");
			// 创建游戏对象并添加组件
			auto gameObject = std::make_unique<engine::object::GameObject>(objectName);
			gameObject->addComponent<engine::component::TransformComponent>(position, scale, rotation);
			gameObject->addComponent<engine::component::SpriteComponent>(std::move(tileInfo.mSprite), scene.getContext().getResourceManager());

			// 获取瓦片json信息
			// 1. 必然存在, 因为getTileInfoByGid(gid)函数已经顺利执行
			// 2. 这里再获取json, 实际上检索了两次, 未来可用优化
			auto tileJsonOpt = getTileJsonByGid(gid);
			if (!tileJsonOpt) {
				spdlog::error("{} : gid 为 {} 的瓦片没有对应的json数据", mLogTag.data(), gid);
				continue;
			}
			auto& tileJson = tileJsonOpt.value();

			// 获取碰撞信息: 如果是SOLID类型, 则添加物理组件, 且图片源矩形区域就是碰撞盒大小
			if (tileInfo.mType == engine::component::TileType::SOLID) {
				auto collider = std::make_unique<engine::physics::AABBCollider>(srcSize);
				gameObject->addComponent<engine::component::ColliderComponent>(std::move(collider));
				// 物理组件不受重力影响
				gameObject->addComponent<engine::component::PhysicsComponent>(&scene.getContext().getPhysicsEngine(), false);
				// 设置标签方便物理引擎检索
				gameObject->setTag("solid");
			}
			// 如果非SOLID类型, 检测自定义碰撞盒是否存在
			else if (auto rect = getColliderRect(tileJson); rect) {
				// 如果有, 添加碰撞组件
				auto collider = std::make_unique<engine::physics::AABBCollider>(rect->size);
				auto* cc = gameObject->addComponent<engine::component::ColliderComponent>(std::move(collider));
				// 自定义碰撞盒的坐标是相对于图片坐标, 也就是针对Transform的偏移量
				cc->setOffset(rect->position);
				gameObject->addComponent<engine::component::PhysicsComponent>(&scene.getContext().getPhysicsEngine(), false);
			}

			// 获取标签信息并设置
			auto tag = getTileProperty<std::string>(tileJson, "tag");
			if (tag) {
				gameObject->setTag(tag.value());
			}
			// 如果是危险瓦片, 且没有手动设置标签, 则自动设置标签为 "hazard"
			else if (tileInfo.mType == engine::component::TileType::HAZARD) {
				gameObject->setTag("hazard");
			}

			// 获取重力信息并设置
			auto gravity = getTileProperty<bool>(tileJson, "gravity");
			if (gravity) {
				auto pc = gameObject->getComponent<engine::component::PhysicsComponent>();
				if (pc) {
					pc->setUseGravity(gravity.value());
				}
				else {
					spdlog::warn("{} : 对象 '{}' 在设置重力信息时没有物理组件, 请检查地图设置.", mLogTag.data(), objectName);
					gameObject->addComponent<engine::component::PhysicsComponent>(&scene.getContext().getPhysicsEngine(), gravity.value());
				}
			}

			// 获取动画信息并设置
			auto animationString = getTileProperty<std::string>(tileJson, "animation");
			if (animationString) {
				// 解析string为json对象
				nlohmann::json animationJson;
				try {
					animationJson = nlohmann::json::parse(animationString.value());
				}
				catch (const nlohmann::json::parse_error& e) {
					spdlog::error("{} : 解析动画json字符串失败: {}", mLogTag.data(), e.what());
					continue;
				}
				// 添加动画组件
				auto* ac = gameObject->addComponent<engine::component::AnimationComponent>();
				// 添加动画到动画组件
				addAnimation(animationJson, ac, srcSize);
			}

			// 获取音效信息
			auto soundString = getTileProperty<std::string>(tileJson, "sound");
			if (soundString) {
				nlohmann::json soundJson;
				try {
					soundJson = nlohmann::json::parse(soundString.value());
				}
				catch (const nlohmann::json::parse_error& e) {
					spdlog::error("{} : 解析音效JSON字符串失败: {}", mLogTag.data(), e.what());
					continue;
				}
				auto* audioComponent = gameObject->addComponent<engine::component::AudioComponent>(&scene.getContext().getAudioPlayer(), &scene.getContext().getCamera());
				addSound(soundJson, audioComponent);
			}

			// 获取生命值信息并设置
			auto health = getTileProperty<int>(tileJson, "health");
			if (health) {
				gameObject->addComponent<engine::component::HealthComponent>(health.value());
			}

			// 添加到场景中
			scene.addGameObject(std::move(gameObject));
			spdlog::info("{} : 加载对象 '{}' 完成", mLogTag.data(), objectName);
		}
	}
}

void LevelLoader::addAnimation(const nlohmann::json& animationJson, engine::component::AnimationComponent* ac, const glm::vec2& spriteSize) {
	// 检查动画json必须是一个对象, 并且动画组件不能为空
	if (!animationJson.is_object() || !ac) {
		spdlog::error("{} : 无效的动画json或动画组件指针", mLogTag.data());
		return;
	}

	// 遍历动画json对象中的每个键值对 (动画名称 : 动画信息)
	for (const auto& keyValue : animationJson.items()) {
		std::string_view animationName = keyValue.key();
		const auto& animationInfo = keyValue.value();
		if (!animationInfo.is_object()) {
			spdlog::warn("{} : 动画 '{}' 的信息无效或为空.", mLogTag.data(), animationName.data());
			continue;
		}

		// 获取可能存在的动画帧信息
		auto durationMs = animationInfo.value("duration", 100);			// 默认持续时间为100ms
		auto duration = static_cast<float>(durationMs) / 1000.f;		// 转换为秒
		auto row = animationInfo.value("row", 0);						// 默认行数为0
		// 帧信息(数组)是必须存在的
		if (!animationInfo.contains("frames") || !animationInfo["frames"].is_array()) {
			spdlog::warn("{} : 动画 '{}' 缺少 'frames' 数组", mLogTag.data(), animationName.data());
			continue;
		}

		// 创建一个动画对象(默认为循环播放)
		auto animation = std::make_unique<engine::render::Animation>(animationName);
		// 遍历数组并进行添加帧信息到动画对象
		for (const auto& frame : animationInfo["frames"]) {
			if (!frame.is_number_integer()) {
				spdlog::warn("{} : 动画 '{}' 中 frames 数组格式错误!", mLogTag.data(), animationName.data());
				continue;
			}
			auto column = frame.get<int>();
			// 计算源矩阵
			SDL_FRect srcRect = {
				column * spriteSize.x,
				row * spriteSize.y,
				spriteSize.x,
				spriteSize.y
			};
			// 添加动画帧到动画
			animation->addFrame(srcRect, duration);
		}
		// 将动画对象添加动画组件中
		ac->addAnimation(std::move(animation));
		spdlog::trace("{} : 添加动画 '{}'到游戏对象", mLogTag.data(), animationName.data());
	}
}

void LevelLoader::addSound(const nlohmann::json& soundJson, engine::component::AudioComponent* audioComponent) {
	if (!soundJson.is_object() || !audioComponent) {
		spdlog::error("{} : 无效的音效JSON或AudioComponent指针", mLogTag.data());
		return;
	}
	// 遍历音效JSON对象中的每个键值对 (音效id : 音效路径)
	for (const auto& sound : soundJson.items()) {
		const std::string& soundId = sound.key();
		const std::string& soundPath = sound.value();
		if (soundId.empty() || soundPath.empty()) {
			spdlog::warn("{} : 音效 '{}' 缺少必要信息.", mLogTag.data(), soundId);
			continue;
		}
		audioComponent->addSound(soundId, soundPath);
	}
}

std::optional<engine::utils::Rect> LevelLoader::getColliderRect(const nlohmann::json& tileJson) {
	if (!tileJson.contains("objectgroup")) {
		return std::nullopt;
	}

	auto& objectgroup = tileJson["objectgroup"];
	if (!objectgroup.contains("objects")) {
		return std::nullopt;
	}

	auto& objects = objectgroup["objects"];
	for (const auto& object : objects) {
		auto rect = engine::utils::Rect(
			glm::vec2(object.value("x", 0.f), object.value("y", 0.f)),
			glm::vec2(object.value("width", 0.f), object.value("height", 0.f))
		);

		if (rect.size.x > 0 && rect.size.y > 0) {
			return rect;
		}
	}
	return std::nullopt;
}

engine::component::TileType LevelLoader::getTileType(const nlohmann::json& tileJson) {
	if (tileJson.contains("properties")) {
		auto& properties = tileJson["properties"];
		for (auto& property : properties) {
			if (property.contains("name") && property["name"] == "solid") {
				auto isSolid = property.value("value", false);
				return isSolid ? engine::component::TileType::SOLID : engine::component::TileType::NORMAL;
			}
			else if (property.contains("name") && property["name"] == "slope") {
				auto slopeType = property.value("value", "");
				if (slopeType == "0_1") {
					return engine::component::TileType::SLOPE_0_1;
				}
				else if (slopeType == "1_0") {
					return engine::component::TileType::SLOPE_1_0;
				}
				else if (slopeType == "0_2") {
					return engine::component::TileType::SLOPE_0_2;
				}
				else if (slopeType == "2_0") {
					return engine::component::TileType::SLOPE_2_0;
				}
				else if (slopeType == "2_1") {
					return engine::component::TileType::SLOPE_2_1;
				}
				else if (slopeType == "1_2") {
					return engine::component::TileType::SLOPE_1_2;
				}
				else {
					spdlog::error("{} : 未知的斜坡类型: {}", mLogTag.data(), slopeType);
					return engine::component::TileType::NORMAL;
				}
			}
			else if (property.contains("name") && property["name"] == "unisolid") {
				auto isUnisolid = property.value("value", false);
				return isUnisolid ? engine::component::TileType::UNISOLID : engine::component::TileType::NORMAL;
			}
			else if (property.contains("name") && property["name"] == "hazard") {
				auto isHazard = property.value("value", false);
				return isHazard ? engine::component::TileType::HAZARD : engine::component::TileType::NORMAL;
			}
			else if (property.contains("name") && property["name"] == "ladder") {
				auto isLadder = property.value("value", false);
				return isLadder ? engine::component::TileType::LADDER : engine::component::TileType::NORMAL;
			}
			// TODO: 更多的自定义逻辑处理
		}
	}
	return engine::component::TileType::NORMAL;
}

engine::component::TileType LevelLoader::getTileTypeById(const nlohmann::json& tilesetJson, int localId) {
	if (tilesetJson.contains("tiles")) {
		auto& tiles = tilesetJson["tiles"];
		for (auto& tile : tiles) {
			if (tile.contains("id") && tile["id"] == localId) {
				return getTileType(tile);
			}
		}
	}
	return engine::component::TileType::NORMAL;
}

engine::component::TileInfo LevelLoader::getTileInfoByGid(int gid) {
	if (gid == 0) {
		return engine::component::TileInfo();
	}
	
	// upper_bound: 查找mTitlesetData中键大于gid的第一个元素, 返回迭代器
	auto tilesetIter = mTilesetData.upper_bound(gid);
	if (tilesetIter == mTilesetData.begin()) {
		spdlog::error("{} gid 为 {} 的瓦片未找到图块集.", mLogTag.data(), gid);
		return engine::component::TileInfo();
	}
	--tilesetIter; // 前移找到不大于gid的最近一个元素

	const auto& tileset = tilesetIter->second;
	// 计算瓦片在图块集中的局部ID
	auto localId = gid - tilesetIter->first;
	// 获取图块集文件路径
	std::string filePath = tileset.value("file_path", "");
	if (filePath.empty()) {
		spdlog::error("{} : Tileset 文件 '{}' 缺少 'file_path' 属性.", mLogTag.data(), tilesetIter->first);
		return engine::component::TileInfo();
	}

	// 图块集分为两种情况, 需要分别考虑
	// 单一图片
	if (tileset.contains("image")) {
		// 获取图片路径
		auto textureId = resolvePath(tileset["image"].get<std::string>(), filePath);
		// 计算瓦片在图片网格中的坐标
		auto coordinateX = localId % tileset["columns"].get<int>();
		auto coordinateY = localId / tileset["columns"].get<int>();
		// 根据坐标确定源矩阵
		SDL_FRect textureRect = {
			static_cast<float>(coordinateX * mTileSize.x),
			static_cast<float>(coordinateY * mTileSize.y),
			static_cast<float>(mTileSize.x),
			static_cast<float>(mTileSize.y)
		};
		engine::render::Sprite sprite{ textureId, textureRect };
		// 无具体的瓦片json, 需id查找
		auto tileType = getTileTypeById(tileset, localId);
		return engine::component::TileInfo(sprite, tileType);
	}
	// 多图片
	else {
		if (!tileset.contains("tiles")) {
			spdlog::error("{} : Tileset 文件 '{}' 缺少 'tiles' 属性.", mLogTag.data(), tilesetIter->first);
			return engine::component::TileInfo();
		}
		// 遍历tiles数组, 根据id查找对应的瓦片
		const auto& tilesJson = tileset["tiles"];
		for (const auto& tileJson : tilesJson) {
			auto tileId = tileJson.value("id", 0);
			if (tileId == localId) {
				if (!tileJson.contains("image")) {
					spdlog::error("{} : Tileset 文件 '{}' 中瓦片缺少 'image' 属性.", tilesetIter->first, tileId);
					return engine::component::TileInfo();
				}

				// 获取图片路径
				auto textureId = resolvePath(tileJson["image"].get<std::string>(), filePath);
				// 确认图片尺寸
				auto imageWidth = tileJson.value("imagewidth", 0);
				auto imageHeight = tileJson.value("imageheight", 0);
				// 从Json中获取源矩阵信息
				SDL_FRect textureRect = {
					static_cast<float>(tileJson.value("x", 0)),
					static_cast<float>(tileJson.value("y", 0)),
					static_cast<float>(tileJson.value("width", imageWidth)),
					static_cast<float>(tileJson.value("height", imageHeight))
				};
				engine::render::Sprite sprite{ textureId, textureRect };
				// 有具体的json, 直接解析自定义属性
				auto tileType = getTileType(tileJson);
				return engine::component::TileInfo(sprite, tileType);
			}
		}
	}

	spdlog::error("{} : 图块集 '{}' 中未找到gid未 {} 的瓦片.", mLogTag.data(), tilesetIter->first, gid);
	return engine::component::TileInfo();
}

std::optional<nlohmann::json> LevelLoader::getTileJsonByGid(int gid) const {
	// 1. 查找mTilesetData中键小于等于gid的最近元素
	auto iter = mTilesetData.upper_bound(gid);
	if (iter == mTilesetData.begin()) {
		spdlog::error("{} : gid 为 {} 的瓦片未找图块集.", mLogTag.data(), gid);
		return std::nullopt;
	}

	--iter;
	// 2. 获取图块集json对象
	const auto& tileset = iter->second;
	auto localId = gid - iter->first;
	if (!tileset.contains("tiles")) {
		spdlog::error("{} : Tileset 文件 '{}' 缺少 'tiles' 属性.", mLogTag.data(), iter->first);
		return std::nullopt;
	}

	// 3. 遍历tiles数组, 根据id查找对应的瓦片并返回瓦片json
	const auto& tilesJson = tileset["tiles"];
	for (const auto& tileJson : tilesJson) {
		auto tileId = tileJson.value("id", 0);
		if (tileId == localId) {
			return tileJson;
		}
	}

	return std::nullopt;
}

void LevelLoader::loadTileset(std::string_view tilesetPath, int firstGid) {
	auto path = std::filesystem::path(tilesetPath);
	std::ifstream tilesetFile(path);
	if (!tilesetFile.is_open()) {
		spdlog::error("{} 无法打开 Tileset 文件 : {}", mLogTag.data(), tilesetPath.data());
		return;
	}

	nlohmann::json tilesetJson;
	try {
		tilesetFile >> tilesetJson;
	}
	catch (const nlohmann::json::parse_error& e) {
		spdlog::error("{} : 解析 Tileset JSON 文件 '{}' 失败: {} (at byte {})", mLogTag.data(), tilesetPath.data(), e.what(), e.byte);
		return;
	}
	tilesetJson["file_path"] = std::string(tilesetPath);
	mTilesetData[firstGid] = std::move(tilesetJson);
	spdlog::info("{} : Tileset 文件 '{}' 加载完成, firstgid: {}", mLogTag.data(), tilesetPath.data(), firstGid);
}

std::string LevelLoader::resolvePath(std::string_view relativePath, std::string_view filePath) {
	try {
		// 获取地图文件的父目录（相对于可执行文件） “assets/maps/level1.tmj” -> “assets/maps”
		auto mapDir = std::filesystem::path(filePath).parent_path();
		// 合并路径（相对于可执行文件）并返回。 
		// std::filesystem::canonical：解析路径中的当前目录（.）和上级目录（..）导航符，得到一个干净的路径
		auto finalPath = std::filesystem::canonical(mapDir / relativePath);
		return finalPath.string();
	}
	catch (const std::exception& e) {
		spdlog::error("{} : 路径解析失败: {}", mLogTag.data(), e.what());
		return std::string(relativePath);
	}
}
} // namespace engine::scene
