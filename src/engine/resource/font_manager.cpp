#include "font_manager.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

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

	TTF_Font* FontManager::loadFont(std::string_view filePath, int pointSize) {
		// 检查点大小是否有效
		if (pointSize <= 0) {
			spdlog::error("{} 无法加载字体 '{}': 无效的点大小 {}", mLogTag.data(), filePath, pointSize);
			return nullptr;
		}

		// 创建映射表的键
		FontKey key = std::make_pair(std::string(filePath), pointSize);

		// 检查缓存
		auto iter = mFonts.find(key);
		if (iter != mFonts.end()) {
			return iter->second.get();
		}

		// 不存在则加载字体
		spdlog::debug("{} 正在加载字体 '{}' ({}pt)", mLogTag.data(), filePath.data(), pointSize);
		TTF_Font* rawFont = TTF_OpenFont(filePath.data(), static_cast<float>(pointSize));
		if (!rawFont) {
			spdlog::error("{} 加载字体 '{}' ({}pt) 失败: {}", mLogTag.data(), filePath.data(), pointSize, SDL_GetError());
			return nullptr;
		}

		// 使用unique_ptr存储到缓存中
		mFonts.emplace(key, std::unique_ptr<TTF_Font, SDLFontDeleter>(rawFont));
		spdlog::debug("{} 成功加载并缓存字体: {} ({}pt)", mLogTag.data(), filePath.data(), pointSize);
		return rawFont;
	}

	TTF_Font* FontManager::getFont(std::string_view filePath, int pointSize) {
		FontKey key = std::make_pair(std::string(filePath), pointSize);
		auto iter = mFonts.find(key);
		if (iter != mFonts.end()) {
			return iter->second.get();
		}
		spdlog::warn("{} 字体 '{}' ({}pt) 不在缓存中,尝试加载", mLogTag.data(), filePath.data(), pointSize);
		return loadFont(filePath, pointSize);
	}

	void FontManager::unloadFont(std::string_view filePath, int pointSize) {
		FontKey key = std::make_pair(std::string(filePath), pointSize);
		auto iter = mFonts.find(key);
		if (iter != mFonts.find(key)) {
			spdlog::debug("{} 卸载字体: {} ({}pt)", mLogTag.data(), filePath.data(), pointSize);
			mFonts.erase(iter);
		}
		else {
			spdlog::warn("{} 尝试加载不存在的字体: {} ({}pt)", mLogTag.data(), filePath.data(), pointSize);
		}
	}

	void FontManager::clearFonts() {
		if (!mFonts.empty()) {
			spdlog::debug("{} 正在清理所有{}个字体.", mLogTag.data(), mFonts.size());
			mFonts.clear();
		}
	}

}
