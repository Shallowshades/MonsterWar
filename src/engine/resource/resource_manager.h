/*****************************************************************//**
 * @file	resource_manager.h
 * @brief	资源管理类
 * @version 2.0
 * 
 * @author	Shallowshades
 * @date	2026.03.18
 *********************************************************************/
#pragma once
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <memory>
#include <string>
#include <string_view>
#include <glm/glm.hpp>
#include <entt/core/fwd.hpp>
#include <nlohmann/json_fwd.hpp>

struct SDL_Renderer;
struct SDL_Texture;
struct Mix_Chunk;
struct Mix_Music;
struct TTF_Font;

/**
 * @namespace engine::resource.
 */
namespace engine::resource {


// 前向声明内部管理器
class TextureManager;
class AudioManager;
class FontManager;

/**
 * @class 资源管理类
 * @brief 作为访问各种资源管理器的中央控制点(外观模式Facade).
 * 在构造时初始化其管理的子系统.构造失败会抛出异常
 */
class ResourceManager final{
public:
	/**
	 * @brief 构造函数, 执行初始化.
	 * @param renderer SDL_Renderer指针,传递给需要它的子管理器,不能为空
	 */
	explicit ResourceManager(SDL_Renderer* renderer);

	/**
	 * @brief 显式声明析构函数, 为了让智能指针正确管理仅有前向声明的类.
	 */
	~ResourceManager();

	/**
	 * @brief 清空所有资源.
	 */
	void clear();

	/**
	 * @brief 删除拷贝和移动构造.
	 */
	ResourceManager(const ResourceManager&) = delete;										///< @brief 删除拷贝构造
	ResourceManager& operator=(const ResourceManager&) = delete;							///< @brief 删除拷贝赋值构造
	ResourceManager(ResourceManager&&) = delete;											///< @brief 删除移动构造
	ResourceManager& operator=(ResourceManager&&) = delete;									///< @brief 删除移动赋值构造

	// 加载资源
	void loadResources(std::string_view filePath);

	// --- 统一资源访问接口 ---
	// -- Texture --
	SDL_Texture* loadTexture(entt::id_type id, std::string_view filePath);					///< @brief 载入纹理资源(通过id + 文件路径)
	SDL_Texture* loadTexture(entt::hashed_string str_hs);									///< @brief 载入纹理资源(通过字符串哈希值)
	SDL_Texture* getTexture(entt::id_type id, std::string_view filePath = "");				///< @brief 尝试获取已加载纹理的指针，如果未加载则尝试加载(通过id + 文件路径)
	SDL_Texture* getTexture(entt::hashed_string str_hs);									///< @brief 尝试获取已加载纹理的指针，如果未加载则尝试加载(通过字符串哈希值)
	void unloadTexture(entt::id_type id);													///< @brief 卸载指定的纹理资源
	glm::vec2 getTextureSize(entt::id_type id, std::string_view filePath = "");				///< @brief 获取指定纹理的尺寸(通过id + 文件路径)
	glm::vec2 getTextureSize(entt::hashed_string str_hs);									///< @brief 获取指定纹理的尺寸(通过字符串哈希值)
	void clearTextures();																	///< @brief 清空所有纹理资源

	// -- Sound Effects (Chunks) --
	Mix_Chunk* loadSound(entt::id_type id, std::string_view filePath);						///< @brief 载入音效资源(通过id + 文件路径)
	Mix_Chunk* loadSound(entt::hashed_string str_hs);										///< @brief 载入音效资源(通过字符串哈希值)
	Mix_Chunk* getSound(entt::id_type id, std::string_view filePath = "");					///< @brief 尝试获取已加载音效的指针，如果未加载则尝试加载(通过id + 文件路径)
	Mix_Chunk* getSound(entt::hashed_string str_hs);										///< @brief 尝试获取已加载音效的指针，如果未加载则尝试加载(通过字符串哈希值)
	void unloadSound(entt::id_type id);														///< @brief 卸载指定的音效资源
	void clearSounds();																		///< @brief 清空所有音效资源

	// -- Music --
	Mix_Music* loadMusic(entt::id_type id, std::string_view filePath);						///< @brief 载入音乐资源(通过id + 文件路径)
	Mix_Music* loadMusic(entt::hashed_string str_hs);										///< @brief 载入音乐资源(通过字符串哈希值)
	Mix_Music* getMusic(entt::id_type id, std::string_view filePath = "");					///< @brief 尝试获取已加载音乐的指针，如果未加载则尝试加载(通过id + 文件路径)
	Mix_Music* getMusic(entt::hashed_string str_hs);										///< @brief 尝试获取已加载音乐的指针，如果未加载则尝试加载(通过字符串哈希值)
	void unloadMusic(entt::id_type id);														///< @brief 卸载指定的音乐资源
	void clearMusic();																		///< @brief 清空所有音乐资源

	// -- Fonts --
	TTF_Font* loadFont(entt::id_type id, int pointSize, std::string_view filePath);			///< @brief 载入字体资源(通过id + 文件路径)
	TTF_Font* loadFont(entt::hashed_string str_hs, int pointSize);							///< @brief 载入字体资源(通过字符串哈希值)
	TTF_Font* getFont(entt::id_type id, int pointSize, std::string_view filePath = "");		///< @brief 尝试获取已加载字体的指针，如果未加载则尝试加载(通过id + 文件路径)
	TTF_Font* getFont(entt::hashed_string str_hs, int pointSize);							///< @brief 尝试获取已加载字体的指针，如果未加载则尝试加载(通过字符串哈希值)
	void unloadFont(entt::id_type id, int pointSize);										///< @brief 卸载指定的字体资源
	void clearFonts();																		///< @brief 清空所有字体资源

private:
	static constexpr std::string_view mLogTag = "ResourceManager";
	std::unique_ptr<TextureManager> mTextureManager;
	std::unique_ptr<AudioManager> mAudioManager;
	std::unique_ptr<FontManager> mFontManager;
};

}

#endif // !RESOURCE_MANAGER_H

