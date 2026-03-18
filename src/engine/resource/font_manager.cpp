#include "font_manager.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <entt/core/hashed_string.hpp>

namespace engine::resource {
FontManager::FontManager() {
	if (!TTF_WasInit() && !TTF_Init()) {
		throw std::runtime_error(mLogTag.data() + std::string(" 错误: TTF_Init失败: ") + std::string(SDL_GetError()));
	}
	spdlog::trace("{} 构造成功", mLogTag);
}

FontManager::~FontManager() {
	if (!mFonts.empty()) {
		spdlog::debug("{} 不为空, 调用clearFonts处理清理逻辑", mLogTag);
		clearFonts();
	}
	TTF_Quit();
	spdlog::trace("{} 析构成功", mLogTag);
}

TTF_Font* FontManager::loadFont(entt::id_type id, int pointSize, std::string_view filePath) {
	// 检查点大小是否有效
	if (pointSize <= 0) {
		spdlog::error("无法加载字体 '{}'：无效的点大小 {}。", id, pointSize);
		return nullptr;
	}

	// 创建映射表的键
	FontKey key = { id, pointSize };

	// 首先检查缓存
	auto it = mFonts.find(key);
	if (it != mFonts.end()) {
		return it->second.get();
	}

	// 缓存中不存在，则判断是否提供了
	spdlog::debug("正在加载字体：{} ({}pt)", id, pointSize);
	TTF_Font* raw_font = TTF_OpenFont(filePath.data(), static_cast<float>(pointSize));
	if (!raw_font) {
		spdlog::error("加载字体 '{}' ({}pt) 失败：{}", id, pointSize, SDL_GetError());
		return nullptr;
	}

	// 使用 unique_ptr 存储到缓存中
	mFonts.emplace(key, std::unique_ptr<TTF_Font, SDLFontDeleter>(raw_font));
	spdlog::debug("成功加载并缓存字体：{} (id = {}, {}pt)", filePath.data(), id, pointSize);
	return raw_font;
}

TTF_Font* FontManager::loadFont(entt::hashed_string strHash, int pointSize) {
	return loadFont(strHash.value(), pointSize, strHash.data());
}

TTF_Font* FontManager::getFont(entt::id_type id, int pointSize, std::string_view filePath) {
	FontKey key = { id, pointSize };
	auto it = mFonts.find(key);
	if (it != mFonts.end()) {
		return it->second.get();
	}

	// 如果未找到，判断是否提供了filePath
	if (filePath.empty()) {
		spdlog::error("字体 '{}' ({}pt) 不在缓存中，且未提供文件路径，返回nullptr。", id, pointSize);
		return nullptr;
	}

	spdlog::info("字体 '{}' (id = {}, {}pt) 不在缓存中，尝试加载。", filePath.data(), id, pointSize);
	return loadFont(id, pointSize, filePath);
}

TTF_Font* FontManager::getFont(entt::hashed_string strHash, int pointSize) {
	return getFont(strHash.value(), pointSize, strHash.data());
}

void FontManager::unloadFont(entt::id_type id, int pointSize) {
	FontKey key = { id, pointSize };
	auto it = mFonts.find(key);
	if (it != mFonts.end()) {
		spdlog::debug("卸载字体：{} ({}pt)", id, pointSize);
		mFonts.erase(it);       // unique_ptr 会处理 TTF_CloseFont
	}
	else {
		spdlog::warn("尝试卸载不存在的字体：{} ({}pt)", id, pointSize);
	}
}

void FontManager::clearFonts() {
	if (!mFonts.empty()) {
		spdlog::debug("{} 正在清理所有{}个字体.", mLogTag.data(), mFonts.size());
		mFonts.clear();
	}
}

}
