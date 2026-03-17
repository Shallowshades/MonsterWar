#include "config.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace engine::core {
Config::Config(std::string_view filePath) {
	if (!loadFromFile(filePath)) {
		spdlog::error("{} : 载入配置文件失败!", mLogTag.data());
	}
}

bool Config::loadFromFile(std::string_view filePath) {
	auto path = std::filesystem::path(filePath);
	std::ifstream file(path);

	// 使用默认值
	if (!file.is_open()) {
		spdlog::warn("{} 配置文件 '{}' 未找到. 使用默认设置并创建默认配置文件.", mLogTag.data(), filePath.data());
		if (!saveToFile(filePath)) {
			spdlog::error("{} 无法创建默认配置文件 '{}'.", mLogTag.data(), filePath.data());
			return false;
		}
		return false;
	}

	// 使用配置文件值
	try {
		nlohmann::json data;
		file >> data;
		fromJson(data);
		spdlog::info("{} 成功从 '{}' 加载配置.", mLogTag.data(), filePath.data());
		return true;
	}
	catch (const std::exception& e) {
		spdlog::error("{} 读取配置文件 '{}' 时出错: {}. 使用默认配置", mLogTag.data(), filePath.data(), e.what());
	}
	return false;
}

bool Config::saveToFile(std::string_view filePath) {
	auto path = std::filesystem::path(filePath);
	std::ofstream file(path);
	if (!file.is_open()) {
		spdlog::error("{} 无法打开配置文件 '{}' 进行写入", mLogTag.data(), filePath.data());
		return false;
	}

	try {
		nlohmann::ordered_json data = toJson();
		file << data.dump(4);
		spdlog::info("{} 成功将配置保存到 '{}'.", mLogTag.data(), filePath.data());
		return true;
	}
	catch (const std::exception& e) {
		spdlog::error("{} 写入配置文件 '{}' 时出错: {}", mLogTag.data(), filePath.data(), e.what());
	}
	return false;
}

void Config::fromJson(const nlohmann::json& data) {
	// 窗口设置
	if (data.contains("window")) {
		const auto& windowConfig = data["window"];
		mWindowTitle = windowConfig.value("title", mWindowTitle);
		mWindowWidth = windowConfig.value("width", mWindowWidth);
		mWindowHeight = windowConfig.value("height", mWindowHeight);
		mWindowResizable = windowConfig.value("resizable", mWindowResizable);
	}

	// 图像设置
	if (data.contains("graphics")) {
		const auto& graphicsConifg = data["graphics"];
		mVsyncEnabled = graphicsConifg.value("vsync", mVsyncEnabled);
	}

	// 帧率设置
	if (data.contains("performance")) {
		const auto& performanceConfig = data["performance"];
		mTargetFps = performanceConfig.value("target_fps", mTargetFps);
		if (mTargetFps < 0) {
			spdlog::warn("{} 目标FPS不能为负数. 设置为0为无限制.", mLogTag.data());
			mTargetFps = 0;
		}
	}

	// 音频设置
	if (data.contains("audio")) {
		const auto& audioConfig = data["audio"];
		mMusicVolume = audioConfig.value("music_volume", mMusicVolume);
		mSoundVolume = audioConfig.value("sound_volume", mSoundVolume);
	}

	// 按键映射
	if (data.contains("input_mappings") && data["input_mappings"].is_object()) {
		const auto& mappingsJson = data["input_mappings"];
		try {
			// 直接尝试从JSON对象转换为map<string, vector<string>>
			auto inputMappings = mappingsJson.get<std::unordered_map<std::string, std::vector<std::string>>>();
			// 如果成功, 则将input_mappings 移动到 mInputMappings
			mInputMappings = std::move(inputMappings);
			spdlog::trace("{} 成功从配置文件加载输入映射.", mLogTag.data());
		} 
		catch (const std::exception& e) {
			spdlog::warn("{} 配置加载警告: 解析'input_mappings'时发生异常. 使用默认映射. 错误: {}", mLogTag.data(), e.what());
		}
	}
	else {
		spdlog::trace("{} 配置跟踪: 未找到 'input_mappings' 部分或不是对象. 使用头文件中定义的默认映射.", mLogTag.data());
	}
}

nlohmann::ordered_json Config::toJson() const {
	return nlohmann::ordered_json{
		{
			"window", {
				{ "title", mWindowTitle },
				{ "width", mWindowWidth },
				{ "height", mWindowHeight },
				{ "resizable", mWindowResizable }
			}
		},
		{
			"graphics", {
				{ "vsync", mVsyncEnabled }
			}
		},
		{
			"performance", {
				{ "target_fps", mTargetFps }
			}
		},
		{
			"audio", {
				{ "music_volume", mMusicVolume },
				{ "sound_volume", mMusicVolume }
			}
		},
		{
			"input_mappings", {
				mInputMappings
			}
		}
	};
}
} // namespace engine::core
