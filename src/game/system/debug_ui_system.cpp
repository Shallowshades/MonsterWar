/*****************************************************************//**
 * @file   debug_ui_system.cpp
 * @brief  调试UI系统实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.08.21
 *********************************************************************/

#include "debug_ui_system.h"
#include "../component/stats_component.h"
#include "../component/class_name_component.h"
#include "../component/blocker_component.h"
#include "../component/skill_component.h"
#include "../component/player_component.h"
#include "../defs/tags.h"
#include "../defs/events.h"
#include "../data/game_stats.h"
#include "../data/level_data.h"
#include "../data/session_data.h"
#include "../factory/blueprint_manager.h"
#include "../../engine/component/name_component.h"
#include "../../engine/core/context.h"
#include "../../engine/core/game_state.h"
#include "../../engine/core/time.h"
#include "../../engine/audio/audio_player.h"
#include "../../engine/utils/math.h"
#include "../../engine/render/renderer.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <cmath>
#include <spdlog/spdlog.h>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace game::system {

    DebugUISystem::DebugUISystem(entt::registry& registry, engine::core::Context& context)
        : mRegistry(registry), mContext(context) {
        // 订阅肖像悬浮事件
        auto& dispatcher = mContext.getDispatcher();
        dispatcher.sink<game::defs::UIPortraitHoverEnterEvent>().connect<&DebugUISystem::onUIPortraitHoverEnterEvent>(this);
        dispatcher.sink<game::defs::UIPortraitHoverLeaveEvent>().connect<&DebugUISystem::onUIPortraitHoverLeaveEvent>(this);
    }

    DebugUISystem::~DebugUISystem() {
        // 断开所有事件连接
        mContext.getDispatcher().disconnect(this);
    }

    void DebugUISystem::update() {
        beginFrame();
        renderHoveredPortrait();
        renderHoveredUnit();
        renderSelectedUnit();
        renderInfoUI();
        renderSettingUI();
        renderDebugUI();
        endFrame();
    }

    void DebugUISystem::beginFrame() {
        // 开始新帧
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 关闭逻辑分辨率 (ImGui目前对于SDL逻辑分辨率支持不好，所以使用时先关闭)
        if (!mContext.getGameState().disableLogicalPresentation()) {
            spdlog::error("关闭逻辑分辨率失败");
        }
    }

    void DebugUISystem::endFrame() {
        // ImGui: 渲染
        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mContext.getRenderer().getSDLRenderer());

        // 渲染完成后，打开(恢复)逻辑分辨率
        if (!mContext.getGameState().enableLogicalPresentation()) {
            spdlog::error("启用逻辑分辨率失败");
        }
    }

    void DebugUISystem::renderHoveredPortrait() {
        // 确定鼠标悬浮的单位肖像存在
        if (mHoveredPortrait == entt::null) return;

        // 角色名不是实体，需要从会话数据与蓝图中获取数据
        const auto& session_data = mRegistry.ctx().get<std::shared_ptr<game::data::SessionData>>();
        const auto& blueprint_mgr = mRegistry.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
        const auto& unit_data = session_data->getUnitData(mHoveredPortrait);
        const auto& class_blueprint = blueprint_mgr->getPlayerClassBlueprint(unit_data.mClassId);
        const auto& stats = class_blueprint.mStats;
        // 计算等级和稀有度对属性的影响
        const auto hp = engine::utils::statModify(stats.mHp, unit_data.mLevel, unit_data.mRarity);
        const auto atk = engine::utils::statModify(stats.mAtk, unit_data.mLevel, unit_data.mRarity);
        const auto def = engine::utils::statModify(stats.mDef, unit_data.mLevel, unit_data.mRarity);
        const auto range = stats.mRange;
        const auto& class_name = class_blueprint.mDisplayInfo.mName;

        // 显示Tooltip信息
        if (!ImGui::BeginTooltip()) {
            ImGui::EndTooltip();
            spdlog::error("鼠标悬浮单位肖像窗口打开失败");
            return;
        }
        ImGui::Text("%s", unit_data.mName.c_str());
        ImGui::SameLine();
        ImGui::Text("职业: %s", class_name.c_str());
        ImGui::Text("等级: %d", unit_data.mLevel);
        ImGui::SameLine();
        ImGui::Text("稀有度: %d", unit_data.mRarity);
        ImGui::Text("生命值: %d", static_cast<int>(std::round(hp)));
        ImGui::SameLine();
        ImGui::Text("攻击力: %d", static_cast<int>(std::round(atk)));
        ImGui::Text("防御力: %d", static_cast<int>(std::round(def)));
        ImGui::SameLine();
        ImGui::Text("攻击范围: %d", static_cast<int>(std::round(range)));
        ImGui::EndTooltip();
    }

    void DebugUISystem::renderHoveredUnit() {
        // 确定鼠标悬浮的单位存在
        auto& entity = mRegistry.ctx().get<entt::entity&>("hovered_unit"_hs);
        if (entity == entt::null || !mRegistry.valid(entity)) return;

        // Tooltip 是悬浮在鼠标上的小窗口，可以显示单位信息
        if (!ImGui::BeginTooltip()) {
            ImGui::EndTooltip();
            spdlog::error("鼠标悬浮单位窗口打开失败");
            return;
        }
        // 获取必要信息并显示
        const auto& stats = mRegistry.get<game::component::StatsComponent>(entity);
        const auto& class_name = mRegistry.get<game::component::ClassNameComponent>(entity);
        // 只有玩家单位才有姓名，所以需要尝试获取
        if (auto name = mRegistry.try_get<engine::component::NameComponent>(entity); name) {
            ImGui::Text("%s  ", name->mName.c_str());
            ImGui::SameLine();
        }
        ImGui::Text("%s", class_name.mClassName.c_str());
        ImGui::Text("等级: %d", stats.mLevel);
        ImGui::SameLine();
        ImGui::Text("稀有度: %d", stats.mRarity);
        ImGui::Text("生命值: %d/%d", static_cast<int>(std::round(stats.mHp)), static_cast<int>(std::round(stats.mMaxHp)));
        ImGui::Text("攻击力: %d", static_cast<int>(std::round(stats.mAtk)));
        ImGui::Text("防御力: %d", static_cast<int>(std::round(stats.mDef)));
        ImGui::Text("攻击范围: %d", static_cast<int>(std::round(stats.mRange)));
        ImGui::Text("攻击间隔: %.2f", stats.mAtkInterval);
        ImGui::EndTooltip();
    }

    void DebugUISystem::renderSelectedUnit() {
        // 确定选中的单位存在
        auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
        if (entity == entt::null || !mRegistry.valid(entity)) return;

        // 设置窗口位置在左上角
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);

        if (!ImGui::Begin("角色状态", nullptr, ImGuiWindowFlags_NoTitleBar)) {
            ImGui::End();
            spdlog::error("角色状态窗口打开失败");
            return;
        }
        // 获取必要信息并显示
        const auto& stats = mRegistry.get<game::component::StatsComponent>(entity);
        const auto& class_name = mRegistry.get<game::component::ClassNameComponent>(entity);
        const auto blocker = mRegistry.try_get<game::component::BlockerComponent>(entity);
        if (auto name = mRegistry.try_get<engine::component::NameComponent>(entity); name) {
            ImGui::Text("%s  ", name->mName.c_str());
            ImGui::SameLine();
        }
        ImGui::Text("%s", class_name.mClassName.c_str());
        ImGui::Text("等级: %d", stats.mLevel);
        ImGui::SameLine();
        ImGui::Text("稀有度: %d", stats.mRarity);
        ImGui::Text("生命值: %d/%d", static_cast<int>(std::round(stats.mHp)), static_cast<int>(std::round(stats.mMaxHp)));
        ImGui::Text("攻击力: %d", static_cast<int>(std::round(stats.mAtk)));
        ImGui::SameLine();
        ImGui::Text("防御力: %d", static_cast<int>(std::round(stats.mDef)));
        ImGui::Text("攻击范围: %d", static_cast<int>(std::round(stats.mRange)));
        ImGui::SameLine();
        ImGui::Text("攻击间隔: %.2f", stats.mAtkInterval);
        if (blocker) {
            ImGui::Text("阻挡数量: %d/%d", blocker->mCurrentCount, blocker->mMaxCount);
        }
        // 升级，消耗COST与出击COST相同
        const auto& player = mRegistry.get<game::component::PlayerComponent>(entity);
        auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
        // COST资源充足时升级按钮才可用
        ImGui::BeginDisabled(game_stats.mCost < player.mCost);
        // 设置快捷键 U 升级
        ImGui::SetNextItemShortcut(ImGuiKey_U, ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Tooltip);
        if (ImGui::Button("升级")) {
            mContext.getDispatcher().enqueue(game::defs::UpgradeUnitEvent{ entity, player.mCost });
        }
        ImGui::SameLine();
        ImGui::Text("快捷键 U: COST消费: %d", player.mCost);
        ImGui::EndDisabled();

        // 撤退，返回 50% 的COST
        auto return_cost = static_cast<int>(player.mCost * 0.5f);
        // 设置快捷键 R 撤退
        ImGui::SetNextItemShortcut(ImGuiKey_R, ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Tooltip);
        if (ImGui::Button("撤退")) {
            mContext.getDispatcher().enqueue(game::defs::RetreatEvent{ entity, return_cost });
        }
        ImGui::SameLine();
        ImGui::Text("快捷键 R: COST返还: %d", return_cost);

        // 技能显示与交互
        if (auto skill = mRegistry.try_get<game::component::SkillComponent>(entity); skill) {
            // 如果技能准备就绪，则按钮可用（激活技能），否则按钮不可用
            auto ready = mRegistry.all_of<game::defs::SkillReadyTag>(entity);
            ImGui::BeginDisabled(!ready);
            // 设置快捷键 S 激活技能
            ImGui::SetNextItemShortcut(ImGuiKey_S, ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Tooltip);
            if (ImGui::Button(skill->mName.c_str())) {
                // 激活技能
                mContext.getDispatcher().enqueue<game::defs::SkillActiveEvent>(entity);
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            // 如果技能激活中，则显示"剩余时间"或"被动技能激活中"
            if (mRegistry.all_of<game::defs::SkillActiveTag>(entity)) {
                if (mRegistry.all_of<game::defs::PassiveSkillTag>(entity)) {
                    ImGui::Text("被动技能激活中");
                } else {
                    ImGui::Text("激活中，剩余时间: %.1f 秒", skill->mDuration - skill->mDurationTimer);
                }
                // 否则显示冷却时间
            } else {
                ImGui::Text("快捷键 S: ");
                ImGui::SameLine();
                if (mRegistry.all_of<game::defs::SkillReadyTag>(entity)) {
                    ImGui::Text("技能准备就绪");
                } else {
                    // 用进度条显示冷却时间百分比
                    ImGui::ProgressBar(skill->mCooldownTimer / skill->mCooldown);
                }
            }
            // 显示技能描述
            ImGui::TextWrapped("%s", skill->mDescription.c_str());
        }
        ImGui::End();
    }

    void DebugUISystem::renderInfoUI() {
        if (!ImGui::Begin("关卡信息", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            spdlog::error("关卡信息窗口打开失败");
            return;
        }
        // 获取关卡相关数据
        const auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
        const auto& waves = mRegistry.ctx().get<game::data::Waves&>();
        const auto& session_data = mRegistry.ctx().get<std::shared_ptr<game::data::SessionData>>();
        // 显示
        ImGui::Text("基地血量: %d / 5", game_stats.mHomeHp);
        ImGui::SameLine();
        ImGui::Text("COST: %d", static_cast<int>(game_stats.mCost));
        ImGui::SameLine();
        ImGui::Text("剩余波次: %d", static_cast<int>(waves.mWaves.size()));
        ImGui::SameLine();
        if (waves.mWaves.size() > 0) {
            ImGui::Text("下一波时间: %d", static_cast<int>(waves.mNextWaveCountDown));
        }
        ImGui::SameLine();
        ImGui::Text("击杀数量: %d / %d", game_stats.mEnemyKilledCount, game_stats.mEnemyCount);
        ImGui::SameLine();
        ImGui::Text("当前关卡: %d", session_data->getLevelNumber());
        ImGui::End();
    }

    void DebugUISystem::renderSettingUI() {
        if (!ImGui::Begin("设置工具", nullptr, ImGuiWindowFlags_NoTitleBar)) {
            ImGui::End();
            spdlog::error("设置工具窗口打开失败");
            return;
        }
        // 场景控制
        auto& game_state = mContext.getGameState();
        // 暂停/继续 用 P 键切换
        ImGui::SetNextItemShortcut(ImGuiKey_P, ImGuiInputFlags_RouteAlways | ImGuiInputFlags_Tooltip);
        if (game_state.isPaused()) {    // 如果游戏暂停，则显示“继续游戏”按钮，快捷键 P
            if (ImGui::Button("继续游戏")) {
                game_state.setState(engine::core::State::Playing);
            }
        }
        else {        // 如果游戏运行中，则显示“暂停游戏”按钮，快捷键也是 P
            if (ImGui::Button("暂停游戏")) {
                game_state.setState(engine::core::State::Paused);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("重新开始")) {
            mContext.getDispatcher().enqueue(game::defs::RestartEvent{});
        }
        if (ImGui::Button("返回标题")) {
            mContext.getDispatcher().enqueue(game::defs::BackToTitleEvent{});
        }
        ImGui::SameLine();
        if (ImGui::Button("保存")) {
            mContext.getDispatcher().enqueue(game::defs::SaveEvent{});
        }
        ImGui::Separator();

        // 游戏速度调节
        auto& time = mContext.getTime();
        float time_scale = time.getTimeScale();
        if (ImGui::Button("0.5倍速")) {
            time_scale = 0.5f;
            time.setTimeScale(time_scale);
        }
        ImGui::SameLine();
        if (ImGui::Button("1倍速")) {
            time_scale = 1.0f;
            time.setTimeScale(time_scale);
        }
        ImGui::SameLine();
        if (ImGui::Button("2倍速")) {
            time_scale = 2.0f;
            time.setTimeScale(time_scale);
        }
        ImGui::SliderFloat("游戏速度", &time_scale, 0.5f, 2.0f);
        time.setTimeScale(time_scale);

        // 音乐音量调节
        float music_volume = mContext.getAudioPlayer().getMusicVolume();
        ImGui::SliderFloat("音乐音量", &music_volume, 0.0f, 1.0f);
        mContext.getAudioPlayer().setMusicVolume(music_volume);
        float sound_volume = mContext.getAudioPlayer().getSoundVolume();
        ImGui::SliderFloat("音效音量", &sound_volume, 0.0f, 1.0f);
        mContext.getAudioPlayer().setSoundVolume(sound_volume);

        // 切换调试工具显示 （勾选结果保存在 mShowDebugUI 中）
        ImGui::Checkbox("显示调试工具", &mShowDebugUI);
        ImGui::End();
    }

    void DebugUISystem::renderDebugUI() {
        if (!mShowDebugUI) return;
        if (!ImGui::Begin("调试工具", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            spdlog::error("调试工具窗口打开失败");
            return;
        }
        auto& game_stats = mRegistry.ctx().get<game::data::GameStats&>();
        if (ImGui::Button("COST + 10")) {
            game_stats.mCost += 10;
        }
        if (ImGui::Button("COST + 100")) {
            game_stats.mCost += 100;
        }
        if (ImGui::Button("通关")) {
            mContext.getDispatcher().enqueue(game::defs::LevelClearEvent{});
        }
        // TODO: 未来可按需添加其他调试工具
        ImGui::End();
    }

    // 事件回调函数
    void DebugUISystem::onUIPortraitHoverEnterEvent(const game::defs::UIPortraitHoverEnterEvent& event) {
        mHoveredPortrait = event.mNameId;
    }

    void DebugUISystem::onUIPortraitHoverLeaveEvent(const game::defs::UIPortraitHoverLeaveEvent&) {
        mHoveredPortrait = entt::null;
    }

}   // namespace game::system
