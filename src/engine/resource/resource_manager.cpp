#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <entt/core/hashed_string.hpp>
#include "resource_manager.h"
#include "texture_manager.h"
#include "audio_manager.h"
#include "font_manager.h"

namespace engine::resource {
ResourceManager::ResourceManager(SDL_Renderer* renderer) {
	// 初始化各个子系统 (如果出现错误会抛出异常,由上层捕获)
	mTextureManager = std::make_unique<TextureManager>(renderer);
	mAudioManager = std::make_unique<AudioManager>();
	mFontManager = std::make_unique<FontManager>();

	spdlog::trace("{} 构造成功", mLogTag);
	// RAII : 构造成功即代表资源管理器可以正常工作, 无需再初始化, 无需检查指针是否为空
}

ResourceManager::~ResourceManager() = default;

void ResourceManager::clear() {
	mFontManager->clearFonts();
	mAudioManager->clearSounds();
	mAudioManager->clearMusic();
	mTextureManager->clearTextures();
	spdlog::trace("{} 清空资源", mLogTag);
}

void ResourceManager::loadResources(std::string_view filePath) {
	std::filesystem::path path(filePath);
	if (!std::filesystem::exists(path)) {
		spdlog::warn("资源映射文件不存在: {}", filePath);
		return;
	}
	std::ifstream file(path);
	nlohmann::json json;
	file >> json;
	try {
		if (json.contains("sound")) {
			for (const auto& [key, value] : json["sound"].items()) {
				loadSound(entt::hashed_string(key.c_str()), value.get<std::string>());
			}
		}
		if (json.contains("music")) {
			for (const auto& [key, value] : json["music"].items()) {
				loadMusic(entt::hashed_string(key.c_str()), value.get<std::string>());
			}
		}
		if (json.contains("texture")) {
			for (const auto& [key, value] : json["texture"].items()) {
				loadTexture(entt::hashed_string(key.c_str()), value.get<std::string>());
			}
		}
		if (json.contains("font")) {
			for (const auto& [key, value] : json["font"].items()) {
				loadFont(entt::hashed_string(key.c_str()), value.get<int>(), value.get<std::string>());
			}
		}
	}
	catch (const nlohmann::json::exception& e) {
		spdlog::error("加载资源文件失败: {}", e.what());
	}
}

// --- 纹理接口实现 ---
SDL_Texture* ResourceManager::loadTexture(entt::id_type id, std::string_view filePath) {
    // 构造函数已经确保了 mTextureManager 不为空，因此不需要再进行if检查，以免性能浪费
    return mTextureManager->loadTexture(id, filePath);
}

SDL_Texture* ResourceManager::loadTexture(entt::hashed_string strHash) {
    return mTextureManager->loadTexture(strHash);
}

SDL_Texture* ResourceManager::getTexture(entt::id_type id, std::string_view filePath) {
    return mTextureManager->getTexture(id, filePath);
}

SDL_Texture* ResourceManager::getTexture(entt::hashed_string strHash) {
    return mTextureManager->getTexture(strHash);
}

glm::vec2 ResourceManager::getTextureSize(entt::id_type id, std::string_view filePath) {
    return mTextureManager->getTextureSize(id, filePath);
}

glm::vec2 ResourceManager::getTextureSize(entt::hashed_string strHash) {
    return mTextureManager->getTextureSize(strHash);
}

void ResourceManager::unloadTexture(entt::id_type id) {
    mTextureManager->unloadTexture(id);
}

void ResourceManager::clearTextures() {
    mTextureManager->clearTextures();
}

// --- 音频接口实现 ---
MIX_Audio* ResourceManager::loadSound(entt::id_type id, std::string_view filePath) {
    return mAudioManager->loadSound(id, filePath);
}

MIX_Audio* ResourceManager::loadSound(entt::hashed_string strHash) {
    return mAudioManager->loadSound(strHash);
}

MIX_Audio* ResourceManager::getSound(entt::id_type id, std::string_view filePath) {
    return mAudioManager->getSound(id, filePath);
}

MIX_Audio* ResourceManager::getSound(entt::hashed_string strHash) {
    return mAudioManager->getSound(strHash);
}

void ResourceManager::unloadSound(entt::id_type id) {
    mAudioManager->unloadSound(id);
}

void ResourceManager::clearSounds() {
    mAudioManager->clearSounds();
}

MIX_Audio* ResourceManager::loadMusic(entt::id_type id, std::string_view filePath) {
    return mAudioManager->loadMusic(id, filePath);
}

MIX_Audio* ResourceManager::loadMusic(entt::hashed_string strHash) {
    return mAudioManager->loadMusic(strHash);
}

MIX_Audio* ResourceManager::getMusic(entt::id_type id, std::string_view filePath) {
    return mAudioManager->getMusic(id, filePath);
}

MIX_Audio* ResourceManager::getMusic(entt::hashed_string strHash) {
    return mAudioManager->getMusic(strHash);
}

void ResourceManager::unloadMusic(entt::id_type id) {
    mAudioManager->unloadMusic(id);
}

void ResourceManager::clearMusic() {
    mAudioManager->clearMusic();
}

MIX_Mixer* ResourceManager::getMixer() {
    return mAudioManager->getMixer();
}

// --- 字体接口实现 ---
TTF_Font* ResourceManager::loadFont(entt::id_type id, int pointSize, std::string_view filePath) {
    return mFontManager->loadFont(id, pointSize, filePath);
}

TTF_Font* ResourceManager::loadFont(entt::hashed_string strHash, int pointSize) {
    return mFontManager->loadFont(strHash, pointSize);
}

TTF_Font* ResourceManager::getFont(entt::id_type id, int pointSize, std::string_view filePath) {
    return mFontManager->getFont(id, pointSize, filePath);
}

TTF_Font* ResourceManager::getFont(entt::hashed_string strHash, int pointSize) {
    return mFontManager->getFont(strHash, pointSize);
}

void ResourceManager::unloadFont(entt::id_type id, int pointSize) {
    mFontManager->unloadFont(id, pointSize);
}

void ResourceManager::clearFonts() {
    mFontManager->clearFonts();
}
}
