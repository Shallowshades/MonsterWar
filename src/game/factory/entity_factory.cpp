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

        // 补充其他必要组件
        mRegistry.emplace<game::component::ClassNameComponent>(entity, class_id, blueprint.mDisplayInfo.mName);
        mRegistry.emplace<engine::component::RenderComponent>(entity);

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

        // 补充其他必要组件
        mRegistry.emplace<game::component::ClassNameComponent>(entity, class_id, blueprint.mDisplayInfo.mName);
        mRegistry.emplace<engine::component::RenderComponent>(entity);  // 使用默认主图层

        return entity;
    }

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
            animations.emplace(anim_id, engine::component::Animation(std::move(frames)));
        }
        mRegistry.emplace<engine::component::AnimationComponent>(entity, std::move(animations), default_animation_id);
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

}   // namespace game::factory
