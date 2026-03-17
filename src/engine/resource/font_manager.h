/*****************************************************************//**
 * @file   font_manager.cpp
 * @brief  字体管理类
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2025.10.19
 *********************************************************************/

#pragma once
#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <functional>

#include <SDL3_ttf/SDL_ttf.h>

namespace engine::resource {
using FontKey = std::pair<std::string, int>;
struct FontKeyHash {
	std::size_t operator()(const FontKey& key) const {
		// 异或运算求取两个哈希值获取
		std::hash<std::string> stringHasher;
		std::hash<int> intHasher;
		return stringHasher(key.first) ^ intHasher(key.second);
	}
};

/**
* @class 字体管理类.
* @brief 管理SDL_ttf字体资源TTF_Font
*
* 提供字体的加载和缓存功能, 通过文件路径和点大小来标识
* 构造失败会抛出异常;仅供ResourceManager内部使用
*/
class FontManager final {
	friend class ResourceManager;
private:
	/**
	* @brief TTF_Font的自定义删除器.
	* @struct TTF_Font的仿函数
	*/
	struct SDLFontDeleter {
		void operator()(TTF_Font* font) const {
			if (font) {
				TTF_CloseFont(font);
			}
		}
	};
public:
	/**
		* @brief 构造函数, 初始化SDL_ttf.
		* @throw 如果SDL_ttf初始化失败抛出std::runtime_error
		*/
	FontManager();
	~FontManager();																					///< @brief 需要手动添加析构函数, 清理资源并关闭.
	FontManager(const FontManager&) = delete;														///< @brief 删除拷贝构造
	FontManager& operator=(const FontManager&) = delete;											///< @brief 删除拷贝赋值构造
	FontManager(FontManager&&) = delete;															///< @brief 删除移动构造
	FontManager& operator=(FontManager&&) = delete;													///< @brief 删除移动赋值构造
private:
	TTF_Font* loadFont(std::string_view filePath, int pointSize);									///< @brief 载入字体资源
	TTF_Font* getFont(std::string_view filePath, int pointSize);									///< @brief 尝试获取已加载的字体
	void unloadFont(std::string_view filePath, int pointSize);										///< @brief 卸载指定的字体资源
	void clearFonts();																				///< @brief 清空所有的字体资源
private:
	static constexpr std::string_view mLogTag = "FontManager";
	// 字体存储（FontKey -> TTF_Font）。  
	// unordered_map 的键需要能转换为哈希值，对于基础数据类型，系统会自动转换
	// 但是对于对于自定义类型（系统无法自动转化），则需要提供自定义哈希函数（第三个模版参数）
	std::unordered_map<FontKey, std::unique_ptr<TTF_Font, SDLFontDeleter>, FontKeyHash> mFonts;
};
} // namespace engine::resource
#endif // !FONT_MANAGER_H
