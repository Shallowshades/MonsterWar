#include "audio_player.h"
#include "../resource/resource_manager.h"
#include <SDL3_mixer/SDL_mixer.h>
#include <spdlog/spdlog.h>
#include <glm/common.hpp>
#include <entt/core/hashed_string.hpp>

namespace engine::audio {
AudioPlayer::~AudioPlayer() {
	if (mMusicTrack) {
		MIX_DestroyTrack(mMusicTrack);
		mMusicTrack = nullptr;
	}
}

AudioPlayer::AudioPlayer(engine::resource::ResourceManager* resourceManager)
	: mResourceManager(resourceManager) {
	if (!mResourceManager) {
		throw std::runtime_error("AudioPlayer : 构造失败: 提供的 ResourceManager 指针为空。");
	}

	mMixer = mResourceManager->getMixer();
	if (!mMixer) {
		throw std::runtime_error("AudioPlayer : 构造失败: 无法获取混音器。");
	}

	// 单条音乐音轨
	mMusicTrack = MIX_CreateTrack(mMixer);
	if (!mMusicTrack) {
		throw std::runtime_error("AudioPlayer : 构造失败: 无法创建音乐音轨: " + std::string(SDL_GetError()));
	}
}

int AudioPlayer::playSound(entt::id_type soundId) {
	MIX_Audio* audio = mResourceManager->getSound(soundId); // 通过 ResourceManager 获取资源
	if (!audio) {
		spdlog::error("AudioPlayer: 无法获取音效 '{}' 播放。", soundId);
		return -1;
	}

	if (!MIX_PlayAudio(mMixer, audio)) { // 即发即忘方式播放音效
		spdlog::error("AudioPlayer: 无法播放音效 id: '{}': {}", soundId, SDL_GetError());
		return -1;
	}
	spdlog::trace("AudioPlayer: 播放音效 id : '{}'。", soundId);
	return 0;
}

int AudioPlayer::playSound(entt::hashed_string hashedPath) {
	MIX_Audio* audio = mResourceManager->getSound(hashedPath, hashedPath.data());
	if (!audio) {
		spdlog::error("{} : 无法获取音效 id : {}, path : {} 播放.", mLogTag.data(), hashedPath.value(), hashedPath.data());
		return -1;
	}

	if (!MIX_PlayAudio(mMixer, audio)) { // 即发即忘方式播放音效
		spdlog::error("{} : 无法播放音效 id: {}, path: {}: {}", mLogTag.data(), hashedPath.value(), hashedPath.data(), SDL_GetError());
		return -1;
	}
	spdlog::trace("{} : 播放音效 id : {}, path : {}。", mLogTag.data(), hashedPath.value(), hashedPath.data());
	return 0;
}

bool AudioPlayer::playMusic(entt::id_type musicId, int loops, int fadeInMs) {
	if (musicId == mCurrentMusicId) return true;			// 如果当前音乐已经在播放，则不重复播放
	mCurrentMusicId = musicId;
	MIX_Audio* music = mResourceManager->getMusic(musicId); // 通过 ResourceManager 获取资源
	if (!music) {
		spdlog::error("AudioPlayer: 无法获取音乐 '{}' 播放。", musicId);
		return false;
	}

	MIX_StopTrack(mMusicTrack, 0);         // 立即停止之前的音乐
	MIX_SetTrackAudio(mMusicTrack, music); // 设置音乐音轨的音频源

	// 用 SDL Properties 指定播放选项（循环次数 / 淡入时长）
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops);
	if (fadeInMs > 0) {
		SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, fadeInMs);
	}
	bool result = MIX_PlayTrack(mMusicTrack, props);
	SDL_DestroyProperties(props);

	if (!result) {
		spdlog::error("AudioPlayer: 无法播放音乐 '{}': {}", musicId, SDL_GetError());
	} else {
		spdlog::trace("AudioPlayer: 播放音乐（loops={}, fadeInMs={}）。", loops, fadeInMs);
	}
	return result;
}

bool AudioPlayer::playMusic(entt::hashed_string hashedPath, int loops, int fadeInMs) {
	if (hashedPath.value() == mCurrentMusicId) {
		return true;
	}
	mCurrentMusicId = hashedPath;
	MIX_Audio* music = mResourceManager->getMusic(hashedPath, hashedPath.data());
	if (!music) {
		spdlog::error("{} : 无法获取音乐 id: {}, path: {} 播放。", mLogTag.data(), hashedPath.value(), hashedPath.data());
		return false;
	}

	MIX_StopTrack(mMusicTrack, 0);         // 立即停止之前的音乐
	MIX_SetTrackAudio(mMusicTrack, music); // 设置音乐音轨的音频源

	// 用 SDL Properties 指定播放选项（循环次数 / 淡入时长）
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops);
	if (fadeInMs > 0) {
		SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, fadeInMs);
	}
	bool result = MIX_PlayTrack(mMusicTrack, props);
	SDL_DestroyProperties(props);

	if (!result) {
		spdlog::error("{} : 无法播放音乐 id: {}, path: {} 播放。error: {}", mLogTag.data(), hashedPath.value(), hashedPath.data(), SDL_GetError());
	} else {
		spdlog::trace("{} : 播放音乐（loops={}, fadeInMs={}）。", mLogTag.data(), loops, fadeInMs);
	}
	return result;
}

void AudioPlayer::stopMusic(int fadeOutMs) {
	if (!mMusicTrack) {
		return;
	}
	Sint64 fadeFrames = (fadeOutMs > 0) ? MIX_TrackMSToFrames(mMusicTrack, fadeOutMs) : 0;
	MIX_StopTrack(mMusicTrack, fadeFrames);
	spdlog::trace("AudioPlayer: 停止音乐。");
}

void AudioPlayer::pauseMusic() {
	MIX_PauseTrack(mMusicTrack);
	spdlog::trace("AudioPlayer: 暂停音乐。");
}

void AudioPlayer::resumeMusic() {
	MIX_ResumeTrack(mMusicTrack);
	spdlog::trace("AudioPlayer: 恢复音乐。");
}

void AudioPlayer::setSoundVolume(float volume) {
	// 混音器整体增益控制音效音量（线性倍率：0 = 静音，1 = 原音量）
	float v = glm::max(0.0f, glm::min(1.0f, volume));
	if (mMixer) {
		MIX_SetMixerGain(mMixer, v);
	}
	spdlog::trace("AudioPlayer: 设置音效音量为 {:.2f}。", v);
}

void AudioPlayer::setMusicVolume(float volume) {
	float v = glm::max(0.0f, glm::min(1.0f, volume));
	if (mMusicTrack) {
		MIX_SetTrackGain(mMusicTrack, v);
	}
	spdlog::trace("AudioPlayer: 设置音乐音量为 {:.2f}。", v);
}

float AudioPlayer::getMusicVolume() {
	if (!mMusicTrack) {
		return 0.0f;
	}
	return glm::max(0.0f, glm::min(1.0f, MIX_GetTrackGain(mMusicTrack)));
}

float AudioPlayer::getSoundVolume() {
	if (!mMixer) {
		return 0.0f;
	}
	return glm::max(0.0f, glm::min(1.0f, MIX_GetMixerGain(mMixer)));
}
} // namespace engine::audio
