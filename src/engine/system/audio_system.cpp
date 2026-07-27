#include "audio_system.h"
#include "../core/context.h"
#include "../component/audio_component.h"
#include "../audio/audio_player.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace engine::system {

    AudioSystem::AudioSystem(entt::registry& registry, engine::core::Context& context)
        : mRegistry(registry), mContext(context) {
        auto& dispatcher = mContext.getDispatcher();
        dispatcher.sink<engine::utils::PlaySoundEvent>().connect<&AudioSystem::onPlaySoundEvent>(this);
    }

    AudioSystem::~AudioSystem() {
        auto& dispatcher = mContext.getDispatcher();
        dispatcher.disconnect(this);
    }

    void AudioSystem::onPlaySoundEvent(const engine::utils::PlaySoundEvent& event) {
        // 如果没有传入目标实体，则直接播放全局音效
        if (event.mEntity == entt::null) {
            spdlog::info("播放全局音效: {}", event.mSoundId);
            mContext.getAudioPlayer().playSound(event.mSoundId);
        }
        // 如果有传入目标实体，且实体有音效组件
        else if (auto audio_component = mRegistry.try_get<engine::component::AudioComponent>(event.mEntity); audio_component) {
            auto it = audio_component->mSounds.find(event.mSoundId);
            // 先尝试在目标实体的音效集合中查找
            if (it != audio_component->mSounds.end()) {
                spdlog::info("实体 ID: {} 中找到了音效: {}", entt::to_integral(event.mEntity), it->second);
                mContext.getAudioPlayer().playSound(it->second);
                // 如果没找到，则播放全局音效
            }
            else {
                spdlog::info("实体 ID: {} 中没有找到音效: {}", entt::to_integral(event.mEntity), event.mSoundId);
                mContext.getAudioPlayer().playSound(event.mSoundId);
            }
        }
        // 如果有传入目标实体，但实体没有音效组件，也尝试播放全局音效
        else {
            spdlog::info("实体 ID: {} 中没有音效组件，尝试播放全局音效: {}", entt::to_integral(event.mEntity), event.mSoundId);
            mContext.getAudioPlayer().playSound(event.mSoundId);
        }
    }

} // namespace engine::system
