/*****************************************************************//**
 * @file   audio_manager.h
 * @brief  音效管理类
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2025.10.19
 *********************************************************************/

#pragma once
#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include <SDL3_mixer/SDL_mixer.h>

namespace engine::resource {
	/**
	 * @class 音效管理类.
	 * @brief 管理SDL_mixer音效(Mix_Chunk)和音乐(Mix_Music)
	 *
	 * 提供音频资源的加载和缓存功能, 构造失败会抛出异常
	 * 仅供ResourceManager内部使用
	 */
	class AudioManager final {
		friend class ResourceManager;
	private:
		// Mix_Chunk的自定义删除器
		struct SDLMixChunkDeleter {
			void operator()(Mix_Chunk* chunk) const {
				if (chunk) {
					Mix_FreeChunk(chunk);
				}
			}
		};

		// Mix_Music的自定义删除器
		struct SDLMixMusicDeleter {
			void operator()(Mix_Music* music) const {
				if (music) {
					Mix_FreeMusic(music);
				}
			}
		};
	public:
		/**
		 * @brief 构造函数, 初始化SDL_mixer并打开音频设备.
		 * @throw 如果SDL_mixer初始化或打开音频设备失败抛出std::runtime_error
		 */
		AudioManager();
		~AudioManager();																				///< @brief 清理资源并关闭 SDL_mixer

		AudioManager(const AudioManager&) = delete;														///< @brief 删除拷贝构造
		AudioManager& operator=(const AudioManager&) = delete;											///< @brief 删除拷贝赋值构造
		AudioManager(AudioManager&&) = delete;															///< @brief 删除移动构造
		AudioManager& operator=(AudioManager&&) = delete;												///< @brief 删除移动赋值构造

	private:
		Mix_Chunk* loadSound(std::string_view filePath);												///< @brief 载入音效资源
		Mix_Chunk* getSound(std::string_view filePath);													///< @brief 尝试获取已加载的音效的指针,如果未加载则尝试加载
		void unloadSound(std::string_view filePath);													///< @brief 卸载指定的音效资源
		void clearSounds();																				///< @brief 清空所有的音效资源

		Mix_Music* loadMusic(std::string_view filePath);												///< @brief 载入音乐资源
		Mix_Music* getMusic(std::string_view filePath);													///< @brief 尝试获取已加载的音乐的指针,如果未加载则尝试加载
		void unloadMusic(std::string_view filePath);													///< @brief 卸载指定的音乐资源
		void clearMusic();																				///< @brief 清空所有的音乐资源

		void clearAudio();																				///< @brief 清空所有音频资源
	private:
		static constexpr std::string_view mLogTag = "AudioManager";
		std::unordered_map<std::string, std::unique_ptr<Mix_Chunk, SDLMixChunkDeleter>> mSounds;		///< @brief 音效存储
		std::unordered_map<std::string, std::unique_ptr<Mix_Music, SDLMixMusicDeleter>> mMusic;			///< @brief 音乐存储
	};
} // namespace engine::resource

#endif // AUDIO_MANAGER_H
