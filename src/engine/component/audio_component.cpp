#include "audio_component.h"
#include "transform_component.h"
#include "../object/game_object.h"
#include "../audio/audio_player.h"
#include "../render/camera.h"
#include <spdlog/spdlog.h>

namespace engine::component {
AudioComponent::AudioComponent(engine::audio::AudioPlayer* audioPlayer, engine::render::Camera* camera)
	: mAudioPlayer(audioPlayer), mCamera(camera) {
	if (!mAudioPlayer || !mCamera) {
		spdlog::error("AudioComponent 初始化失败: 音频播放器或相机为空");
	}
}

void AudioComponent::init() {
	if (!mOwner) {
		spdlog::error("AudioComponent 没有所有者 GameObject！");
		return;
	}
	mTransform = mOwner->getComponent<TransformComponent>();
	if (!mTransform) {
		spdlog::warn("AudioComponent 所在的 GameObject 上没有 TransformComponent！，无法进行空间定位");
	}
}

void AudioComponent::playSound(std::string_view soundId, int channel, bool useSpatial) {
	// 如果 sound_id 是音效 ID，则在查找在map中查找对应的路径； 没找到的话则把 sound_id 当作路径直接使用
	auto soundPath = mSoundIdToPath.find(std::string(soundId)) != mSoundIdToPath.end() ? mSoundIdToPath[std::string(soundId)] : soundId;

	if (useSpatial && mTransform) {    // 使用空间定位
		// TODO: (SDL_Mixer 不支持空间定位，未来更换音频库时可以方便地实现)
				// 这里给一个简单的功能：150像素范围内播放，否则不播放
		auto cameraCenter = mCamera->getPosition() + mCamera->getViewPortSize() / 2.0f; // 相机中心
		auto objectPosition = mTransform->getPosition();
		float distance = glm::length(cameraCenter - objectPosition);
		if (distance > 150.0f) {
			spdlog::debug("AudioComponent::playSound: 音效 '{}' 超出范围，不播放。", soundId.data());
			return; // 超出范围，不播放
		}
		mAudioPlayer->playSound(soundPath, channel);
	}
	else {    // 不使用空间定位
		mAudioPlayer->playSound(soundPath, channel);
	}
}

void AudioComponent::addSound(std::string_view soundId, std::string_view soundPath) {
	if (mSoundIdToPath.find(std::string(soundId)) != mSoundIdToPath.end()) {
		spdlog::warn("AudioComponent::addSound: 音效 ID '{}' 已存在，覆盖旧路径。", soundId.data());
	}
	mSoundIdToPath[std::string(soundId)] = std::string(soundPath);
	spdlog::debug("AudioComponent::addSound: 添加音效 ID '{}' 路径 '{}'", soundId.data(), soundPath.data());
}
} // namespace engine::component
