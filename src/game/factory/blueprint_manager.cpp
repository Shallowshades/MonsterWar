/*****************************************************************//**
 * @file   blueprint_manager.cpp
 * @brief  蓝图管理器实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.21
 *********************************************************************/

#include "blueprint_manager.h"
#include "../../engine/resource/resource_manager.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <entt/core/hashed_string.hpp>

namespace game::factory {

    BlueprintManager::BlueprintManager(engine::resource::ResourceManager& resource_manager)
        : mResourceManager(resource_manager) {
    }

    bool BlueprintManager::loadPlayerClassBlueprints(std::string_view player_json_path) {
        auto path = std::filesystem::path(player_json_path);
        std::ifstream file(path);
        nlohmann::json json;
        file >> json;
        file.close();
        // --- 解析蓝图 ---
        try {
            for (auto& [class_name, data_json] : json.items()) {
                entt::id_type class_id = entt::hashed_string(class_name.c_str());
                // 解析 Projectile
                entt::id_type projectile_id = parseProjectileID(data_json);
                // 解析 Stats
                data::StatsBlueprint stats = parseStats(data_json);
                // 解析 Sprite
                data::SpriteBlueprint sprite = parseSprite(data_json);
                // 解析 Animation
                std::unordered_map<entt::id_type, data::AnimationBlueprint> animations = parseAnimationsMap(data_json);
                // 解析Sound
                data::SoundBlueprint sounds = parseSound(data_json);
                // 解析Player数据
                data::PlayerBlueprint player = parsePlayer(data_json);
                // 解析DisplayInfo
                data::DisplayInfoBlueprint display_info = parseDisplayInfo(data_json);
                // 解析完毕，组合蓝图并插入容器
                mPlayerClassBlueprints.emplace(class_id, data::PlayerClassBlueprint{ class_id,
                    projectile_id,
                    class_name,
                    std::move(stats),
                    std::move(player),
                    std::move(sounds),
                    std::move(sprite),
                    std::move(display_info),
                    std::move(animations) }
                );
            }
        }
        catch (const std::exception& e) {
            spdlog::error("加载玩家单位数据时出错: {}", e.what());
            return false;
        }
        return true;
    }

    bool BlueprintManager::loadEnemyClassBlueprints(std::string_view enemy_json_path) {
        auto path = std::filesystem::path(enemy_json_path);
        std::ifstream file(path);
        nlohmann::json json;
        file >> json;
        file.close();
        // 解析蓝图
        try {
            for (auto& [class_name, data_json] : json.items()) {
                entt::id_type class_id = entt::hashed_string(class_name.c_str());
                // 分别解析各个子蓝图
                entt::id_type projectile_id = parseProjectileID(data_json);
                data::StatsBlueprint stats = parseStats(data_json);
                data::SpriteBlueprint sprite = parseSprite(data_json);
                std::unordered_map<entt::id_type, data::AnimationBlueprint> animations = parseAnimationsMap(data_json);
                data::SoundBlueprint sounds = parseSound(data_json);
                data::EnemyBlueprint enemy = parseEnemy(data_json);
                data::DisplayInfoBlueprint display_info = parseDisplayInfo(data_json);
                // 组合蓝图并插入容器
                mEnemyClassBlueprints.emplace(class_id, data::EnemyClassBlueprint{ class_id,
                    projectile_id,
                    class_name,
                    std::move(stats),
                    std::move(enemy),
                    std::move(sounds),
                    std::move(sprite),
                    std::move(display_info),
                    std::move(animations) });
            }
        }
        catch (const std::exception& e) {
            spdlog::error("加载敌人单位数据时出错: {}", e.what());
            return false;
        }
        return true;
    }

