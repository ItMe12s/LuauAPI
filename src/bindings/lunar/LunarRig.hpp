#pragma once

#include "bindings/lunar/LunarModel.hpp"

#include <Geode/Result.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct lua_State;

namespace luax::lunar {

    struct RigNodeSpec {
        std::string id;
        std::optional<std::string> sprite;
        std::optional<std::string> parent;
        float x = 0.F;
        float y = 0.F;
        float rot = 0.F;
        float sx = 1.F;
        float sy = 1.F;
        float z = 0.F;
        std::optional<float> opacity;
        std::optional<float> ax;
        std::optional<float> ay;
        std::optional<float> skx;
        std::optional<float> sky;
    };

    struct RigSpec {
        std::vector<RigNodeSpec> nodes;
    };

    class LunarRig final : public cocos2d::CCNode {
    public:
        static LunarRig* create();

        geode::Result<void> addMember(cocos2d::CCNode* node, std::optional<std::string> id);

        geode::Result<cocos2d::CCNode*> addToParent(
            std::string_view parentId, cocos2d::CCNode* child, std::optional<std::string> id
        );

        cocos2d::CCNode* getNode(std::string_view id) const;

        geode::Result<void> applySpec(RigSpec const& spec);

    protected:
        ~LunarRig() override = default;

        void removeChild(cocos2d::CCNode* child, bool cleanup) override;

    private:
        geode::Result<void> registerId(std::string const& id, cocos2d::CCNode* node);
        void forgetNode(cocos2d::CCNode* node);

        std::unordered_map<std::string, geode::WeakRef<cocos2d::CCNode>> m_nodes;
    };

    geode::Result<void> registerLunarRig(lua_State* L);

} // namespace luax::lunar
