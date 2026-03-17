/*****************************************************************//**
 * @file   audio_component.h
 * @brief  音频组件
 * @version 1.0
 * 
 * @author Shallowshades
 * @date   2025.12.16
 *********************************************************************/

#pragma once

#ifndef AUDIO_COMPONENT_H
#define AUDIO_COMPONENT_H

#include "component.h"
#include <string>
#include <string_view>
#include <unordered_map>

namespace engine::audio { class AudioPlayer; }
namespace engine::render { class Camera; }
namespace engine::object { class GameObject; }

namespace engine::component {
class TransformComponent;
/**
* @brief 音频组件，用于处理音频播放和管理。
*/
class AudioComponent final : public Component {
    friend class engine::object::GameObject;
public:
    AudioComponent(engine::audio::AudioPlayer* audioPlayer, engine::render::Camera* camera);
    virtual ~AudioComponent() override = default;

    // 禁止拷贝和移动
    AudioComponent(const AudioComponent&) = delete;
    AudioComponent& operator=(const AudioComponent&) = delete;
    AudioComponent(AudioComponent&&) = delete;
    AudioComponent& operator=(AudioComponent&&) = delete;

    /**
    * @brief 播放音效。
    * @param sound_path 音效文件的id (或路径)。
    * @param channel 要播放的特定通道，或 -1 表示第一个可用通道。
    * @param use_spatial 是否使用空间定位。
    */
    void playSound(std::string_view soundId, int channel = -1, bool useSpatial = false);

    /**
    * @brief 添加音效到映射表。
    * @param sound_id 音效的标识符（针对本组件唯一即可）。
    * @param sound_path 音效文件的路径。
    */
    void addSound(std::string_view soundId, std::string_view soundPath);

private:
    // 核心循环方法
    void init() override;
    void update(float, engine::core::Context&) override {}
private:
    engine::audio::AudioPlayer* mAudioPlayer;                           ///< @brief 音频播放器的非拥有指针
    engine::render::Camera* mCamera;                                    ///< @brief 相机的非拥有指针，用于音频空间定位
    engine::component::TransformComponent* mTransform = nullptr;        ///< @brief 缓存变换组件
    std::unordered_map<std::string, std::string> mSoundIdToPath;        ///< @brief 音效id 到路径的映射表
};
}

#endif // AUDIO_COMPONENT_H
