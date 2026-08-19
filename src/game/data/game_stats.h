/*****************************************************************//**
 * @file   game_stats.h
 * @brief  关卡内游戏资源及统计数据
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.19
 *********************************************************************/

#pragma once
#ifndef GAME_STATS_H
#define GAME_STATS_H

namespace game::data {

/**
 * @brief 关卡内游戏资源及统计数据
 *
 * 包含可用cost、cost生成速率、基地血量、敌人数量、敌人到达数量、敌人击杀数量等。
 * @note 通常存入 registry.ctx()，供规则系统、UI 等共享访问。
 */
struct GameStats {
    float mCost{ 10.0f };                 ///< @brief 可用cost
    float mCostGenPerSecond{ 1.0f };      ///< @brief cost生成速率
    int mHomeHp{ 5 };                     ///< @brief 基地血量
    int mEnemyCount{ 0 };                 ///< @brief 敌人(总)数量
    int mEnemyArrivedCount{ 0 };          ///< @brief 敌人到达数量
    int mEnemyKilledCount{ 0 };           ///< @brief 敌人击杀数量
};

}   // namespace game::data

#endif // GAME_STATS_H
