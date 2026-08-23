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
#include "../defs/constants.h"
#include "../data/game_stats.h"
#include "../data/level_data.h"
#include "../data/session_data.h"
#include "../factory/blueprint_manager.h"
#include "../scene/title_scene.h"
#include "../scene/level_clear_scene.h"
#include "../scene/end_scene.h"
#include "../../engine/component/name_component.h"
#include "../../engine/core/context.h"
#include "../../engine/core/game_state.h"
#include "../../engine/core/time.h"
#include "../../engine/audio/audio_player.h"
#include "../../engine/resource/resource_manager.h"
#include "../../engine/utils/math.h"
#include "../../engine/render/renderer.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <algorithm>
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
        // 渲染可能激活的存档面板（GameScene 通过 ctx 暴露显示标志）
        auto& show_save_panel = mRegistry.ctx().get<bool&>("show_save_panel"_hs);
        renderSavePanelUI(show_save_panel);
        endFrame();
    }

    void DebugUISystem::updateTitle(game::scene::TitleScene& title_scene) {
        beginFrame();
        renderTitleLogo();
        renderTitleButtons(title_scene);
        // 渲染可能激活的角色信息和载入面板（friend 可直接访问TitleScene的私有成员）
        renderUnitInfoUI(title_scene.mShowUnitInfo);
        renderLoadPanelUI(title_scene.mShowLoadPanel);
        endFrame();
    }

    void DebugUISystem::updateLevelClear(game::scene::LevelClearScene& level_clear_scene) {
        beginFrame();
        renderLevelClearText();
        renderLevelClearTable(level_clear_scene);
        renderLevelClearButtons(level_clear_scene);
        // 渲染可能激活的存档面板（friend 可直接访问LevelClearScene的私有成员）
        renderSavePanelUI(level_clear_scene.mShowSavePanel);
        endFrame();
    }

    void DebugUISystem::updateEnd(game::scene::EndScene& end_scene) {
        beginFrame();
        renderEndText(end_scene);
        renderEndButtons(end_scene);
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

    // ----------------------------- TitleScene -----------------------------
    void DebugUISystem::renderTitleLogo() {
        if (!ImGui::Begin("TitleLogo", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground)) {
            ImGui::End();
            spdlog::error("TitleLogo窗口打开失败");
            return;
        }
        // 获取LOGO图片信息
        auto& resource_manager = mContext.getResourceManager();
        auto logo_texture = resource_manager.getTexture("assets/textures/UI/title.png"_hs);
        auto size = resource_manager.getTextureSize("assets/textures/UI/title.png"_hs);
        // 图片显示参数：SDL_Texture*, 显示尺寸(像素)。源矩形区域默认为整张图片。
        // ImGui(1.91+) 的 ImTextureID 为 ImU64，需要把 SDL_Texture* 显式转为整数指针
        ImGui::Image((ImTextureID)(intptr_t)logo_texture, ImVec2(size.x, size.y));
        ImGui::End();
    }

    void DebugUISystem::renderTitleButtons(game::scene::TitleScene& title_scene) {
        if (!ImGui::Begin("TitleUI", nullptr, ImGuiWindowFlags_NoTitleBar)) {
            ImGui::End();
            spdlog::error("TitleUI窗口打开失败");
            return;
        }
        // 设置按钮字体更大
        ImGui::SetWindowFontScale(2.0f);
        if (ImGui::Button("开始游戏", ImVec2(200, 60))) {
            title_scene.onStartGameClick();     // 直接调用TitleScene的私有函数，不需要通过dispatcher发信号
        }
        ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        if (ImGui::Button("确认角色", ImVec2(200, 60))) {
            title_scene.onConfirmRoleClick();
        }
        ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        if (ImGui::Button("载入游戏", ImVec2(200, 60))) {
            title_scene.onLoadGameClick();
        }
        ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        if (ImGui::Button("退出游戏", ImVec2(200, 60))) {
            title_scene.onQuitClick();
        }
        ImGui::SetWindowFontScale(1.0f);    // 恢复默认字体大小
        ImGui::End();
    }

    // ----------------------------- LevelClearScene -----------------------------
    void DebugUISystem::renderLevelClearText() {
        if (!ImGui::Begin("通关结算文本", nullptr, ImGuiWindowFlags_NoTitleBar)) {
            ImGui::End();
            spdlog::error("通关结算文本窗口打开失败");
            return;
        }
        ImGui::SetWindowFontScale(3.0f);
        ImGui::Text("通关成功！");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::End();
    }

    void DebugUISystem::renderLevelClearTable(game::scene::LevelClearScene& level_clear_scene) {
        if (!ImGui::Begin("通关结算", nullptr, ImGuiWindowFlags_NoTitleBar)) {
            ImGui::End();
            spdlog::error("通关结算窗口打开失败");
            return;
        }
        renderUnitTable();  // 支持排序的角色表格
        ImGui::Separator();
        // 显示部分通关统计信息（friend 直接访问私有成员）
        const auto& game_stats = level_clear_scene.mGameStats;
        const auto& session_data = level_clear_scene.mSessionData;
        ImGui::Text("关卡: %d", session_data->getLevelNumber());
        ImGui::SameLine();
        ImGui::Text("击杀数量: %d / %d", game_stats.mEnemyKilledCount, game_stats.mEnemyCount);
        ImGui::SameLine();
        ImGui::Text("基地血量: %d / 5", game_stats.mHomeHp);
        ImGui::SameLine();
        ImGui::Text("奖励点数: %d", game_stats.mEnemyKilledCount + game_stats.mHomeHp * 5);
        ImGui::SameLine();
        ImGui::Text("剩余点数: %d", session_data->getPoint());
        ImGui::End();
    }

    void DebugUISystem::renderLevelClearButtons(game::scene::LevelClearScene& level_clear_scene) {
        if (!ImGui::Begin("通关结算按钮", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            spdlog::error("通关结算按钮窗口打开失败");
            return;
        }
        ImGui::SetWindowFontScale(1.5f);
        if (ImGui::Button("下一关", ImVec2(150, 45))) {
            level_clear_scene.onNextLevelClick();     // 直接调用私有回调
        }
        ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        if (ImGui::Button("保存", ImVec2(150, 45))) {
            level_clear_scene.onSaveClick();
        }
        ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        if (ImGui::Button("返回标题", ImVec2(150, 45))) {
            level_clear_scene.onBackToTitleClick();
        }
        ImGui::SetWindowFontScale(1.0f);
        ImGui::End();
    }

    // ----------------------------- EndScene -----------------------------
    void DebugUISystem::renderEndText(game::scene::EndScene& end_scene) {
        if (!ImGui::Begin("游戏结束", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            spdlog::error("游戏结束窗口打开失败");
            return;
        }
        ImGui::SetWindowFontScale(5.0f);
        if (end_scene.mIsWin) {
            ImGui::Text("恭喜你，游戏胜利!");
        } else {
            ImGui::Text("游戏失败，请再接再厉！");
        }
        ImGui::SetWindowFontScale(1.0f);
        ImGui::End();
    }

    void DebugUISystem::renderEndButtons(game::scene::EndScene& end_scene) {
        if (!ImGui::Begin("游戏结束按钮", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::End();
            spdlog::error("游戏结束按钮窗口打开失败");
            return;
        }
        ImGui::SetWindowFontScale(1.5f);
        if (ImGui::Button("返回标题", ImVec2(150, 45))) {
            end_scene.onBackToTitleClick();
        }
        ImGui::SameLine(); ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20.0f);
        if (ImGui::Button("退出游戏", ImVec2(150, 45))) {
            end_scene.onQuitClick();
        }
        ImGui::SetWindowFontScale(1.0f);
        ImGui::End();
    }

    // ----------------------------- Shared（标题/游戏共用） -----------------------------
    void DebugUISystem::renderUnitInfoUI(bool& show_unit_info) {
        if (!show_unit_info) return;
        // 关闭窗口时，第二个参数(show_unit_info)会被设置为false，因此需要传入引用
        if (!ImGui::Begin("角色信息", &show_unit_info, ImGuiWindowFlags_NoCollapse)) {
            ImGui::End();
            spdlog::error("角色信息窗口打开失败");
            return;
        }
        renderUnitTable();
        ImGui::Separator();
        const auto session_data = mRegistry.ctx().get<std::shared_ptr<game::data::SessionData>>();
        ImGui::Text("剩余点数: %d", session_data->getPoint());
        ImGui::End();
    }

    void DebugUISystem::renderLoadPanelUI(bool& show_load_panel) {
        if (!show_load_panel) return;
        if (!ImGui::Begin("读档选择", &show_load_panel, ImGuiWindowFlags_NoCollapse)) {
            ImGui::End();
            spdlog::error("读档选择窗口打开失败");
            return;
        }
        const auto& session_data = mRegistry.ctx().get<std::shared_ptr<game::data::SessionData>>();
        if (ImGui::Button("SLOT 1")) {
            session_data->loadFromFile("assets/save/SLOT_1.json");
        }
        ImGui::SameLine();
        if (ImGui::Button("SLOT 2")) {
            session_data->loadFromFile("assets/save/SLOT_2.json");
        }
        ImGui::SameLine();
        if (ImGui::Button("SLOT 3")) {
            session_data->loadFromFile("assets/save/SLOT_3.json");
        }
        // 如果已经通关了，则提示将进入下一关，否则显示"当前关卡"
        if (session_data->isLevelClear()) {
            ImGui::Text("下一关: %d", session_data->getLevelNumber() + 1);
        } else {
            ImGui::Text("当前关卡: %d", session_data->getLevelNumber());
        }
        ImGui::End();
    }

    void DebugUISystem::renderSavePanelUI(bool& show_save_panel) {
        if (!show_save_panel) return;
        if (!ImGui::Begin("存档选择", &show_save_panel, ImGuiWindowFlags_NoCollapse)) {
            ImGui::End();
            spdlog::error("存档选择窗口打开失败");
            return;
        }
        const auto& session_data = mRegistry.ctx().get<std::shared_ptr<game::data::SessionData>>();
        if (ImGui::Button("SLOT 1")) {
            session_data->saveToFile("assets/save/SLOT_1.json");
        }
        ImGui::SameLine();
        if (ImGui::Button("SLOT 2")) {
            session_data->saveToFile("assets/save/SLOT_2.json");
        }
        ImGui::SameLine();
        if (ImGui::Button("SLOT 3")) {
            session_data->saveToFile("assets/save/SLOT_3.json");
        }
        // 根据是否已经通关，切换显示提示信息
        if (session_data->isLevelClear()) {
            ImGui::Text("下一关: %d", session_data->getLevelNumber() + 1);
        } else {
            ImGui::Text("当前关卡: %d", session_data->getLevelNumber());
        }
        ImGui::End();
    }

    void DebugUISystem::renderUnitTable() {
        // 显示表格，需指定列数(14)，标志位使用ImGuiTableFlags_Sortable，可以让表格支持排序
        if (!ImGui::BeginTable("角色信息", 14, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Sortable)) {
            ImGui::End();
            spdlog::error("角色信息表格打开失败");
            return;
        }
        // 定义标题列
        ImGui::TableSetupColumn("姓名");
        ImGui::TableSetupColumn("职业");
        ImGui::TableSetupColumn("类型");
        ImGui::TableSetupColumn("等级");
        ImGui::TableSetupColumn("稀有度");
        ImGui::TableSetupColumn("COST");
        ImGui::TableSetupColumn("生命值");
        ImGui::TableSetupColumn("攻击力");
        ImGui::TableSetupColumn("防御力");
        ImGui::TableSetupColumn("攻击范围");
        ImGui::TableSetupColumn("攻击间隔");
        ImGui::TableSetupColumn("阻挡数量");
        ImGui::TableSetupColumn("技能");
        ImGui::TableSetupColumn("升级");
        // 渲染标题行
        ImGui::TableHeadersRow();
        // 获取数据
        const auto session_data = mRegistry.ctx().get<std::shared_ptr<game::data::SessionData>>();
        auto& unit_data_list = session_data->getUnitDataList();
        const auto blueprint_manager = mRegistry.ctx().get<std::shared_ptr<game::factory::BlueprintManager>>();
        const auto ui_config = mRegistry.ctx().get<std::shared_ptr<game::data::UIConfig>>();

        // --- 点击标题列，就按照该列排序 ---
        // 获取排序规格参数 (当sort_specs->SpecsDirty为true时，表示点击了某一列标题，需要重新排序)
        if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs()) {
            if (sort_specs->SpecsDirty && !unit_data_list.empty()) {
                // 获取第一个（也是唯一的）排序规格参数 (多列排序会有更多规格参数)
                const ImGuiTableColumnSortSpecs& spec = sort_specs->Specs[0];
                const int col = spec.ColumnIndex;       // 获取列的索引，对应鼠标点击的列
                const bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);

                // 创建字符串比较函数 (如果左<右，返回-1；如果左>右，返回1；如果左==右，返回0)
                const auto compareStrings = [](const std::string& a, const std::string& b) {
                    return (a < b) ? -1 : (a > b ? 1 : 0);
                };

                // 执行排序
                std::stable_sort(unit_data_list.begin(), unit_data_list.end(),
                                 [&](const game::data::UnitData* lhs, const game::data::UnitData* rhs) {
                    // 获取职业蓝图（用于获取属性）
                    const auto& pcb_l = blueprint_manager->getPlayerClassBlueprint(lhs->mClassId);
                    const auto& pcb_r = blueprint_manager->getPlayerClassBlueprint(rhs->mClassId);

                    // delta用于记录比较结果（-1表示小于，0表示等于，1表示大于）
                    int delta = 0;

                    switch (col) {
                        case 0: {   // 姓名
                            delta = compareStrings(lhs->mName, rhs->mName);
                            break;
                        }
                        case 1: {   // 职业
                            delta = compareStrings(lhs->mClass, rhs->mClass);
                            break;
                        }
                        case 2: {   // 类型（近战、远程、混合）
                            const int type_l = static_cast<int>(pcb_l.mPlayer.mType);
                            const int type_r = static_cast<int>(pcb_r.mPlayer.mType);
                            delta = (type_l < type_r) ? -1 : (type_l > type_r ? 1 : 0);
                            break;
                        }
                        case 3: {   // 等级
                            delta = (lhs->mLevel < rhs->mLevel) ? -1 : (lhs->mLevel > rhs->mLevel ? 1 : 0);
                            break;
                        }
                        case 4: {   // 稀有度
                            delta = (lhs->mRarity < rhs->mRarity) ? -1 : (lhs->mRarity > rhs->mRarity ? 1 : 0);
                            break;
                        }
                        case 5: {   // COST
                            // 要考虑相等的情况，因此使用int排序而非float
                            const int cost_l = static_cast<int>(std::round(engine::utils::statModify(static_cast<float>(pcb_l.mPlayer.mCost), 1, lhs->mRarity)));
                            const int cost_r = static_cast<int>(std::round(engine::utils::statModify(static_cast<float>(pcb_r.mPlayer.mCost), 1, rhs->mRarity)));
                            delta = (cost_l < cost_r) ? -1 : (cost_l > cost_r ? 1 : 0);
                            break;
                        }
                        case 6: {   // 生命值
                            const int hp_l = static_cast<int>(std::round(engine::utils::statModify(pcb_l.mStats.mHp, lhs->mLevel, lhs->mRarity)));
                            const int hp_r = static_cast<int>(std::round(engine::utils::statModify(pcb_r.mStats.mHp, rhs->mLevel, rhs->mRarity)));
                            delta = (hp_l < hp_r) ? -1 : (hp_l > hp_r ? 1 : 0);
                            break;
                        }
                        case 7: {   // 攻击力
                            const int atk_l = static_cast<int>(std::round(engine::utils::statModify(pcb_l.mStats.mAtk, lhs->mLevel, lhs->mRarity)));
                            const int atk_r = static_cast<int>(std::round(engine::utils::statModify(pcb_r.mStats.mAtk, rhs->mLevel, rhs->mRarity)));
                            delta = (atk_l < atk_r) ? -1 : (atk_l > atk_r ? 1 : 0);
                            break;
                        }
                        case 8: {   // 防御力
                            const int def_l = static_cast<int>(std::round(engine::utils::statModify(pcb_l.mStats.mDef, lhs->mLevel, lhs->mRarity)));
                            const int def_r = static_cast<int>(std::round(engine::utils::statModify(pcb_r.mStats.mDef, rhs->mLevel, rhs->mRarity)));
                            delta = (def_l < def_r) ? -1 : (def_l > def_r ? 1 : 0);
                            break;
                        }
                        case 9: {   // 攻击范围
                            const int range_l = static_cast<int>(std::round(pcb_l.mStats.mRange));
                            const int range_r = static_cast<int>(std::round(pcb_r.mStats.mRange));
                            delta = (range_l < range_r) ? -1 : (range_l > range_r ? 1 : 0);
                            break;
                        }
                        case 10: {  // 攻击间隔
                            const float ai_l = pcb_l.mStats.mAtkInterval;
                            const float ai_r = pcb_r.mStats.mAtkInterval;
                            delta = (ai_l < ai_r) ? -1 : (ai_l > ai_r ? 1 : 0);
                            break;
                        }
                        case 11: {  // 阻挡数量
                            delta = (pcb_l.mPlayer.mBlock < pcb_r.mPlayer.mBlock) ? -1 : (pcb_l.mPlayer.mBlock > pcb_r.mPlayer.mBlock ? 1 : 0);
                            break;
                        }
                        case 12: {  // 技能
                            const auto& sk_l = blueprint_manager->getSkillBlueprint(pcb_l.mPlayer.mSkillId);
                            const auto& sk_r = blueprint_manager->getSkillBlueprint(pcb_r.mPlayer.mSkillId);
                            delta = compareStrings(sk_l.mName, sk_r.mName);
                            break;
                        }
                        case 13: {  // 升级按钮 (和COST排序一致)
                            const int cost_l = static_cast<int>(std::round(engine::utils::statModify(static_cast<float>(pcb_l.mPlayer.mCost), 1, lhs->mRarity)));
                            const int cost_r = static_cast<int>(std::round(engine::utils::statModify(static_cast<float>(pcb_r.mPlayer.mCost), 1, rhs->mRarity)));
                            delta = (cost_l < cost_r) ? -1 : (cost_l > cost_r ? 1 : 0);
                            break;
                        }
                        default: break;
                    }

                    // 根据升序还是降序返回比较结果
                    return ascending ? (delta < 0) : (delta > 0);
                });

                // 完成排序后，将SpecsDirty设置为false，下轮更新会跳过排序操作（即脏标识模式）
                sort_specs->SpecsDirty = false;
            }
        }

        // 渲染数据行
        for (const auto& unit : unit_data_list) {
            // 获取并计算属性数据信息
            const auto& player_class_blueprint = blueprint_manager->getPlayerClassBlueprint(unit->mClassId);
            const auto& skill_blueprint = blueprint_manager->getSkillBlueprint(player_class_blueprint.mPlayer.mSkillId);
            const auto& stats = player_class_blueprint.mStats;
            const auto hp = engine::utils::statModify(stats.mHp, unit->mLevel, unit->mRarity);
            const auto atk = engine::utils::statModify(stats.mAtk, unit->mLevel, unit->mRarity);
            const auto def = engine::utils::statModify(stats.mDef, unit->mLevel, unit->mRarity);
            const auto cost = engine::utils::statModify(static_cast<float>(player_class_blueprint.mPlayer.mCost), 1, unit->mRarity);
            // 根据类型显示近战/远程/混合
            std::string type = player_class_blueprint.mPlayer.mType == game::defs::PlayerType::MELEE ? "近战" :
                player_class_blueprint.mPlayer.mType == game::defs::PlayerType::RANGED ? "远程" :
                player_class_blueprint.mPlayer.mType == game::defs::PlayerType::MIXED ? "混合" : "未知";

            // 获取头像信息
            const auto& portrait_image = ui_config->getPortrait(unit->mNameId);
            auto portrait_texture = mContext.getResourceManager().getTexture(portrait_image.getTextureId(), portrait_image.getTexturePath());
            auto portrait_rect = portrait_image.getSourceRect();  // 源矩形的区域
            auto sprite_sheet_size = mContext.getResourceManager().getTextureSize(portrait_image.getTextureId());   // 获取精灵图的尺寸

            // 计算头像的UV坐标（即源矩形左上、右下的坐标，相对于整张精灵图大小的比例，取值在0～1之间）
            float u = portrait_rect->mPosition.x / sprite_sheet_size.x;
            float v = portrait_rect->mPosition.y / sprite_sheet_size.y;
            float u2 = (portrait_rect->mPosition.x + portrait_rect->mSize.x) / sprite_sheet_size.x;
            float v2 = (portrait_rect->mPosition.y + portrait_rect->mSize.y) / sprite_sheet_size.y;

            // 设置显示尺寸
            const glm::vec2 DISPLAY_SIZE = glm::vec2(128.0f, 128.0f);

            // 新建一行
            ImGui::TableNextRow();
            // 每一行依次填充对应列的信息
            ImGui::TableNextColumn();       // 第一列：姓名
            ImGui::Text("%s", unit->mName.c_str());
            // 如果鼠标悬浮在该UI组件上，显示复杂信息（支持各种UI组件及其组合，例如下面的Tooltip组件）
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                // 图片显示参数：SDL_Texture*, 显示尺寸(像素), 源矩形的左上角UV坐标，源矩形的右下角UV坐标
                ImGui::Image((ImTextureID)(intptr_t)portrait_texture, ImVec2(DISPLAY_SIZE.x, DISPLAY_SIZE.y), ImVec2(u, v), ImVec2(u2, v2));
                ImGui::EndTooltip();
            }
            ImGui::TableNextColumn();       // 第二列：职业
            ImGui::Text("%s", player_class_blueprint.mDisplayInfo.mName.c_str());
            // 如果鼠标悬浮在该UI组件上，显示描述信息（只支持文本）
            ImGui::SetItemTooltip("%s", player_class_blueprint.mDisplayInfo.mDescription.c_str());
            ImGui::TableNextColumn();       // 第三列：类型
            ImGui::Text("%s", type.c_str());
            ImGui::TableNextColumn();       // 第四列：等级
            ImGui::Text("%d", unit->mLevel);
            ImGui::TableNextColumn();       // 第五列：稀有度
            ImGui::Text("%d", unit->mRarity);
            ImGui::TableNextColumn();       // 第六列：COST
            ImGui::Text("%d", static_cast<int>(std::round(cost)));
            ImGui::TableNextColumn();       // 第七列：生命值
            ImGui::Text("%d", static_cast<int>(std::round(hp)));
            ImGui::TableNextColumn();        // 第八列：攻击力
            ImGui::Text("%d", static_cast<int>(std::round(atk)));
            ImGui::TableNextColumn();        // 第九列：防御力
            ImGui::Text("%d", static_cast<int>(std::round(def)));
            ImGui::TableNextColumn();        // 第十列：攻击范围
            ImGui::Text("%d", static_cast<int>(std::round(stats.mRange)));
            ImGui::TableNextColumn();        // 第十一列：攻击间隔
            ImGui::Text("%.2f", stats.mAtkInterval);
            ImGui::TableNextColumn();        // 第十二列：阻挡数量
            ImGui::Text("%d", player_class_blueprint.mPlayer.mBlock);
            ImGui::TableNextColumn();        // 第十三列：技能
            ImGui::Text("%s", skill_blueprint.mName.c_str());
            ImGui::SetItemTooltip("%s", skill_blueprint.mDescription.c_str());
            ImGui::TableNextColumn();        // 第十四列：升级按钮

            // 使用 name 作为下一个UI组件(即Button)的 ID，确保唯一性，否则同名Button会冲突
            ImGui::PushID(unit->mName.c_str());
            // 根据积分点数，判断是否可以升级，并决定升级按钮是否可用
            bool can_upgrade = session_data->getPoint() >= static_cast<int>(std::round(cost));
            ImGui::BeginDisabled(!can_upgrade);
            std::string button_text = "- " + std::to_string(static_cast<int>(std::round(cost)));
            if (ImGui::Button(button_text.c_str())) {   // 如果没有PushID，默认会以Button中显示参数作为ID，那会出现重复ID
                session_data->addPoint(-static_cast<int>(std::round(cost)));
                unit->mLevel += 1;
            }
            ImGui::EndDisabled();
            ImGui::PopID();     // 与前面的PushID对应使用，用于结束ID范围
            ImGui::SetItemTooltip("升级耗费的点数：%d", static_cast<int>(std::round(cost)));
        }
        ImGui::EndTable();
    }

    // 事件回调函数
    void DebugUISystem::onUIPortraitHoverEnterEvent(const game::defs::UIPortraitHoverEnterEvent& event) {
        mHoveredPortrait = event.mNameId;
    }

    void DebugUISystem::onUIPortraitHoverLeaveEvent(const game::defs::UIPortraitHoverLeaveEvent&) {
        mHoveredPortrait = entt::null;
    }

}   // namespace game::system
