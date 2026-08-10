#include "ui_config.h"
#include "../../engine/render/image.h"
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <entt/core/hashed_string.hpp>

namespace {

// 找不到时的兜底图片：优先返回 map 首元素，map 为空则返回静态空 Image（避免解引用 begin() 的 UB）
template <typename TMap>
engine::render::Image& fallbackImage(TMap& map) {
    if (!map.empty()) return map.begin()->second;
    static engine::render::Image empty{};
    return empty;
}

}   // namespace

namespace game::data {

UIConfig::~UIConfig() = default;

bool UIConfig::loadFromFile(std::string_view path) {
    std::filesystem::path file_path(path);
    if (!std::filesystem::exists(file_path)) {
        spdlog::error("UI config 文件不存在: {}", path);
        return false;
    }
    std::ifstream file{ file_path };   // 花括号：避开 most-vexing-parse
    if (!file.is_open()) {
        spdlog::error("UI config 文件打开失败: {}", path);
        return false;
    }
    nlohmann::json json;
    try {
        file >> json;   // nlohmann 反序列化，非法 JSON 会抛异常 → 必须进 try
        loadIcon(json["icon"]);
        loadPortrait(json["portrait"]);
        loadPortraitFrame(json["portrait_frame"]);
        loadLayout(json["layout"]);
    } catch (const std::exception& e) {
        spdlog::error("载入 UI config 失败: {}", e.what());
        return false;
    }
    return true;
}

void UIConfig::loadIcon(nlohmann::json& json) {
    for (auto& [key, value] : json.items()) {
        entt::id_type id = entt::hashed_string(key.c_str());
        auto texture_path = value["sprite_sheet"].get<std::string>();
        engine::utils::Rect src_rect{ static_cast<float>(value["x"]),
            static_cast<float>(value["y"]),
            static_cast<float>(value["width"]),
            static_cast<float>(value["height"]) };
        mIconMap[id] = engine::render::Image(texture_path, src_rect, false);
    }
}

void UIConfig::loadPortrait(nlohmann::json& json) {
    for (auto& [key, value] : json.items()) {
        entt::id_type id = entt::hashed_string(key.c_str());
        auto texture_path = value["sprite_sheet"].get<std::string>();
        engine::utils::Rect src_rect{ static_cast<float>(value["x"]),
            static_cast<float>(value["y"]),
            static_cast<float>(value["width"]),
            static_cast<float>(value["height"]) };
        mPortraitMap[id] = engine::render::Image(texture_path, src_rect, false);
    }
}

void UIConfig::loadPortraitFrame(nlohmann::json& json) {
    for (auto& [key, value] : json.items()) {
        auto texture_path = value["sprite_sheet"].get<std::string>();
        int level = value["level"].get<int>();
        engine::utils::Rect src_rect{ static_cast<float>(value["x"]),
            static_cast<float>(value["y"]),
            static_cast<float>(value["width"]),
            static_cast<float>(value["height"]) };
        mPortraitFrameMap[level] = engine::render::Image(texture_path, src_rect, false);
    }
}

void UIConfig::loadLayout(nlohmann::json& json) {
    mUnitPanelPadding = json["unit_panel"]["padding"].get<float>();
    mUnitPanelFrameSize = { json["unit_panel"]["frame_size"]["width"].get<float>(),
        json["unit_panel"]["frame_size"]["height"].get<float>() };
    mUnitPanelFontSize = json["unit_panel"]["font_size"].get<int>();
    mUnitPanelFontPath = json["unit_panel"]["font_path"].get<std::string>();
    mUnitPanelFontOffset = { json["unit_panel"]["font_offset"]["x"].get<float>(),
        json["unit_panel"]["font_offset"]["y"].get<float>() };
}

engine::render::Image& UIConfig::getIcon(entt::id_type id) {
    if (auto it = mIconMap.find(id); it != mIconMap.end()) {
        return it->second;
    }
    spdlog::error("Icon 未找到: {}", id);
    return fallbackImage(mIconMap);
}

engine::render::Image& UIConfig::getPortrait(entt::id_type id) {
    if (auto it = mPortraitMap.find(id); it != mPortraitMap.end()) {
        return it->second;
    }
    spdlog::error("Portrait 未找到: {}", id);
    return fallbackImage(mPortraitMap);
}

engine::render::Image& UIConfig::getPortraitFrame(int rarity) {
    if (auto it = mPortraitFrameMap.find(rarity); it != mPortraitFrameMap.end()) {
        return it->second;
    }
    spdlog::error("Portrait Frame 未找到: {}", rarity);
    return fallbackImage(mPortraitFrameMap);
}

}   // namespace game::data
