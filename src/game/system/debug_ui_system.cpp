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
#include "../../engine/component/name_component.h"
#include "../../engine/core/context.h"
#include "../../engine/core/game_state.h"
#include "../../engine/render/renderer.h"
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <cmath>
#include <spdlog/spdlog.h>
#include <entt/entity/registry.hpp>
#include <entt/core/hashed_string.hpp>

using namespace entt::literals;

namespace game::system {

    DebugUISystem::DebugUISystem(entt::registry& registry, engine::core::Context& context)
        : mRegistry(registry), mContext(context) {
    }

    void DebugUISystem::update() {
        beginFrame();
        renderHoveredUnit();
        renderSelectedUnit();
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
        // TODO: 技能相关按钮与信息
        ImGui::End();
    }

}   // namespace game::system
