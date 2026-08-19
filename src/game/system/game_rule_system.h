/*****************************************************************//**
 * @file   game_rule_system.h
 * @brief  游戏规则系统
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.19
 *********************************************************************/

#pragma once
#ifndef GAME_RULE_SYSTEM_H
#define GAME_RULE_SYSTEM_H

#include "../defs/events.h"
#include <entt/entity/fwd.hpp>
#include <entt/signal/fwd.hpp>

namespace game::system {

/**
 * @brief 游戏规则系统
 *
 * 负责处理游戏规则，如cost更新、敌人到达基地、单位升级、单位撤退等。
 * @note 关卡数据 GameStats 存于 registry.ctx()，由本系统与其他模块共享。
 */
class GameRuleSystem {
    entt::registry& mRegistry;
    entt::dispatcher& mDispatcher;

    bool mIsLevelClear{ false };        ///< @brief 是否关卡通关
    float mLevelClearTimer{ 0.0f };     ///< @brief 关卡通关计时器(实现延迟切换场景)

public:
    GameRuleSystem(entt::registry& registry, entt::dispatcher& dispatcher);
    ~GameRuleSystem();

    void update(float delta_time);

private:
    // 事件回调函数
    void onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent& event);
    void onUpgradeUnitEvent(const game::defs::UpgradeUnitEvent& event);
    void onRetreatEvent(const game::defs::RetreatEvent& event);
    void onLevelClearDelayedEvent(const game::defs::LevelClearDelayedEvent& event);
};

}   // namespace game::system

#endif // GAME_RULE_SYSTEM_H
