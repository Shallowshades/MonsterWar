/*****************************************************************//**
 * @file   audio_player.h
 * @brief  音频播放器
 * @version 3.0
 *
 * @author Shallowshades
 * @date   2026.07.14
 *
 * @note 适配 SDL_mixer 3.2 全新 API（对照参考仓库 8aceada 提交）：
 *       - 音效：MIX_PlayAudio 即发即忘（fire-and-forget）播放
 *       - 音乐：MIX_Audio + 单条 MIX_Track
 *       - 音效音量：MIX_SetMixerGain（混音器整体增益，线性 0~1）
 *       - 音乐音量：MIX_SetTrackGain（音轨增益，线性 0~1）
 *********************************************************************/

#pragma once
#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <string_view>
#include <entt/entity/fwd.hpp>

namespace engine::resource {
    class ResourceManager;
}

struct MIX_Audio;
struct MIX_Mixer;
struct MIX_Track;

namespace engine::audio {
/**
* @brief 用于控制音频播放的单例类。
*
* @note 提供播放音效和音乐的方法，使用由 ResourceManager 管理的资源。
* @note 必须使用有效的 ResourceManager 实例初始化。
* @note 音乐音轨由本类创建并拥有，析构时通过 MIX_DestroyTrack 回收。
*/
class AudioPlayer final {
public:
    /**
	* @brief 构造函数，使用 ResourceManager 初始化。
	* @throws std::runtime_error 如果 ResourceManager 为空或无法取得混音器。
	*/
    explicit AudioPlayer(engine::resource::ResourceManager* resourceManager);
    ~AudioPlayer();

    // 删除复制/移动操作
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;
    AudioPlayer(AudioPlayer&&) = delete;
    AudioPlayer& operator=(AudioPlayer&&) = delete;

    // --- 播放控制方法 ---
    /**
	* @brief 播放音效。
	* @note 必须确保 ResourceManager 加载了音效。
	* @param soundId 音效Id。
	* @return 成功返回 0，出错返回 -1。
	*/
    int playSound(entt::id_type soundId);

    /**
     * @brief 播放音效。
     * @note 如果尚未缓存，则通过 ResourceManager 加载音效。
     * @param hashedPath 音效文件路径。
     * @return 成功返回 0，出错返回 -1。
     */
    int playSound(entt::hashed_string hashedPath);

    /**
     * @brief 播放背景音乐。如果正在播放，则重启为新的音乐。
     * @note 必须确保 ResourceManager 加载了音乐。
     * @param musicId 音乐ID。
     * @param loops 循环次数（-1 无限循环，0 播放一次，1 播放两次，以此类推）。默认为 -1。
     * @param fadeInMs 音乐淡入的时间（毫秒）（0 表示不淡入）。默认为 0。
     * @return 成功返回 true ，出错返回 false 。
     */
    bool playMusic(entt::id_type musicId, int loops = -1, int fadeInMs = 0);

    /**
	* @brief 播放背景音乐。如果正在播放，则重启为新的音乐。
	* @note 如果尚未缓存，则通过 ResourceManager 加载音乐。
	* @param hashedPath 音乐文件的路径。
	* @param loops 循环次数（-1 无限循环，0 播放一次，1 播放两次，以此类推）。默认为 -1。
	* @param fadeInMs 音乐淡入的时间（毫秒）（0 表示不淡入）。默认为 0。
	* @return 成功返回 true ，出错返回 false 。
	*/
    bool playMusic(entt::hashed_string hashedPath, int loops = -1, int fadeInMs = 0);

    /**
	* @brief 停止当前正在播放的背景音乐。
	* @param fadeOutMs 淡出时间（毫秒）（0 表示立即停止）。默认为 0。
	*/
    void stopMusic(int fadeOutMs = 0);

    /**
	* @brief 暂停当前正在播放的背景音乐。
	*/
    void pauseMusic();

    /**
	* @brief 恢复已暂停的背景音乐。
	*/
    void resumeMusic();

    /**
    * @brief 设置音效音量。
    * @param volume 音量级别（0.0-1.0）。
    */
    void setSoundVolume(float volume);

    /**
    * @brief 设置音乐音量。
    * @param volume 音量级别（0.0-1.0）。
    */
    void setMusicVolume(float volume);

    /**
    * @brief 获取当前音乐音量。
    * @return 音量级别（0.0-1.0）。
    */
    float getMusicVolume();

    /**
	* @brief 获取当前音效音量。
	* @return 音量级别（0.0-1.0）。
	*/
    float getSoundVolume();

private:
    static constexpr std::string_view mLogTag = "AudioPlayer";          ///< @brief 日志标识
	engine::resource::ResourceManager* mResourceManager;                ///< @brief 指向 ResourceManager 的非拥有指针，用于加载和管理音频资源。
    MIX_Mixer* mMixer{ nullptr };                                       ///< @brief 混音器句柄（来自 ResourceManager，非拥有）
    MIX_Track* mMusicTrack{ nullptr };                                  ///< @brief 音乐音轨（单条，本类拥有）
    entt::id_type mCurrentMusicId{ 0 };                                 ///< @brief 当前正在播放的音乐路径，用于避免重复播放同一音乐。
};

} // namespace engine::audio

#endif // !AUDIO_PLAYER_H
