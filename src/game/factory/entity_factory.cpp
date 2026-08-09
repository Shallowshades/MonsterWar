/*****************************************************************//**
 * @file   entity_factory.cpp
 * @brief  实体工厂实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.21
 *********************************************************************/

#include "entity_factory.h"
#include "blueprint_manager.h"
#include "../data/entity_blueprint.h"
#include "../../engine/utils/math.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/component/sprite_component.h"
#include "../../engine/component/animation_component.h"
#include "../../engine/component/velocity_component.h"
#include "../../engine/component/render_component.h"
#include "../defs/tags.h"
#include "../../engine/component/audio_component.h"
#include "../component/stats_component.h"
#include "../component/enemy_component.h"
#include "../component/class_name_component.h"
#include "../component/player_component.h"
#include "../component/blocker_component.h"
#include "../component/projectile_component.h"
#include <entt/entity/registry.hpp>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

using namespace entt::literals;

namespace game::factory {

    EntityFactory::EntityFactory(entt::registry& registry,
        BlueprintManager& blueprint_manager)
        : mRegistry(registry), mBlueprintManager(blueprint_manager) {
    }

    entt::entity EntityFactory::createPlayerUnit(entt::id_type class_id, const glm::vec2& position, int level, int rarity) {
        auto entity = mRegistry.create();
        const auto& blueprint = mBlueprintManager.getPlayerClassBlueprint(class_id);
        // 添加组件
        addTransformComponent(entity, position);
        addSpriteComponent(entity, blueprint.mSprite);
        addAnimationComponent(entity, blueprint.mAnimations, blueprint.mSprite, "idle"_hs);
        addAudioComponent(entity, blueprint.mSounds);
        addStatsComponent(entity, blueprint.mStats, level, rarity);
        addPlayerComponent(entity, blueprint.mPlayer, rarity);
        addProjectileIDComponent(entity, blueprint.mProjectileId);

        // 补充其他必要组件
        mRegistry.emplace<game::component::ClassNameComponent>(entity, class_id, blueprint.mDisplayInfo.mName);
        mRegistry.emplace<engine::component::RenderComponent>(entity);
        mRegistry.emplace<game::defs::HasHealthBarTag>(entity);

        return entity;
    }

    entt::entity EntityFactory::createEnemyUnit(entt::id_type class_id, const glm::vec2& position, int target_waypoint_id, int level, int rarity) {
        auto entity = mRegistry.create();
        const auto& blueprint = mBlueprintManager.getEnemyClassBlueprint(class_id);
        // 添加组件
        addTransformComponent(entity, position);
        addSpriteComponent(entity, blueprint.mSprite);
        addAnimationComponent(entity, blueprint.mAnimations, blueprint.mSprite, "walk"_hs);
        addAudioComponent(entity, blueprint.mSounds);
        addStatsComponent(entity, blueprint.mStats, level, rarity);
        addEnemyComponent(entity, blueprint.mEnemy, target_waypoint_id);
        addProjectileIDComponent(entity, blueprint.mProjectileId);

        // 补充其他必要组件
        mRegistry.emplace<game::component::ClassNameComponent>(entity, class_id, blueprint.mDisplayInfo.mName);
        mRegistry.emplace<engine::component::RenderComponent>(entity);  // 使用默认主图层
        mRegistry.emplace<game::defs::HasHealthBarTag>(entity);

        return entity;
    }

