#pragma once

#include "bindings/lunar/LunarModel.hpp"

#include <Geode/Result.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct lua_State;

namespace luax::lunar {

    struct RigNodeSpec {
        std::string id;
        std::optional<std::string> sprite;
        std::optional<std::string> parent;
        std::optional<float> x;
        std::optional<float> y;
        std::optional<float> rot;
        std::optional<float> sx;
        std::optional<float> sy;
        std::optional<float> z;
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

        [[nodiscard]] geode::Result<void> addMember(cocos2d::CCNode* node, std::optional<std::string> id);

        [[nodiscard]] geode::Result<cocos2d::CCNode*> addToParent(
            std::string_view parentId, cocos2d::CCNode* child, std::optional<std::string> id
        );

        [[nodiscard]] cocos2d::CCNode* getNode(std::string_view id) const;

        std::map<std::string, geode::WeakRef<cocos2d::CCNode>, std::less<>> const& nodes() const noexcept {
            return m_nodes;
        }

        [[nodiscard]] geode::Result<void> applySpec(RigSpec const& spec);

    protected:
        ~LunarRig() override = default;

        void removeChild(cocos2d::CCNode* child, bool cleanup) override;

    private:
        [[nodiscard]] geode::Result<void> registerId(std::string const& id, cocos2d::CCNode* node);
        void forgetNode(cocos2d::CCNode* node);

        std::map<std::string, geode::WeakRef<cocos2d::CCNode>, std::less<>> m_nodes;
    };

    geode::Result<void> registerLunarRig(lua_State* L);

} // namespace luax::lunar
