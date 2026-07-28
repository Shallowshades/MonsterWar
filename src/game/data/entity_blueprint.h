/*****************************************************************//**
 * @file   entity_blueprint.h
 * @brief  实体蓝图结构体定义
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.21
 *********************************************************************/

#pragma once
#ifndef ENTITY_BLUEPRINT_H
#define ENTITY_BLUEPRINT_H

#include "../defs/constants.h"
#include "../../engine/utils/math.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>

namespace game::data {

    /**
     * @brief 属性蓝图，用于创建属性组件
     */
    struct StatsBlueprint {
        float mHp{ 0 };
        float mAtk{ 0 };
        float mDef{ 0 };
        float mRange{ 0 };
        float mAtkInterval{ 0 };
    };

    /**
     * @brief 精灵蓝图，用于创建精灵组件
     */
    struct SpriteBlueprint {
        entt::id_type mId{ entt::null };
        std::string mPath;
        engine::utils::Rect mSrcRect{};
        glm::vec2 mSize{ 0.0f };
        glm::vec2 mOffset{ 0.0f };
        bool mFaceRight{ true };                ///< @brief 角色图片默认朝右，如果朝左就设置为false
    };

    /**
     * @brief 单一动画的蓝图，多个蓝图构成的关联容器即可用于创建动画组件
     */
    struct AnimationBlueprint {
        float mMsPerFrame{ 0.0f };
        int mRow{ 0 };
        std::vector<int> mFrames;               ///< @brief 动画帧索引数组
        std::unordered_map<int, entt::id_type> mEvents;   ///< @brief 动画事件，键为帧索引，值为事件ID
    };

    /**
     * @brief 声音蓝图，用于创建声音组件
     */
    struct SoundBlueprint {
        std::unordered_map<entt::id_type, entt::id_type> mSounds;
    };

    /// @brief 玩家蓝图, 用于创建玩家组件、放置类型、阻挡数量等
    struct PlayerBlueprint {
        game::defs::PlayerType mType{ game::defs::PlayerType::UNKNOWN };
        entt::id_type mSkillId{ entt::null };
        bool mHealer{ false };
        int mBlock{ 0 };
        int mCost{ 0 };
    };

    /**
     * @brief 敌人蓝图，用于创建敌人组件（EnemyComponent）
     */
    struct EnemyBlueprint {
        bool mRanged{ false };
        float mSpeed{};
    };

    /**
     * @brief 显示信息蓝图，可用于查找对应职业的名称和描述
     */
    struct DisplayInfoBlueprint {
        std::string mName;
        std::string mDescription;
    };

    /// @brief 玩家职业蓝图, 包含所有必要的子蓝图，用于创建玩家实体中的所有组件
    struct PlayerClassBlueprint {
        entt::id_type mClassId{ entt::null };
        entt::id_type mProjectileId{ entt::null };
        std::string mClassName;
        StatsBlueprint mStats{};
        PlayerBlueprint mPlayer{};
        SoundBlueprint mSounds{};
        SpriteBlueprint mSprite{};
        DisplayInfoBlueprint mDisplayInfo{};
        std::unordered_map<entt::id_type, AnimationBlueprint> mAnimations;
    };

    /**
     * @brief 敌人类型蓝图，包含所有必要的子蓝图，用于创建敌人实体中的所有组件
     */
    struct EnemyClassBlueprint {
        entt::id_type mClassId{ entt::null };
        entt::id_type mProjectileId{ entt::null };
        std::string mClassName;
        StatsBlueprint mStats{};
        EnemyBlueprint mEnemy{};
        SoundBlueprint mSounds{};
        SpriteBlueprint mSprite{};
        DisplayInfoBlueprint mDisplayInfo{};
        std::unordered_map<entt::id_type, AnimationBlueprint> mAnimations;
    };

    /// @brief 投射物蓝图, 用于创建投射物组件
    struct ProjectileBlueprint {
        entt::id_type mId{ entt::null };
        std::string mName;
        float mArcHeight{};
        float mTotalFlightTime{};
        SpriteBlueprint mSprite{};
        SoundBlueprint mSounds{};
    };

}   // namespace game::data

#endif // ENTITY_BLUEPRINT_H
