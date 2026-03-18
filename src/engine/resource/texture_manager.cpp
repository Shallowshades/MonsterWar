#include "texture_manager.h"
#include <SDL3_image/SDL_image.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <entt/core/hashed_string.hpp>

namespace engine::resource {
TextureManager::TextureManager(SDL_Renderer* renderer) : mRenderer(renderer) {
	if (!mRenderer) {
		throw std::runtime_error(mLogTag.data() + std::string(" 构造失败: 渲染器指针为空"));
	}
	// SDL3中无需手动调用IMG_Init/IMG_Quit
	spdlog::trace("{} 构造成功", mLogTag);
}


SDL_Texture* TextureManager::loadTexture(entt::id_type id, std::string_view filePath) {
	// 检查是否已加载
	auto it = mTextures.find(id);
	if (it != mTextures.end()) {
		return it->second.get();
	}

	// 如果没加载则尝试加载纹理
	SDL_Texture* rawTexture = IMG_LoadTexture(mRenderer, filePath.data());

	// 载入纹理时，设置纹理缩放模式为最邻近插值(必不可少，否则TileLayer渲染中会出现边缘空隙/模糊)
	if (!SDL_SetTextureScaleMode(rawTexture, SDL_SCALEMODE_NEAREST)) {
		spdlog::warn("无法设置纹理缩放模式为最邻近插值");
	}

	if (!rawTexture) {
		spdlog::error("加载纹理失败: '{}': {}", filePath.data(), SDL_GetError());
		return nullptr;
	}

	// 使用带有自定义删除器的 unique_ptr 存储加载的纹理
	mTextures.emplace(id, std::unique_ptr<SDL_Texture, SDLTextureDeleter>(rawTexture));
	spdlog::debug("成功加载并缓存纹理: {}", filePath.data());

	return rawTexture;
}

SDL_Texture* TextureManager::loadTexture(entt::hashed_string strHash) {
	return loadTexture(strHash.value(), strHash.data());
}

SDL_Texture* TextureManager::getTexture(entt::id_type id, std::string_view filePath) {
	// 查找现有纹理
	auto it = mTextures.find(id);
	if (it != mTextures.end()) {
		return it->second.get();
	}

	// 如果未找到，判断是否提供了file_path
	if (filePath.empty()) {
		spdlog::error("纹理 '{}' 未找到缓存，且未提供文件路径，返回nullptr。", id);
		return nullptr;
	}

	spdlog::info("纹理 '{}' 未找到缓存，尝试从文件路径加载。", id);
	return loadTexture(id, filePath);
}

SDL_Texture* TextureManager::getTexture(entt::hashed_string strHash) {
	return getTexture(strHash.value(), strHash.data());
}

glm::vec2 TextureManager::getTextureSize(entt::id_type id, std::string_view filePath) {
	// 获取纹理
	SDL_Texture* texture = getTexture(id, filePath);
	if (!texture) {
		spdlog::error("无法获取纹理: {}", filePath.data());
		return glm::vec2(0);
	}

	// 获取纹理尺寸
	glm::vec2 size;
	if (!SDL_GetTextureSize(texture, &size.x, &size.y)) {
		spdlog::error("无法查询纹理尺寸: {}", filePath.data());
		return glm::vec2(0);
	}
	return size;
}

glm::vec2 TextureManager::getTextureSize(entt::hashed_string strHash) {
	return getTextureSize(strHash.value(), strHash.data());
}

void TextureManager::unloadTexture(entt::id_type id) {
	auto it = mTextures.find(id);
	if (it != mTextures.end()) {
		spdlog::debug("卸载纹理: id = {}", id);
		mTextures.erase(it); // unique_ptr 通过自定义删除器处理删除
	}
	else {
		spdlog::warn("尝试卸载不存在的纹理: id = {}", id);
	}
}

void TextureManager::clearTextures() {
	if (!mTextures.empty()) {
		spdlog::debug("正在清除所有 {} 个缓存的纹理。", mTextures.size());
		mTextures.clear(); // unique_ptr 处理所有元素的删除
	}
}

} // namespace engine::resource
