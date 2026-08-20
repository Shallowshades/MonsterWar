/*****************************************************************//**
 * @file   level_config.h
 * @brief  关卡配置类
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.20
 *********************************************************************/

#pragma once
#ifndef LEVEL_CONFIG_H
#define LEVEL_CONFIG_H

#include "level_data.h"
#include <string_view>
#include <vector>

namespace game::data {

    /**
     * @brief 关卡配置类
     * @note 负责载入json配置文件，并从中获取各类关卡数据
     */
    class LevelConfig {
        std::vector<game::data::LevelData> mLevelData;  ///< @brief 关卡数据（每关对应一个LevelData）

    public:
        /**
         * @brief 加载关卡配置文件
         * @param level_json_path 配置文件路径
         * @return 是否加载成功
         */
        bool loadFromFile(std::string_view level_json_path = "assets/data/level_config.json");

        // --- getters（获取指定关卡编号的对应数据）---（关卡编号从1开始，数组角标从0开始，因此每次获取时需要减1）
        [[nodiscard]] game::data::LevelData& getLevelData(int level_number) { return mLevelData[level_number - 1]; }
        [[nodiscard]] game::data::Waves& getWavesData(int level_number) { return mLevelData[level_number - 1].mWavesData; }
        [[nodiscard]] int getLevelCount() const { return static_cast<int>(mLevelData.size()); }
        [[nodiscard]] std::string_view getMapPath(int level_number) const { return mLevelData[level_number - 1].mMapPath; }
        [[nodiscard]] int getTotalEnemyCount(int level_number) const { return mLevelData[level_number - 1].mTotalEnemyCount; }
        [[nodiscard]] bool isFinalLevel(int level_number) const { return level_number == getLevelCount(); }
        [[nodiscard]] int getEnemyLevel(int level_number) const { return mLevelData[level_number - 1].mEnemyLevel; }
        [[nodiscard]] int getEnemyRarity(int level_number) const { return mLevelData[level_number - 1].mEnemyRarity; }
    };

}   // namespace game::data

#endif // LEVEL_CONFIG_H