    bool BlueprintManager::loadProjectileBlueprints(std::string_view projectile_json_path) {
        auto path = std::filesystem::path(projectile_json_path);
        std::ifstream file(path);
        nlohmann::json json;
        file >> json;
        file.close();
        // --- 解析蓝图 ---
        try {
            for (auto& [name, data_json] : json.items()) {
                // 解析基础数据
                entt::id_type id = entt::hashed_string(name.c_str());
                float arc_height = data_json["arc_height"].get<float>();
                float total_flight_time = data_json["total_flight_time"].get<float>();
                // 解析 Sprite
                data::SpriteBlueprint sprite = parseSprite(data_json);
                // 解析 Sound
                data::SoundBlueprint sounds = parseSound(data_json);
                // 解析其它数据，组合蓝图并插入容器
                mProjectileBlueprints.emplace(id, data::ProjectileBlueprint{ id,
                    name,
                    arc_height,
                    total_flight_time,
                    std::move(sprite),
                    std::move(sounds) }
                );
            }
        }
        catch (const std::exception& e) {
            spdlog::error("加载投射物数据时出错: {}", e.what());
            return false;
        }
        return true;
    }

    bool BlueprintManager::loadEffectBlueprints(std::string_view effect_json_path) {
        auto path = std::filesystem::path(effect_json_path);
        std::ifstream file(path);
        nlohmann::json json;
        file >> json;
        file.close();
        // --- 解析蓝图 ---
        try {
            for (auto& [name, data_json] : json.items()) {
                // 解析基础数据
                entt::id_type id = entt::hashed_string(name.c_str());
                // 解析 Sprite
                data::SpriteBlueprint sprite = parseSprite(data_json);
                // 解析单个动画
                data::AnimationBlueprint animation = parseOneAnimation(data_json);
                // 解析完毕，组合蓝图并插入容器
                mEffectBlueprints.emplace(id, data::EffectBlueprint{ id,
                    name,
                    std::move(sprite),
                    std::move(animation) }
                );
            }
        }
        catch (const std::exception& e) {
            spdlog::error("加载效果数据时出错: {}", e.what());
            return false;
        }
        return true;
    }

    const data::PlayerClassBlueprint& BlueprintManager::getPlayerClassBlueprint(entt::id_type id) const {
        if (auto it = mPlayerClassBlueprints.find(id); it != mPlayerClassBlueprints.end()) {
            return it->second;
        }
        spdlog::error("未找到对应 id 的 PlayerClassBlueprint: {}", id);
        return mPlayerClassBlueprints.begin()->second;
    }

    const data::EnemyClassBlueprint& BlueprintManager::getEnemyClassBlueprint(entt::id_type id) const {
        if (auto it = mEnemyClassBlueprints.find(id); it != mEnemyClassBlueprints.end()) {
            return it->second;
        }
        spdlog::error("未找到对应 id 的 EnemyClassBlueprint: {}", id);
        return mEnemyClassBlueprints.begin()->second;
    }

    const data::ProjectileBlueprint& BlueprintManager::getProjectileBlueprint(entt::id_type id) const {
        if (auto it = mProjectileBlueprints.find(id); it != mProjectileBlueprints.end()) {
            return it->second;
        }
        spdlog::error("未找到对应 id 的 ProjectileBlueprint: {}", id);
        return mProjectileBlueprints.begin()->second;
    }

    const data::EffectBlueprint& BlueprintManager::getEffectBlueprint(entt::id_type id) const {
        if (auto it = mEffectBlueprints.find(id); it != mEffectBlueprints.end()) {
            return it->second;
        }
        spdlog::error("未找到对应 id 的 EffectBlueprint: {}", id);
        return mEffectBlueprints.begin()->second;
    }

    // --- 拆分步骤的私有解析函数 ---

    entt::id_type BlueprintManager::parseProjectileID(const nlohmann::json& json) {
        if (json.contains("projectile")) {
            return entt::hashed_string(json["projectile"].get<std::string>().c_str());
        }
        return entt::null;
    }

    data::StatsBlueprint BlueprintManager::parseStats(const nlohmann::json& json) {
        return data::StatsBlueprint{ json["hp"].get<float>(),
            json["atk"].get<float>(),
            json["def"].get<float>(),
            json["range"].get<float>(),
            json["atk_interval"].get<float>() };
    }

    data::SpriteBlueprint BlueprintManager::parseSprite(const nlohmann::json& json) {
        auto width = json["width"].get<float>();
        auto height = json["height"].get<float>();
        auto path_str = json["sprite_sheet"].get<std::string>();
        auto path_id = entt::hashed_string(path_str.c_str());
        // 可选部分：源矩形的起点默认值为 0,0，渲染目标大小默认值为 width,height
        return data::SpriteBlueprint{ path_id,
            path_str,
            engine::utils::Rect{glm::vec2(json.value("x", 0), json.value("y", 0)), glm::vec2(width, height)},
            glm::vec2(json.value("size_x", width), json.value("size_y", height)),
            glm::vec2(json.value("offset_x", 0), json.value("offset_y", 0)),
            json.value("face_right", true)
        };
    }

