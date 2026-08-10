/*****************************************************************//**
 * @file   session_data.cpp
 * @brief  会话数据实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.10
 *********************************************************************/

#include "session_data.h"
#include <fstream>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <entt/core/hashed_string.hpp>

namespace game::data {

    bool SessionData::loadDefaultData(std::string_view path) {
        if (!std::filesystem::exists(path)) {
            spdlog::error("Session data 文件未找到: {}", path);
            return false;
        }
        // 花括号初始化避免 most-vexing-parse
        std::ifstream file{ std::filesystem::path(path) };
        if (!file.is_open()) {
            spdlog::error("无法打开Session data文件: {}", path);
            return false;
        }

        nlohmann::json json;
        try {
            file >> json;
            // 解析成功后再清空旧数据，避免解析失败时丢失已有进度
            clear();
            mLevelNumber = json["level"].get<int>();
            mPoint = json["point"].get<int>();
            mLevelClear = json["level_clear"].get<bool>();
            // 角色数据：角色名id、职业id、角色名、职业、等级、稀有度
            for (const auto& [name, data] : json["unit"].items()) {
                entt::id_type name_id = entt::hashed_string(name.c_str());
                std::string class_str = data["class"].get<std::string>();
                entt::id_type class_id = entt::hashed_string(class_str.c_str());
                int level = data["level"].get<int>();
                int rarity = data["rarity"].get<int>();
                mUnitMap.emplace(name_id, UnitData{ name_id, class_id, name, class_str, level, rarity });
            }
        } catch (const std::exception& e) {
            spdlog::error("加载Session data失败: {}", e.what());
            return false;
        }
        return true;
    }

    bool SessionData::saveToFile(std::string_view path) {
        std::filesystem::path file_path{ path };
        // 如果父目录不存在，则自动创建
        if (!file_path.parent_path().empty() && !std::filesystem::exists(file_path.parent_path())) {
            try {
                std::filesystem::create_directories(file_path.parent_path());
            } catch (const std::exception& e) {
                spdlog::error("无法创建目录 {}: {}", file_path.parent_path().string(), e.what());
                return false;
            }
        }
        std::ofstream file{ file_path };
        if (!file.is_open()) {
            spdlog::error("无法打开存档文件: {}", path);
            return false;
        }

        nlohmann::json json;
        json["level"] = mLevelNumber;
        json["point"] = mPoint;
        json["level_clear"] = mLevelClear;
        // 角色数据：以角色名为 key，写入职业、等级、稀有度
        for (const auto& [id, data] : mUnitMap) {
            json["unit"][data.mName]["class"] = data.mClass;
            json["unit"][data.mName]["level"] = data.mLevel;
            json["unit"][data.mName]["rarity"] = data.mRarity;
        }
        file << json.dump(4);
        file.close();
        spdlog::info("存档文件已保存: {}", path);
        return true;
    }

    void SessionData::addUnit(std::string_view name, std::string_view class_str, int level, int rarity) {
        // 传长度避免依赖 data() 的 NUL 结尾
        entt::id_type name_id = entt::hashed_string(name.data(), name.size());
        entt::id_type class_id = entt::hashed_string(class_str.data(), class_str.size());
        mUnitMap.emplace(name_id,
            UnitData{ name_id, class_id, std::string(name), std::string(class_str), level, rarity });
    }

    void SessionData::removeUnit(entt::id_type name_id) {
        if (auto it = mUnitMap.find(name_id); it != mUnitMap.end()) {
            mUnitMap.erase(it);
        } else {
            spdlog::error("未找到该角色: {}", name_id);
        }
    }

    void SessionData::addUnitLevel(entt::id_type name_id, int add_level) {
        if (auto it = mUnitMap.find(name_id); it != mUnitMap.end()) {
            it->second.mLevel += add_level;
        } else {
            spdlog::error("未找到该角色: {}", name_id);
        }
    }

    void SessionData::addUnitRarity(entt::id_type name_id, int add_rarity) {
        if (auto it = mUnitMap.find(name_id); it != mUnitMap.end()) {
            it->second.mRarity += add_rarity;
        } else {
            spdlog::error("未找到该角色: {}", name_id);
        }
    }

    void SessionData::clearUnits() {
        mUnitMap.clear();
    }

    void SessionData::clear() {
        mLevelNumber = 1;
        mPoint = 0;
        mLevelClear = false;
        mUnitMap.clear();
    }

}   // namespace game::data