    entt::entity EntityFactory::createProjectile(entt::id_type id, const glm::vec2& start_position, const glm::vec2& target_position, entt::entity target, float damage) {
        // 创建投射物实体
        auto entity = mRegistry.create();
        const auto& blueprint = mBlueprintManager.getProjectileBlueprint(id);
        // --- 依次添加必要组件 ---
        // 添加ProjectileComponent
        mRegistry.emplace<game::component::ProjectileComponent>(entity,
            target,
            damage,
            start_position,
            target_position,
            start_position,
            blueprint.mArcHeight,
            blueprint.mTotalFlightTime,
            0.0f);
        // 添加SpriteComponent
        addSpriteComponent(entity, blueprint.mSprite);
        // 添加TransformComponent
        addTransformComponent(entity, start_position);
        // 添加AudioComponent
        addAudioComponent(entity, blueprint.mSounds);
        // 添加RenderComponent(让投射物位于主图层+1，即可以遮住角色)
        mRegistry.emplace<engine::component::RenderComponent>(entity, engine::component::RenderComponent::MAIN_LAYER + 1);
        return entity;
    }

    entt::entity EntityFactory::createEnemyDeadEffect(entt::id_type class_id, const glm::vec2& position, const bool is_flipped) {
        auto entity = mRegistry.create();
        const auto& blueprint = mBlueprintManager.getEnemyClassBlueprint(class_id);
        // 添加Transform组件
        addTransformComponent(entity, position);
        // 添加Sprite组件
        addSpriteComponent(entity, blueprint.mSprite, is_flipped);
        // 添加Animation组件(死亡动画名称为"damage")
        addOneAnimationComponent(entity, blueprint.mAnimations.at("damage"_hs), blueprint.mSprite, "damage"_hs);

        // 补充其他必要组件
        mRegistry.emplace<engine::component::RenderComponent>(entity);
        mRegistry.emplace<game::defs::OneShotRemoveTag>(entity);
        return entity;
    }

    // --- 组件创建函数 ---

    void EntityFactory::addTransformComponent(entt::entity entity, const glm::vec2& position, const glm::vec2& scale, float rotation) {
        mRegistry.emplace<engine::component::TransformComponent>(entity, position, scale, rotation);
    }

    void EntityFactory::addSpriteComponent(entt::entity entity, const data::SpriteBlueprint& sprite, const bool is_flipped) {
        mRegistry.emplace<engine::component::SpriteComponent>(entity,
            engine::component::Sprite(sprite.mPath,
                sprite.mSrcRect,
                is_flipped),
            sprite.mSize,
            sprite.mOffset);
        // 如果图片朝左就添加 FaceLeftTag
        if (!sprite.mFaceRight) {
            mRegistry.emplace<game::defs::FaceLeftTag>(entity);
        }
    }

    void EntityFactory::addAnimationComponent(entt::entity entity,
        const std::unordered_map<entt::id_type, data::AnimationBlueprint>& animation_blueprints,
        const data::SpriteBlueprint& sprite_blueprint,
        entt::id_type default_animation_id) {
        // 先创建动画map容器
        std::unordered_map<entt::id_type, engine::component::Animation> animations;
        for (const auto& [anim_id, anim_blueprint] : animation_blueprints) {
            std::vector<engine::component::AnimationFrame> frames;
            for (const auto& frame_index : anim_blueprint.mFrames) {
                engine::utils::Rect source_rect = sprite_blueprint.mSrcRect;
                source_rect.mPosition.x += frame_index * source_rect.mSize.x;
                source_rect.mPosition.y += anim_blueprint.mRow * source_rect.mSize.y;
                frames.emplace_back(source_rect, anim_blueprint.mMsPerFrame);
            }
            // 将创建好的动画帧容器插入动画map容器 (可直接使用蓝图中的事件信息)
            animations.emplace(anim_id, engine::component::Animation(std::move(frames), anim_blueprint.mEvents));
        }
        mRegistry.emplace<engine::component::AnimationComponent>(entity, std::move(animations), default_animation_id);
    }