    std::unordered_map<entt::id_type, data::AnimationBlueprint> BlueprintManager::parseAnimationsMap(const nlohmann::json& json) {
        std::unordered_map<entt::id_type, data::AnimationBlueprint> animations;
        for (auto& [anim_name, anim_data] : json["animation"].items()) {
            // 每轮循环解析一个具体的动画 （动画名，动画数据）
            auto anim_name_id = entt::hashed_string(anim_name.c_str());
            std::vector<int> frames = anim_data["frames"].get<std::vector<int>>();
            // 处理可能存在的事件信息
            std::unordered_map<int, entt::id_type> events;
            if (anim_data.contains("events")) {
                for (auto& [event_name, event_frame] : anim_data["events"].items()) {
                    events.emplace(event_frame.get<int>(), entt::hashed_string(event_name.c_str()));
                }
            }
            data::AnimationBlueprint animation{ anim_data.value("duration", 100.0f),
                anim_data.value("row", 0),
                std::move(frames),
                std::move(events)
            };
            animations.emplace(anim_name_id, animation);
        }
        return animations;
    }

    data::AnimationBlueprint BlueprintManager::parseOneAnimation(const nlohmann::json& json) {
        // 特效只有一个动画，animation 是单个对象而不是动画名到动画数据的 map
        auto anim_data = json["animation"];
        std::vector<int> frames = anim_data["frames"].get<std::vector<int>>();
        // 处理可能存在的事件信息
        std::unordered_map<int, entt::id_type> events;
        if (anim_data.contains("events")) {
            for (auto& [event_name, event_frame] : anim_data["events"].items()) {
                events.emplace(event_frame.get<int>(), entt::hashed_string(event_name.c_str()));
            }
        }
        return data::AnimationBlueprint{ anim_data.value("duration", 100.0f),
            anim_data.value("row", 0),
            std::move(frames),
            std::move(events)
        };
    }

    data::SoundBlueprint BlueprintManager::parseSound(const nlohmann::json& json) {
        data::SoundBlueprint sounds;
        if (json.contains("sounds")) {
            for (auto& [sound_key, sound_value] : json["sounds"].items()) {
                std::string sound_path = sound_value.get<std::string>();
                entt::id_type sound_id = entt::hashed_string(sound_path.c_str());
                mResourceManager.loadSound(sound_id, sound_path);
                sounds.mSounds.emplace(entt::hashed_string(sound_key.c_str()), sound_id);
            }
        }
        return sounds;
    }

    data::PlayerBlueprint BlueprintManager::parsePlayer(const nlohmann::json& json) {
        // 解析类型
        auto type_str = json["type"].get<std::string>();
        auto type = type_str == "melee" ? game::defs::PlayerType::MELEE :   // 三目运算符嵌套
            type_str == "ranged" ? game::defs::PlayerType::RANGED :
            type_str == "mixed" ? game::defs::PlayerType::MIXED :
            game::defs::PlayerType::UNKNOWN;
        // 解析技能
        entt::id_type skill_id = entt::null;
        if (json.contains("skill")) {
            skill_id = entt::hashed_string(json["skill"].get<std::string>().c_str());
        }
        // 解析其他数据并返回
        data::PlayerBlueprint player{ type,
            skill_id,
            json["healer"].get<bool>(),
            json["block"].get<int>(),
            json["cost"].get<int>() };
        return player;
    }

    data::EnemyBlueprint BlueprintManager::parseEnemy(const nlohmann::json& json) {
        return data::EnemyBlueprint{ json["ranged"].get<bool>(), json["speed"].get<float>() };
    }

    data::DisplayInfoBlueprint BlueprintManager::parseDisplayInfo(const nlohmann::json& json) {
        return data::DisplayInfoBlueprint{ json.value("name", ""), json.value("description", "") };
    }

}   // namespace game::factory
