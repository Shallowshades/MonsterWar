/*****************************************************************//**
 * @file   texture_manager.h
 * @brief  纹理管理器
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2025.10.19
 *********************************************************************/

#pragma once
#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <SDL3/SDL_render.h>
#include <glm/glm.hpp>

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
		SDL_Texture* loadTexture(std::string_view filePath);										///< @brief 载入纹理资源
		SDL_Texture* getTexture(std::string_view filePath);											///< @brief 尝试获取已加载的纹理
		glm::vec2 getTextureSize(std::string_view filePath);										///< @brief 卸载指定的纹理资源
		void unloadTexture(std::string_view filePath);												///< @brief 获取指定的纹理尺寸
		void clearTextures();																		///< @brief 清空所有的纹理资源

	private:
		static constexpr std::string_view mLogTag = "TextureManager";
		std::unordered_map<std::string, std::unique_ptr<SDL_Texture, SDLTextureDeleter>> mTextures;	///< @brief 存储文件路径和指向管理纹理的unique_ptr的映射
		SDL_Renderer* mRenderer = nullptr;															///< @brief 指向主渲染器的非拥有指针
	}; // class TextureManager

} // namespace engine::resource
#endif // TEXTURE_MANAGER_H
