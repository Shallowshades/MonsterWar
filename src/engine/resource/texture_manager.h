/*****************************************************************//**
 * @file   texture_manager.h
 * @brief  纹理管理器
 * @version 2.0
 *
 * @author Shallowshades
 * @date   2026.03.18
 *********************************************************************/

#pragma once
#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <memory>
#include <string_view>
#include <unordered_map>
#include <SDL3/SDL_render.h>
#include <glm/glm.hpp>
#include <entt/core/fwd.hpp>

namespace engine::resource {
/**
* @brief 管理SDL_Texture资源加载,存储和检索.
*
* 在构造时初始化。使用文件路径作为键，确保纹理只加载一次并正确释放。
* 依赖于一个有效的 SDL_Renderer，构造失败会抛出异常。
*/
class TextureManager final {
	friend class ResourceManager;
private:
	// SDL_Texture的删除器函数对象,用于智能指针管理
	struct SDLTextureDeleter {
		void operator()(SDL_Texture* texture) const {
			if (texture) {
				SDL_DestroyTexture(texture);
			}
		}
	};

public:
	/**
	* @brief 构造函数.
	* @param renderer指向有效的SDL_Renderer上下文指针;不能为空
	* @throws 如果renderer为nullptr或者初始化失败std::runtime_error
	*/
	explicit TextureManager(SDL_Renderer* renderer);

	TextureManager(const TextureManager&) = delete;												///< @brief 删除拷贝构造
	TextureManager& operator=(const TextureManager&) = delete;									///< @brief 删除拷贝赋值构造
	TextureManager(TextureManager&&) = delete;													///< @brief 删除移动构造
	TextureManager& operator=(TextureManager&&) = delete;										///< @brief 删除移动赋值构造

private:
    /**
     * @brief 从文件路径加载纹理
     * @param id 纹理的唯一标识符, 通过entt::hashed_string生成
     * @param filePath 纹理文件的路径
     * @return 加载的纹理的指针
     * @note 如果纹理已经加载，则返回已加载的纹理的指针
     * @note 如果纹理未加载，则从文件路径加载纹理，并返回加载的纹理的指针
     */
    SDL_Texture* loadTexture(entt::id_type id, std::string_view filePath);

    /**
     * @brief 从字符串哈希值加载纹理
     * @param strHash entt::hashed_string类型
     * @return 加载的纹理的指针
     * @note 如果纹理已经加载，则返回已加载的纹理的指针
     * @note 如果纹理未加载，则从字符串对应的文件路径加载纹理，并返回加载的纹理的指针
     */
    SDL_Texture* loadTexture(entt::hashed_string strHash);

    /**
     * @brief 获取纹理
     * @param id 纹理的唯一标识符, 通过entt::hashed_string生成
     * @param filePath 纹理文件的路径
     * @return 加载的纹理的指针
     * @note 如果纹理已经加载，则返回已加载的纹理的指针
     * @note 如果纹理未加载，且提供了filePath，则尝试从文件路径加载纹理，并返回加载的纹理的指针
     * @note 如果纹理未加载，且没有提供filePath，则返回nullptr
     */
    SDL_Texture* getTexture(entt::id_type id, std::string_view filePath = "");

    /**
     * @brief 从字符串哈希值获取纹理
     * @param strHash entt::hashed_string类型
     * @return 加载的纹理的指针
     * @note 如果纹理已经加载，则返回已加载的纹理的指针
     * @note 如果纹理未加载，则返回nullptr
     */
    SDL_Texture* getTexture(entt::hashed_string strHash);

    /**
     * @brief 获取纹理的尺寸
     * @param id 纹理的唯一标识符, 通过entt::hashed_string生成
     * @param filePath 纹理文件的路径
     * @return 纹理的尺寸
     * @note 如果纹理未加载，且提供了filePath，则尝试从文件路径加载纹理，并返回加载的纹理的尺寸
     */
    glm::vec2 getTextureSize(entt::id_type id, std::string_view filePath = "");

    /**
     * @brief 从字符串哈希值获取纹理的尺寸
     * @param strHash entt::hashed_string类型
     * @return 纹理的尺寸
     * @note 如果纹理未加载，则返回glm::vec2(0.0f, 0.0f)
     */
    glm::vec2 getTextureSize(entt::hashed_string strHash);

    /**
     * @brief 卸载纹理
     * @param id 纹理的唯一标识符, 通过entt::hashed_string生成
     */
    void unloadTexture(entt::id_type id);

    /**
     * @brief 清空所有纹理资源
     */
    void clearTextures();

private:
	static constexpr std::string_view mLogTag = "TextureManager";
	std::unordered_map<entt::id_type, std::unique_ptr<SDL_Texture, SDLTextureDeleter>> mTextures;	///< @brief 存储文件路径和指向管理纹理的unique_ptr的映射
	SDL_Renderer* mRenderer = nullptr;																///< @brief 指向主渲染器的非拥有指针
}; // class TextureManager

} // namespace engine::resource
#endif // TEXTURE_MANAGER_H
