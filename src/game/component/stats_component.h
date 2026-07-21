/*****************************************************************//**
 * @file   stats_component.h
 * @brief  属性组件
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.21
 *********************************************************************/

#pragma once
#ifndef STATS_COMPONENT_H
#define STATS_COMPONENT_H

namespace game::component {

/**
 * @brief 属性组件
 * 用于存储角色的属性，包括生命值、攻击力、防御力、
 * 攻击范围、攻击间隔、攻击计时器、等级和稀有度。
 */
struct StatsComponent {
    float mHp{};
    float mMaxHp{};
    float mAtk{};
    float mDef{};
    float mRange{};             ///< @brief 攻击范围（射程）
    float mAtkInterval{};       ///< @brief 攻击间隔（决定攻速）
    float mAtkTimer{};          ///< @brief 攻击计时器
    int mLevel{ 1 };
    int mRarity{ 1 };           ///< @brief 稀有度，从1开始（1:普通，2:稀有，3:史诗，4:传说，5:神话...）
};

}   // namespace game::component

#endif // STATS_COMPONENT_H
