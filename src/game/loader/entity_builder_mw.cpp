/*****************************************************************//**
 * @file   entity_builder_mw.cpp
 * @brief  MonsterWar 关卡实体生成器实现
 * @version 1.0
 *
 * @author Shallowshades
 * @date   2026.07.20
 *********************************************************************/

#include "entity_builder_mw.h"
#include "../../engine/core/context.h"
#include "../../engine/component/tilelayer_component.h"
#include "../defs/tags.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace game::loader
{

    EntityBuilderMW::EntityBuilderMW(engine::loader::LevelLoader& level_loader,
        engine::core::Context& context,
        entt::registry& registry,
        std::unordered_map<int, game::data::WaypointNode>& waypoint_nodes,
        std::vector<int>& start_points)
        : engine::loader::BasicEntityBuilder(level_loader, context, registry), mWaypointNodes(waypoint_nodes), mStartPoints(start_points)
    {
    }

    EntityBuilderMW* EntityBuilderMW::build() {
        if (mObjectJson && !mTileInfo) {  // 代表自己绘制的形状,当前游戏只用到了路径节点
            buildPath();
        }
        else {
            BasicEntityBuilder::build();
            buildPlace();               // 如果识别到地点类型就添加
        }

        return this;
    }

    void EntityBuilderMW::buildPath() {
        // 检查数据有效性
        if (mObjectJson->value("point", false) != true) return;
        if (!mObjectJson->contains("properties") || !mObjectJson->at("properties").is_array()) return;
        auto id = mObjectJson->value("id", 0);
        if (id == 0) return;

        // 解析数据并添加到容器
        auto position = glm::vec2(mObjectJson->value("x", 0.0f), mObjectJson->value("y", 0.0f));
        std::vector<int> next_node_ids;
        for (auto& property : mObjectJson->at("properties")) {
            // 如果是对象类型，且名称以 next 开头，则添加到 next_node_ids
            if (property.value("type", "") == "object" && property.value("name", "").starts_with("next")) {
                auto next_node_id = property.value("value", 0);
                if (next_node_id != 0) {
                    next_node_ids.push_back(next_node_id);
                }
            }
            // 如果名称是 start，且值为真，则将自身id添加到 mStartPoints 中
            if (property.value("name", "") == "start" && property.value("value", false) == true) {
                mStartPoints.push_back(id);
            }
        }
        // 添加到节点容器中
        mWaypointNodes[id] = game::data::WaypointNode(id, std::move(position), std::move(next_node_ids));
        spdlog::trace("mWaypointNodes size: {}", mWaypointNodes.size());
    }

    void EntityBuilderMW::buildPlace() {
        // 读取瓦片的自定义属性，如果标记了 place 类型，则给实体添加对应的放置区域标签
        if (mTileInfo && mTileInfo->mProperties) {
            auto& properties = mTileInfo->mProperties.value();
            for (auto& property : properties) {
                if (property.value("name", "") == "place") {
                    auto type = property.value("value", "");
                    if (type == "melee") {
                        mRegistry.emplace<game::defs::MeleePlaceTag>(mEntityId);
                        spdlog::trace("添加近战放置区域标签, 实体: {}", entt::to_integral(mEntityId));
                    }
                    else if (type == "range") {
                        mRegistry.emplace<game::defs::RangedPlaceTag>(mEntityId);
                        spdlog::trace("添加远程放置区域标签, 实体: {}", entt::to_integral(mEntityId));
                    }
                    // TODO: 未来如果有其他类型可以继续添加
                }
            }
        }
    }

}   // namespace game::loader
