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
#include <string_view>
#include <unordered_map>
#include <utility>
#include <entt/core/fwd.hpp>
#include <SDL3_ttf/SDL_ttf.h>

namespace engine::resource {
using FontKey = std::pair<entt::id_type, int>;
struct FontKeyHash {
	std::size_t operator()(const FontKey& key) const {
		// 采用C++20标准库的hash_combine实现思路
		std::size_t h1 = std::hash<entt::id_type>{}(key.first);
		std::size_t h2 = std::hash<int>{}(key.second);
		// 推荐的哈希合并方式，参考boost::hash_combine
		return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
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
    /**
	* @brief 从文件路径加载指定点大小的字体
	* @param id 字体的唯一标识符, 通过entt::hashed_string生成
	* @param pointSize 字体的点大小
	* @param filePath 字体文件的路径
	* @return 加载的字体的指针
	* @note 如果字体已经加载，则返回已加载字体的指针
	* @note 如果字体未加载，则从文件路径加载字体，并返回加载的字体的指针
	*/
    TTF_Font* loadFont(entt::id_type id, int pointSize, std::string_view filePath);

    /**
     * @brief 从字符串哈希值加载指定点大小的字体
     * @param strHash entt::hashed_string类型
     * @param pointSize 字体的点大小
     * @return 加载的字体的指针
     * @note 如果字体已经加载，则返回已加载字体的指针
     * @note 如果字体未加载，则从哈希字符串对应的文件路径加载字体，并返回加载的字体的指针
     */
    TTF_Font* loadFont(entt::hashed_string strHash, int pointSize);

    /**
     * @brief 尝试获取已加载字体的指针，如果未加载则尝试加载
     * @param id 字体的唯一标识符, 通过entt::hashed_string生成
     * @param pointSize 字体的点大小
     * @param filePath 字体文件的路径
     * @return 加载的字体的指针
     * @note 如果字体已经加载，则返回已加载字体的指针
     * @note 如果字体未加载，且提供了filePath，则尝试从文件路径加载字体，并返回加载的字体的指针
     */
    TTF_Font* getFont(entt::id_type id, int pointSize, std::string_view filePath = "");

    /**
     * @brief 从字符串哈希值获取字体
     * @param strHash entt::hashed_string类型
     * @param pointSize 字体的点大小
     * @return 加载的字体的指针
     * @note 如果字体已经加载，则返回已加载字体的指针
     * @note 如果字体未加载，则从哈希字符串对应的文件路径加载字体，并返回加载的字体的指针
     */
    TTF_Font* getFont(entt::hashed_string strHash, int pointSize);

    /**
     * @brief 卸载特定字体（通过路径哈希值和大小标识）
     * @param id 字体的唯一标识符, 通过entt::hashed_string生成
     * @param pointSize 字体的点大小
     */
    void unloadFont(entt::id_type id, int pointSize);

    /**
     * @brief 清空所有缓存的字体
     */
    void clearFonts();
private:
	static constexpr std::string_view mLogTag = "FontManager";
	// 字体存储（FontKey -> TTF_Font）。  
	// unordered_map 的键需要能转换为哈希值，对于基础数据类型，系统会自动转换
	// 但是对于对于自定义类型（系统无法自动转化），则需要提供自定义哈希函数（第三个模版参数）
	std::unordered_map<FontKey, std::unique_ptr<TTF_Font, SDLFontDeleter>, FontKeyHash> mFonts;
};
} // namespace engine::resource
#endif // !FONT_MANAGER_H
