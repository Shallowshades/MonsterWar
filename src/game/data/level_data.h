/*****************************************************************//**
 * @file   level_data.h
 * @brief  关卡数据结构（波次、关卡配置数据）
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.20
 *********************************************************************/

#pragma once
#ifndef LEVEL_DATA_H
#define LEVEL_DATA_H

#include <entt/entity/fwd.hpp>
#include <queue>
#include <string>
#include <utility>
#include <vector>

namespace game::data {

    /**
     * @brief 单一波次数据
     * @note 包含下一波次间隔、本波次敌人生成间隔和"本波次敌人类型-数量"对
     */
    struct Wave {
        float mNextWaveInterval{};                              ///< @brief 下一波次间隔（单位：秒）
        float mSpawnInterval{};                                 ///< @brief 本波次敌人生成间隔（单位：秒）
        std::vector<std::pair<entt::id_type, int>> mEnemyTypes; ///< @brief 敌人类型-数量对（类型为敌人名哈希）
    };

    /**
     * @brief 多波次数据，即一关中所有的波次
     * @note 包含下一波次倒计时、波次队列
     */
    struct Waves {
        float mNextWaveCountDown{}; ///< @brief 下一波次倒计时（单位：秒）
        std::queue<Wave> mWaves;    ///< @brief 波次队列（先进先出）
    };

    /**
     * @brief 关卡数据，包含一关中的波次数据及其他必要信息
     * @note 关卡号、敌人等级、敌人稀有度、关卡名称、地图路径、准备时间、总敌人数量
     */
    struct LevelData {
        int mLevelNumber{ 1 };      ///< @brief 关卡号
        int mEnemyLevel{ 1 };       ///< @brief 敌人等级（本关所有敌人统一等级）
        int mEnemyRarity{ 1 };      ///< @brief 敌人稀有度（本关所有敌人统一稀有度）
        std::string mName;          ///< @brief 关卡名称
        std::string mMapPath;       ///< @brief 地图路径
        float mPrepTime{ 5.0f };    ///< @brief 开局准备时间（单位：秒）
        int mTotalEnemyCount{ 0 };  ///< @brief 总敌人数量
        Waves mWavesData;           ///< @brief 波次数据
    };

}   // namespace game::data

#endif // LEVEL_DATA_H
