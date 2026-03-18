#include "audio_player.h"
#include "../resource/resource_manager.h"
#include <SDL3_mixer/SDL_mixer.h> 
#include <spdlog/spdlog.h>
#include <glm/common.hpp>
#include <entt/core/hashed_string.hpp>

namespace engine::audio {
AudioPlayer::~AudioPlayer() = default;

AudioPlayer::AudioPlayer(engine::resource::ResourceManager* resourceManager)
	: mResourceManager(resourceManager) {
	if (!mResourceManager) {
		throw std::runtime_error("AudioPlayer : 构造失败: 提供的 ResourceManager 指针为空。");
	}
}

int AudioPlayer::playSound(entt::id_type soundId, int channel) {
	Mix_Chunk* chunk = mResourceManager->getSound(soundId); // 通过 ResourceManager 获取资源
	if (!chunk) {
		spdlog::error("AudioPlayer: 无法获取音效 '{}' 播放。", soundId);
		return -1;
	}

	int playedChannel = Mix_PlayChannel(channel, chunk, 0);    // 播放音效
	if (playedChannel == -1) {
		spdlog::error("AudioPlayer: 无法播放音效 id : '{}': {}", soundId, SDL_GetError());
	}
	else {
		spdlog::trace("AudioPlayer: 播放音效 id : '{}' 在通道 {}。", soundId, playedChannel);
	}
	return playedChannel;
}

int AudioPlayer::playSound(entt::hashed_string hashedPath, int channel) {
	Mix_Chunk* chunk = mResourceManager->getSound(hashedPath, hashedPath.data());
	if (!chunk) {
		spdlog::error("{} : 无法获取音效 id : {}, path : {} 播放.", mLogTag.data(), hashedPath.value(), hashedPath.data());
		return -1;
	}

	int playedChannel = Mix_PlayChannel(channel, chunk, 0);
	if (playedChannel == -1) {
		spdlog::error("{} : 无法播放音效 id : {}, path : {}: {}", mLogTag.data(), hashedPath.value(), hashedPath.data(), SDL_GetError());
	}
	else {
		spdlog::trace("{} : 播放音效 id : {}, path : {} 在通道 {}.", mLogTag.data(), hashedPath.value(), hashedPath.data(), playedChannel);
	}
	return playedChannel;
}

bool AudioPlayer::playMusic(entt::id_type musicId, int loops, int fadeInMs) {
	if (musicId == mCurrentMusicId) return true;			// 如果当前音乐已经在播放，则不重复播放
	mCurrentMusicId = musicId;
	Mix_Music* music = mResourceManager->getMusic(musicId); // 通过 ResourceManager 获取资源
	if (!music) {
		spdlog::error("AudioPlayer: 无法获取音乐 '{}' 播放。", musicId);
		return false;
	}

	Mix_HaltMusic();        // 停止之前的音乐

	bool result = false;
	if (fadeInMs > 0) {
		result = Mix_FadeInMusic(music, loops, fadeInMs);    // 淡入播放音乐
	}
	else {
		result = Mix_PlayMusic(music, loops);
	}

	if (!result) {
		spdlog::error("AudioPlayer: 无法播放音乐 '{}': {}", musicId, SDL_GetError());
	}
	else {
		spdlog::trace("AudioPlayer: 播放音乐 '{}'。", musicId);
	}
	return result;
}

bool AudioPlayer::playMusic(entt::hashed_string hashedPath, int loops, int fadeInMs) {
	if (hashedPath.value() == mCurrentMusicId) {
		return true;
	}
	mCurrentMusicId = hashedPath;
	Mix_Music* music = mResourceManager->getMusic(hashedPath, hashedPath.data());
	if (!music) {
		spdlog::error("{} : 无法获取音乐 id: {}, path: {} 播放。", mLogTag.data(), hashedPath.value(), hashedPath.data());
		return false;
	}

	Mix_HaltMusic();

	bool result = false;
	if (fadeInMs > 0) {
		result = Mix_FadeInMusic(music, loops, fadeInMs);
	}
	else {
		result = Mix_PlayMusic(music, loops);
	}

	if (!result) {
		spdlog::error("{} : 无法播放音乐 id: {}, path: {} 播放。error: {}", mLogTag.data(), hashedPath.value(), hashedPath.data(), SDL_GetError());
	}
	else {
		spdlog::trace("{} : 播放音乐 id: {}, path: {}。", mLogTag.data(), hashedPath.value(), hashedPath.data());
	}
	return result;
}

void AudioPlayer::stopMusic(int fade_out_ms) {
	if (fade_out_ms > 0) {
		Mix_FadeOutMusic(fade_out_ms);  // 淡出音乐
	}
	else {
		Mix_HaltMusic();
	}
	spdlog::trace("AudioPlayer: 停止音乐。");
}

void AudioPlayer::pauseMusic() {
	Mix_PauseMusic();
	spdlog::trace("AudioPlayer: 暂停音乐。");
}

void AudioPlayer::resumeMusic() {
	Mix_ResumeMusic();
	spdlog::trace("AudioPlayer: 恢复音乐。");
}

void AudioPlayer::setSoundVolume(float volume, int channel) {
	// 将浮点音量(0-1)转换为SDL_mixer的音量(0-128)
	int sdl_volume = static_cast<int>(glm::max(0.0f, glm::min(1.0f, volume)) * MIX_MAX_VOLUME);
	Mix_Volume(channel, sdl_volume);
	spdlog::trace("AudioPlayer: 设置通道 {} 的音量为 {:.2f}。", channel, volume);
}

void AudioPlayer::setMusicVolume(float volume) {
	int sdl_volume = static_cast<int>(glm::max(0.0f, glm::min(1.0f, volume)) * MIX_MAX_VOLUME);
	Mix_VolumeMusic(sdl_volume);
	spdlog::trace("AudioPlayer: 设置音乐音量为 {:.2f}。", volume);
}

float AudioPlayer::getMusicVolume() {
	// SDL_mixer的音量(0-128)转为 0～1.0 的浮点数
	return static_cast<float>(Mix_VolumeMusic(-1)) / static_cast<float>(MIX_MAX_VOLUME);
	/* 参数 -1 表示查询当前音量 */
}

float AudioPlayer::getSoundVolume(int channel) {
	return static_cast<float>(Mix_Volume(channel, -1)) / static_cast<float>(MIX_MAX_VOLUME);
}
} // namespace engine::audio