    void EntityFactory::addOneAnimationComponent(entt::entity entity,
        const data::AnimationBlueprint& animation_blueprint,
        const data::SpriteBlueprint& sprite_blueprint,
        entt::id_type animation_id,
        bool loop) {
        // 创建动画帧容器
        std::vector<engine::component::AnimationFrame> frames;
        // 依次读取动画蓝图中每一个动画帧，并插入容器
        for (const auto& frame_index : animation_blueprint.mFrames) {
            engine::utils::Rect source_rect = sprite_blueprint.mSrcRect;
            source_rect.mPosition.x += frame_index * source_rect.mSize.x;
            source_rect.mPosition.y += animation_blueprint.mRow * source_rect.mSize.y;
            frames.emplace_back(source_rect, animation_blueprint.mMsPerFrame);
        }
        // 创建动画map容器 (只有一个动画)
        std::unordered_map<entt::id_type, engine::component::Animation> animations;
        // 将创建好的动画帧容器插入动画map容器，注意传入loop参数
        animations.emplace(animation_id, engine::component::Animation(std::move(frames), animation_blueprint.mEvents, loop));
        // 通过动画map容器创建动画组件
        mRegistry.emplace<engine::component::AnimationComponent>(entity, std::move(animations), animation_id);
    }

    void EntityFactory::addStatsComponent(entt::entity entity, const data::StatsBlueprint& stats, int level, int rarity) {
        // 计算等级和稀有度对属性的影响（未来可改成数据驱动方便调整）
        auto hp = engine::utils::statModify(stats.mHp, level, rarity);
        auto atk = engine::utils::statModify(stats.mAtk, level, rarity);
        auto def = engine::utils::statModify(stats.mDef, level, rarity);

        mRegistry.emplace_or_replace<game::component::StatsComponent>(entity,
            hp,
            hp,
            atk,
            def,
            stats.mRange,
            stats.mAtkInterval,
            0.0f,
            level,
            rarity);
    }

    void EntityFactory::addPlayerComponent(entt::entity entity, const data::PlayerBlueprint& player, int rarity) {
        auto cost = static_cast<int>(std::round(player.mCost * (0.9f + 0.1f * rarity)));
        mRegistry.emplace<game::component::PlayerComponent>(entity, cost);
        // 添加类型标签（近战、远程、治疗）
        if (player.mType == game::defs::PlayerType::MELEE) {
            mRegistry.emplace<game::defs::MeleeUnitTag>(entity);    // 近战单位标签
            // 近战类型添加阻挡者组件
            mRegistry.emplace<game::component::BlockerComponent>(entity, player.mBlock);
        }
        else if (player.mType == game::defs::PlayerType::RANGED) {
            mRegistry.emplace<game::defs::RangedUnitTag>(entity);    // 远程单位标签
            if (player.mHealer) {
                mRegistry.emplace<game::defs::HealerTag>(entity);    // 治疗单位标签
            }
        }
        // TODO: 未来添加技能组件
    }

    void EntityFactory::addEnemyComponent(entt::entity entity, const data::EnemyBlueprint& enemy, int target_waypoint_id) {
        mRegistry.emplace<game::component::EnemyComponent>(entity, target_waypoint_id, enemy.mSpeed);
        mRegistry.emplace<engine::component::VelocityComponent>(entity, glm::vec2(0, 0));
        if (enemy.mRanged) {
            mRegistry.emplace<game::defs::RangedUnitTag>(entity);
        }
        else {
            mRegistry.emplace<game::defs::MeleeUnitTag>(entity);
        }
    }

    void EntityFactory::addAudioComponent(entt::entity entity, const data::SoundBlueprint& sounds) {
        if (sounds.mSounds.empty()) return;
        std::unordered_map<entt::id_type, entt::id_type> audio_map;
        for (const auto& [sound_key, sound_id] : sounds.mSounds) {
            audio_map.emplace(sound_key, sound_id);
        }
        mRegistry.emplace<engine::component::AudioComponent>(entity, std::move(audio_map));
    }

    void EntityFactory::addProjectileIDComponent(entt::entity entity, entt::id_type id) {
        if (id == entt::null) return;
        mRegistry.emplace<game::component::ProjectileIDComponent>(entity, id);
    }

}   // namespace game::factory
