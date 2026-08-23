/*****************************************************************//**
 * @file   events.h
 * @brief  游戏层事件定义
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#pragma once
#ifndef GAME_EVENTS_H
#define GAME_EVENTS_H

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>

namespace game::defs {

    struct EnemyArriveHomeEvent {};         ///< @brief 敌人到达基地的事件

    /// @brief 攻击（命中）事件
    struct AttackEvent {
        entt::entity mAttacker{ entt::null }; ///< @brief 攻击者
        entt::entity mTarget{ entt::null };   ///< @brief 目标
        float mDamage{};                    ///< @brief 原始伤害
    };

    /// @brief 治疗（命中）事件
    struct HealEvent {
        entt::entity mHealer{ entt::null };   ///< @brief 治疗者
        entt::entity mTarget{ entt::null };   ///< @brief 目标
        float mAmount{};                    ///< @brief 治疗量
    };

    /// @brief 发射投射物事件
    struct EmitProjectileEvent {
        entt::id_type mId{ entt::null };          ///< @brief 投射物ID
        entt::entity mTarget{ entt::null };       ///< @brief 目标实体
        glm::vec2 mStartPosition{};             ///< @brief 起始位置
        glm::vec2 mTargetPosition{};             ///< @brief 目标位置
        float mDamage{};                        ///< @brief 伤害
    };

    /// @brief 敌人死亡特效事件
    struct EnemyDeadEffectEvent {
        entt::id_type mClassId{ entt::null };   ///< @brief 敌人ID
        glm::vec2 mPosition{};                  ///< @brief 死亡位置
        bool mIsFlipped{ false };               ///< @brief 是否翻转
    };

    /// @brief (通用)特效事件
    struct EffectEvent {
        entt::id_type mNameId{ entt::null };    ///< @brief 特效ID
        glm::vec2 mPosition{};                  ///< @brief 位置
        bool mIsFlipped{ false };               ///< @brief 是否翻转
    };

    /// @brief 预备出击事件（点击肖像时发送，用于下一课的出击布阵）
    struct PrepUnitEvent {
        entt::id_type mNameId{ entt::null };    ///< @brief 角色名ID
        entt::id_type mClassId{ entt::null };   ///< @brief 职业ID
        int mCost{ 0 };                         ///< @brief 出击费用
    };

    /// @brief 单位肖像悬停进入事件
    struct UIPortraitHoverEnterEvent {
        entt::id_type mNameId{ entt::null };    ///< @brief 角色名ID
    };

    /// @brief 单位肖像悬停离开事件
    struct UIPortraitHoverLeaveEvent {};

    /// @brief 移除单位肖像事件（单位出战后移除其肖像）
    struct RemoveUIPortraitEvent {
        entt::id_type mNameId{ entt::null };    ///< @brief 角色名ID
    };

    /// @brief 移除玩家单位事件（撤退/死亡时移除实体）
    struct RemovePlayerUnitEvent {
        entt::entity mEntity{ entt::null };     ///< @brief 玩家单位实体
    };

    /// @brief 单位升级事件
    struct UpgradeUnitEvent {
        entt::entity mEntity{ entt::null };     ///< @brief 升级的单位实体
        int mCost{ 0 };                         ///< @brief 升级费用
    };

    /// @brief 单位撤退事件
    struct RetreatEvent {
        entt::entity mEntity{ entt::null };     ///< @brief 撤退的单位实体
        int mCost{ 0 };                         ///< @brief 撤退返还的费用
    };

    /// @brief 关卡通关事件（通关延迟计时结束后发送，用于切换场景）
    struct LevelClearEvent {};

    /// @brief 关卡通关延迟事件（进入通关倒计时）
    struct LevelClearDelayedEvent {
        float mDelayTime{ 0.0f };               ///< @brief 延迟时间
    };

    /// @brief 游戏结束事件
    struct GameEndEvent {
        bool mIsWin{ false };                   ///< @brief 是否胜利
    };

}   // namespace game::defs

#endif // GAME_EVENTS_H
