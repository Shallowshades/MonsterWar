#include "skill_system.h"
#include "../defs/tags.h"
#include "../factory/entity_factory.h"
#include "../factory/blueprint_manager.h"
#include "../component/skill_component.h"
#include "../component/cost_regen_component.h"
#include "../component/stats_component.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/utils/events.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace game::system {

    SkillSystem::SkillSystem(entt::registry& registry, entt::dispatcher& dispatcher, game::factory::EntityFactory& entity_factory)
        : mRegistry(registry), mDispatcher(dispatcher), mEntityFactory(entity_factory) {
        mDispatcher.sink<game::defs::SkillReadyEvent>().connect<&SkillSystem::onSkillReadyEvent>(this);
        mDispatcher.sink<game::defs::SkillActiveEvent>().connect<&SkillSystem::onSkillActiveEvent>(this);
        mDispatcher.sink<game::defs::SkillDurationEndEvent>().connect<&SkillSystem::onSkillDurationEndEvent>(this);
        mDispatcher.sink<game::defs::RemovePlayerUnitEvent>().connect<&SkillSystem::onRemoveUnitEvent>(this);
    }

    SkillSystem::~SkillSystem() {
        mDispatcher.disconnect(this);
    }

    // --- 事件回调函数 ---

    void SkillSystem::onSkillReadyEvent(const game::defs::SkillReadyEvent& event) {
        if (event.mEntity == entt::null || !mRegistry.valid(event.mEntity)) return;
        // 获取技能和位置组件
        auto& skill = mRegistry.get<game::component::SkillComponent>(event.mEntity);
        const auto& transform = mRegistry.get<engine::component::TransformComponent>(event.mEntity);
        // 先删除可能存在的显示实体
        if (skill.mDisplayEntity != entt::null && mRegistry.valid(skill.mDisplayEntity)) {
            mRegistry.emplace_or_replace<game::defs::DeadTag>(skill.mDisplayEntity);
        }
        // 创建新的显示实体 (技能准备就绪)
        skill.mDisplayEntity = mEntityFactory.createSkillDisplay("skill_ready"_hs, transform.mPosition + game::defs::SKILL_DISPLAY_OFFSET);
    }

    void SkillSystem::onSkillActiveEvent(const game::defs::SkillActiveEvent& event) {
        if (event.mEntity == entt::null || !mRegistry.valid(event.mEntity)) return;
        // 如果技能未就绪，则返回
        if (!mRegistry.any_of<game::defs::SkillReadyTag>(event.mEntity)) return;

        // 获取技能和位置组件
        auto& skill = mRegistry.get<game::component::SkillComponent>(event.mEntity);
        const auto& transform = mRegistry.get<engine::component::TransformComponent>(event.mEntity);
        // 删除可能存在的显示实体
        if (skill.mDisplayEntity != entt::null && mRegistry.valid(skill.mDisplayEntity)) {
            mRegistry.emplace_or_replace<game::defs::DeadTag>(skill.mDisplayEntity);
        }
        // 创建新的显示实体 (技能激活)
        skill.mDisplayEntity = mEntityFactory.createSkillDisplay("skill_active"_hs, transform.mPosition + game::defs::SKILL_DISPLAY_OFFSET);

        // 移除技能准备标签，添加技能激活标签
        mRegistry.remove<game::defs::SkillReadyTag>(event.mEntity);
        mRegistry.emplace<game::defs::SkillActiveTag>(event.mEntity);

        // 如果技能是盾御，且动作未锁定，则播放guard动画
        if (skill.mSkillId == "shield"_hs && !mRegistry.any_of<game::defs::ActionLockTag>(event.mEntity)) {
            mDispatcher.enqueue(engine::utils::PlayAnimationEvent{ event.mEntity, "guard"_hs, true });
        }

        // 添加Buff
        addBuff(event.mEntity, skill.mSkillId);
    }

    void SkillSystem::onSkillDurationEndEvent(const game::defs::SkillDurationEndEvent& event) {
        if (event.mEntity == entt::null || !mRegistry.valid(event.mEntity)) return;
        // 获取技能组件
        auto& skill = mRegistry.get<game::component::SkillComponent>(event.mEntity);
        // 删除技能显示实体
        if (skill.mDisplayEntity != entt::null && mRegistry.valid(skill.mDisplayEntity)) {
            mRegistry.emplace_or_replace<game::defs::DeadTag>(skill.mDisplayEntity);
        }
        // 移除技能激活标签（TimerSystem 可能已移除，EnTT remove 缺失返回 0 不抛，安全）
        mRegistry.remove<game::defs::SkillActiveTag>(event.mEntity);

        // 如果技能是盾御，且动作未锁定，则播放idle动画
        if (skill.mSkillId == "shield"_hs && !mRegistry.any_of<game::defs::ActionLockTag>(event.mEntity)) {
            mDispatcher.enqueue(engine::utils::PlayAnimationEvent{ event.mEntity, "idle"_hs, true });
        }

        // 移除Buff
        removeBuff(event.mEntity, skill.mSkillId);
    }

    void SkillSystem::onRemoveUnitEvent(const game::defs::RemovePlayerUnitEvent& event) {
        if (event.mEntity == entt::null || !mRegistry.valid(event.mEntity)) return;
        // 移除技能显示实体
        if (auto skill = mRegistry.try_get<game::component::SkillComponent>(event.mEntity); skill) {
            if (skill->mDisplayEntity != entt::null && mRegistry.valid(skill->mDisplayEntity)) {
                mRegistry.emplace_or_replace<game::defs::DeadTag>(skill->mDisplayEntity);
            }
        }
    }

    // --- Buff增删函数 ---

    void SkillSystem::addBuff(entt::entity entity, entt::id_type skill_id) {
        if (entity == entt::null || !mRegistry.valid(entity)) return;

        // 获取Buff信息
        auto blueprint_mgr = mRegistry.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
        const auto& skill_blueprint = blueprint_mgr->getSkillBlueprint(skill_id);
        const auto& buff_blueprint = skill_blueprint.mBuff;

        // 将Buff应用到角色的Stats中
        auto& stats = mRegistry.get<game::component::StatsComponent>(entity);
        stats.mHp *= buff_blueprint.mHpMultiplier;
        stats.mAtk *= buff_blueprint.mAtkMultiplier;
        stats.mDef *= buff_blueprint.mDefMultiplier;
        stats.mRange *= buff_blueprint.mRangeMultiplier;
        stats.mAtkInterval *= buff_blueprint.mAtkIntervalMultiplier;

        // 若存在Cost相关Buff，则添加COST恢复组件
        if (buff_blueprint.mCostRegen > 0.0f) {
            mRegistry.emplace_or_replace<game::component::CostRegenComponent>(entity, buff_blueprint.mCostRegen);
        }
    }

    void SkillSystem::removeBuff(entt::entity entity, entt::id_type skill_id) {
        if (entity == entt::null || !mRegistry.valid(entity)) return;
        // 获取Buff信息
        auto blueprint_mgr = mRegistry.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
        const auto& skill_blueprint = blueprint_mgr->getSkillBlueprint(skill_id);
        const auto& buff_blueprint = skill_blueprint.mBuff;

        // 从角色的Stats中移除Buff（乘除对称，除回原值）
        auto& stats = mRegistry.get<game::component::StatsComponent>(entity);
        stats.mHp /= buff_blueprint.mHpMultiplier;
        stats.mAtk /= buff_blueprint.mAtkMultiplier;
        stats.mDef /= buff_blueprint.mDefMultiplier;
        stats.mRange /= buff_blueprint.mRangeMultiplier;
        stats.mAtkInterval /= buff_blueprint.mAtkIntervalMultiplier;

        // 若存在Cost相关Buff，则移除COST恢复组件
        if (buff_blueprint.mCostRegen > 0.0f) {
            mRegistry.remove<game::component::CostRegenComponent>(entity);
        }
    }

}   // namespace game::system
