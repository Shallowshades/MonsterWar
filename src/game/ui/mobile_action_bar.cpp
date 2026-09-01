/*****************************************************************//**
 * @file   mobile_action_bar.cpp
 * @brief  移动端操作栏实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.09.01
 *********************************************************************/

#include "mobile_action_bar.h"
#include "../data/ui_config.h"
#include "../data/session_data.h"
#include "../data/game_stats.h"
#include "../factory/blueprint_manager.h"
#include "../component/player_component.h"
#include "../component/skill_component.h"
#include "../defs/tags.h"
#include "../defs/events.h"
#include "../../engine/core/context.h"
#include "../../engine/core/game_state.h"
#include "../../engine/input/input_manager.h"
#include "../../engine/ui/ui_panel.h"
#include "../../engine/ui/ui_button.h"
#include "../../engine/ui/ui_label.h"
#include "../../engine/ui/ui_manager.h"
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <entt/core/hashed_string.hpp>
#include <spdlog/spdlog.h>
#include <cmath>

using namespace entt::literals;

namespace game::ui {

    MobileActionBar::MobileActionBar(entt::registry& registry,
        engine::ui::UIManager& ui_manager,
        engine::core::Context& context)
        : mRegistry(registry), mUIManager(ui_manager), mContext(context) {
        createUI();
        spdlog::trace("MobileActionBar 构造完成。");
    }

    MobileActionBar::~MobileActionBar() = default;

    void MobileActionBar::update(float) {
        refreshVisibility();
    }

    void MobileActionBar::createUI() {
        if (!mUIManager.init(mContext.getGameState().getLogicalSize())) return;

        auto ui_config = mRegistry.ctx().get<std::shared_ptr<game::data::UIConfig>>();
        const auto window_size = mContext.getGameState().getLogicalSize();
        const auto frame_size = ui_config->getUnitPanelFrameSize();
        const float padding = 12.0f;
        const float button_size = 72.0f;

        // 操作栏放在肖像栏上方
        float panel_height = button_size + padding * 2.0f;
        float panel_width = 4.0f * (button_size + padding) + padding;
        glm::vec2 panel_pos{
            (window_size.x - panel_width) * 0.5f,
            window_size.y - frame_size.y - 2.0f * ui_config->getUnitPanelPadding() - panel_height - 8.0f
        };

        auto anchor = std::make_unique<engine::ui::UIPanel>(panel_pos, glm::vec2(panel_width, panel_height));
        anchor->setId("mobile_action_bar"_hs);
        anchor->setBackgroundColor(engine::utils::FColor(0.0f, 0.0f, 0.0f, 0.35f));

        // 使用通用肖像框作为按钮底图（移动端先保证可点，后续可换正式按钮资源）
        auto& btn_image = ui_config->getPortraitFrame(1);

        auto make_button = [&](std::string_view text, const glm::vec2& pos, std::function<void()> callback) {
            auto button = std::make_unique<engine::ui::UIButton>(mContext,
                btn_image, btn_image, btn_image,
                pos,
                glm::vec2(button_size, button_size),
                std::move(callback));
            button->setId(entt::hashed_string(text.data()));
            button->addChild(std::make_unique<engine::ui::UILabel>(mContext.getTextRenderer(),
                text,
                ui_config->getUnitPanelFontPath(),
                18,
                engine::utils::FColor::white(),
                glm::vec2(4.0f, button_size - 26.0f)));
            return button;
        };

        float x = padding;
        const float step = button_size + padding;

        // 升级
        anchor->addChild(make_button("升级", glm::vec2(x, padding), [this]() {
            auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
            if (entity == entt::null || !mRegistry.valid(entity)) return;
            if (auto player = mRegistry.try_get<game::component::PlayerComponent>(entity); player) {
                mContext.getDispatcher().enqueue(game::defs::UpgradeUnitEvent{ entity, player->mCost });
                mContext.getInputManager().consumeNextClick();
            }
        }));
        x += step;

        // 撤退
        anchor->addChild(make_button("撤退", glm::vec2(x, padding), [this]() {
            auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
            if (entity == entt::null || !mRegistry.valid(entity)) return;
            if (auto player = mRegistry.try_get<game::component::PlayerComponent>(entity); player) {
                auto return_cost = static_cast<int>(player->mCost * 0.5f);
                mContext.getDispatcher().enqueue(game::defs::RetreatEvent{ entity, return_cost });
                mContext.getInputManager().consumeNextClick();
            }
        }));
        x += step;

        // 技能
        anchor->addChild(make_button("技能", glm::vec2(x, padding), [this]() {
            auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
            if (entity == entt::null || !mRegistry.valid(entity)) return;
            if (mRegistry.all_of<game::defs::SkillReadyTag>(entity)) {
                mContext.getDispatcher().enqueue(game::defs::SkillActiveEvent{ entity });
                mContext.getInputManager().consumeNextClick();
            }
        }));
        x += step;

        // 取消
        anchor->addChild(make_button("取消", glm::vec2(x, padding), [this]() {
            auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
            if (entity != entt::null && mRegistry.valid(entity)) {
                mRegistry.remove<game::defs::ShowRangeTag>(entity);
            }
            mRegistry.ctx().get<entt::entity&>("selected_unit"_hs) = entt::null;
            mContext.getInputManager().consumeNextClick();
        }));

        mUIManager.addElement(std::move(anchor));
        mAnchorPanel = static_cast<engine::ui::UIPanel*>(mUIManager.getRootElement()->getChildById("mobile_action_bar"_hs));
        refreshVisibility();
    }

    void MobileActionBar::refreshVisibility() {
        if (!mAnchorPanel) return;
        auto& entity = mRegistry.ctx().get<entt::entity&>("selected_unit"_hs);
        bool visible = entity != entt::null && mRegistry.valid(entity) &&
            mRegistry.all_of<game::component::PlayerComponent>(entity);
        mAnchorPanel->setVisible(visible);
    }

}   // namespace game::ui
