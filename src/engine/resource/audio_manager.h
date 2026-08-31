/*****************************************************************//**
 * @file   audio_manager.h
 * @brief  音效管理类
 * @version 3.0
 *
 * @author Shallowshades
 * @date   2026.03.18
 *
 * @note 适配 SDL_mixer 3.2 全新 API：Mix_Chunk/Mix_Music 已被
 *       MIX_Audio（加载一次、可反复播放的音频数据）取代。
 *********************************************************************/

#pragma once
#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <memory>
#include <unordered_map>
#include <string_view>
#include <entt/core/fwd.hpp>
#include <SDL3_mixer/SDL_mixer.h>

namespace engine::resource {
/**
* @brief 管理 SDL_mixer 音效 (MIX_Audio) 和音乐 (MIX_Audio)。
*
* 提供音频资源的加载和缓存功能。构造失败时会抛出异常。
* 仅供 ResourceManager 内部使用。
*/
class AudioManager final {
    friend class ResourceManager;

private:
    // MIX_Audio 的自定义删除器
    struct SDLMixAudioDeleter {
        void operator()(MIX_Audio* audio) const {
            if (audio) {
                MIX_DestroyAudio(audio);
            }
        }
    };

public:
    /**
    * @brief 构造函数。初始化 SDL_mixer 并创建混音器。
    * @throws std::runtime_error 如果 SDL_mixer 初始化或创建混音器失败。
    */
    AudioManager();

    ~AudioManager();            ///< @brief 需要手动添加析构函数，清理资源并退出 SDL_mixer。

    // 当前设计中，我们只需要一个AudioManager，所有权不变，所以不需要拷贝、移动相关构造及赋值运算符
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;

    /**
	* @brief 获取混音器句柄。
	* @note AudioPlayer 依赖它创建音轨/播放音频。
	*/
    MIX_Mixer* getMixer() const { return mMixer; }

private:  // 仅供 ResourceManager 访问的方法
    /**
	* @brief 从文件路径加载音效
	* @param id 音效的唯一标识符, 通过entt::hashed_string生成
	* @param filePath 音效文件的路径
	* @return 加载的音效的指针
	* @note 如果音效已经加载，则返回已加载音效的指针
	* @note 如果音效未加载，则从文件路径加载音效，并返回加载的音效的指针
	* @note 音效使用预解码（predecode=true），反复播放时无需每次解码
	*/
    MIX_Audio* loadSound(entt::id_type id, std::string_view filePath);

    /**
	* @brief 从字符串哈希值加载音效
	* @param strHash entt::hashed_string类型
	* @return 加载的音效的指针
	* @note 如果音效已经加载，则返回已加载音效的指针
	* @note 如果音效未加载，则从哈希字符串对应的文件路径加载音效，并返回加载的音效的指针
	*/
    MIX_Audio* loadSound(entt::hashed_string strHash);

    /**
    * @brief 从文件路径获取音效
    * @param id 音效的唯一标识符, 通过entt::hashed_string生成
    * @return 加载的音效的指针
    * @note 如果音效已经加载，则返回已加载音效的指针
    * @note 如果音效未加载，则从哈希字符串对应的文件路径加载音效，并返回加载的音效的指针
    */
    MIX_Audio* getSound(entt::id_type id, std::string_view filePath = "");

    /**
    * @brief 从字符串哈希值获取音效
    * @param strHash entt::hashed_string类型
    * @return 加载的音效的指针
    * @note 如果音效已经加载，则返回已加载音效的指针
    * @note 如果音效未加载，则从哈希字符串对应的文件路径加载音效，并返回加载的音效的指针
    */
    MIX_Audio* getSound(entt::hashed_string strHash);

    /**
	* @brief 卸载指定的音效资源
	* @param id 音效的唯一标识符, 通过entt::hashed_string生成
	*/
    void unloadSound(entt::id_type id);

    /**
    * @brief 清空所有音效资源
    */
    void clearSounds();

    /**
	* @brief 从文件路径加载音乐
	* @param id 音乐的唯一标识符, 通过entt::hashed_string生成
	* @param filePath 音乐文件的路径
	* @return 加载的音乐的指针
	* @note 如果音乐已经加载，则返回已加载音乐的指针
	* @note 如果音乐未加载，则从文件路径加载音乐，并返回加载的音乐的指针
	* @note 音乐不预解码（predecode=false），边播边解码，省内存
	*/
    MIX_Audio* loadMusic(entt::id_type id, std::string_view filePath);

    /**
    * @brief 从字符串哈希值加载音乐
    * @param strHash entt::hashed_string类型
    * @return 加载的音乐的指针
    * @note 如果音乐已经加载，则返回已加载音乐的指针
    * @note 如果音乐未加载，则从哈希字符串对应的文件路径加载音乐，并返回加载的音乐的指针
    */
    MIX_Audio* loadMusic(entt::hashed_string strHash);

    /**
    * @brief 从文件路径获取音乐
    * @param id 音乐的唯一标识符, 通过entt::hashed_string生成
    * @return 加载的音乐的指针
    * @note 如果音乐已经加载，则返回已加载音乐的指针
    * @note 如果音乐未加载，则从哈希字符串对应的文件路径加载音乐，并返回加载的音乐的指针
    */
    MIX_Audio* getMusic(entt::id_type id, std::string_view filePath = "");

    /**
	* @brief 从字符串哈希值获取音乐
	* @param strHash entt::hashed_string类型
	* @return 加载的音乐的指针
	* @note 如果音乐已经加载，则返回已加载音乐的指针
	* @note 如果音乐未加载，则从哈希字符串对应的文件路径加载音乐，并返回加载的音乐的指针
	*/
    MIX_Audio* getMusic(entt::hashed_string strHash);

    /**
	* @brief 卸载指定的音乐资源
	* @param id 音乐的唯一标识符, 通过entt::hashed_string生成
	*/
    void unloadMusic(entt::id_type id);

    /**
	* @brief 清空所有音乐资源
	*/
    void clearMusic();

    /**
	* @brief 清空所有音频资源
	*/
    void clearAudio();

private:
	// 混音器句柄（由 AudioManager 创建并拥有）
	MIX_Mixer* mMixer{ nullptr };
	// 音效存储 (文件路径 -> MIX_Audio)
	std::unordered_map<entt::id_type, std::unique_ptr<MIX_Audio, SDLMixAudioDeleter>> mSounds;
	// 音乐存储 (文件路径 -> MIX_Audio)
	std::unordered_map<entt::id_type, std::unique_ptr<MIX_Audio, SDLMixAudioDeleter>> mMusic;
};
} // namespace engine::resource

#endif // AUDIO_MANAGER_H
