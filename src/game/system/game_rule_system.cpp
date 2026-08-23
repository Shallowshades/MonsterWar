#include "game_rule_system.h"
#include "../data/game_stats.h"
#include "../factory/blueprint_manager.h"
#include "../component/cost_regen_component.h"
#include "../component/stats_component.h"
#include "../component/class_name_component.h"
#include "../../engine/component/transform_component.h"
#include "../../engine/utils/math.h"
#include "../../engine/utils/events.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>

using namespace entt::literals;

namespace game::system {

    GameRuleSystem::GameRuleSystem(entt::registry& registry, entt::dispatcher& dispatcher)
        : mRegistry(registry), mDispatcher(dispatcher) {
        mDispatcher.sink<game::defs::EnemyArriveHomeEvent>().connect<&GameRuleSystem::onEnemyArriveHome>(this);
        mDispatcher.sink<game::defs::UpgradeUnitEvent>().connect<&GameRuleSystem::onUpgradeUnitEvent>(this);
        mDispatcher.sink<game::defs::RetreatEvent>().connect<&GameRuleSystem::onRetreatEvent>(this);
        mDispatcher.sink<game::defs::LevelClearDelayedEvent>().connect<&GameRuleSystem::onLevelClearDelayedEvent>(this);
    }

    GameRuleSystem::~GameRuleSystem() {
        mDispatcher.disconnect(this);
    }

    void GameRuleSystem::update(float delta_time) {
        // 更新cost（基础生成速率）
        auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
        game_stats.mCost += game_stats.mCostGenPerSecond * delta_time;

        // 额外处理带 CostRegenComponent 的实体（如建筑）
        auto view_cost_regen = mRegistry.view<game::component::CostRegenComponent>();
        for (auto entity : view_cost_regen) {
            const auto& cost_regen = view_cost_regen.get<game::component::CostRegenComponent>(entity);
            game_stats.mCost += cost_regen.mRate * delta_time;
        }

        // 如果已通关，计时器归零后发送通关事件并切换场景
        if (mIsLevelClear) {
            mLevelClearTimer -= delta_time;
            if (mLevelClearTimer <= 0.0f) {
                mDispatcher.enqueue(game::defs::LevelClearEvent{});
                mIsLevelClear = false;    // 重置关卡通关标志，避免重复触发
            }
        }
    }

    void GameRuleSystem::onEnemyArriveHome(const game::defs::EnemyArriveHomeEvent&) {
        // 游戏已结束（基地被摧毁）后忽略后续到达的敌人，避免基地血量变负数、重复触发结束事件
        if (mIsGameOver) return;

        spdlog::info("敌人到达基地");
        auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
        game_stats.mEnemyArrivedCount++;      // 敌人到达数量+1
        game_stats.mHomeHp -= 1;              // 基地血量-1
        if (game_stats.mHomeHp <= 0) {
            spdlog::warn("基地被摧毁");
            mIsGameOver = true;               // 标记游戏结束，忽略后续到达
            // 游戏失败
            mDispatcher.enqueue(game::defs::GameEndEvent{ false });
        }
        else if ((game_stats.mEnemyArrivedCount + game_stats.mEnemyKilledCount) >= game_stats.mEnemyCount) {
            // 全歼敌人，通关成功，延迟切换场景
            mDispatcher.enqueue(game::defs::LevelClearDelayedEvent{ 2.0f });
        }
    }

    void GameRuleSystem::onUpgradeUnitEvent(const game::defs::UpgradeUnitEvent& event) {
        if (event.mEntity == entt::null || !mRegistry.valid(event.mEntity)) return;
        // 扣除cost
        auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
        game_stats.mCost -= event.mCost;
        // 等级 + 1
        auto& stats = mRegistry.get<game::component::StatsComponent>(event.mEntity);
        stats.mLevel++;
        // 根据蓝图的初始属性，按等级和稀有度重算属性
        auto& blueprint_mgr = mRegistry.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
        const auto& class_name = mRegistry.get<game::component::ClassNameComponent>(event.mEntity);
        const auto& stats_blueprint = blueprint_mgr->getPlayerClassBlueprint(class_name.mClassId).mStats;
        stats.mHp = engine::utils::statModify(stats_blueprint.mHp, stats.mLevel, stats.mRarity);
        stats.mMaxHp = stats.mHp;
        stats.mAtk = engine::utils::statModify(stats_blueprint.mAtk, stats.mLevel, stats.mRarity);
        stats.mDef = engine::utils::statModify(stats_blueprint.mDef, stats.mLevel, stats.mRarity);
        // NOTE: 本地特效系统目前只支持敌人死亡特效，升级特效待后续扩展
        // 播放升级音效（通过实体的AudioComponent播放"level_up"音效）
        mDispatcher.enqueue(engine::utils::PlaySoundEvent{ event.mEntity, "level_up"_hs });
    }

    void GameRuleSystem::onRetreatEvent(const game::defs::RetreatEvent& event) {
        if (event.mEntity == entt::null || !mRegistry.valid(event.mEntity)) return;
        // 返还cost
        auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
        game_stats.mCost += event.mCost;
        // 发送移除单位事件
        mDispatcher.enqueue(game::defs::RemovePlayerUnitEvent{ event.mEntity });
    }

    void GameRuleSystem::onLevelClearDelayedEvent(const game::defs::LevelClearDelayedEvent& event) {
        // 设置关卡通关标志和计时器
        mIsLevelClear = true;
        mLevelClearTimer = event.mDelayTime;
    }

}   // namespace game::system
