#include "bindings/lunar/LunarRig.hpp"

#include "framework/stack/Stack.hpp"
#include "framework/stack/TableUtil.hpp"
#include "framework/usertype/Usertype.hpp"

#include <Geode/Geode.hpp>
#include <algorithm>
#include <fmt/format.h>
#include <lua.h>
#include <lualib.h>

namespace luax::lunar {
    namespace {

        geode::Result<RigSpec> parseRigSpec(lua_State* L, int idx, char const* method) {
            luaL_checktype(L, idx, LUA_TTABLE);
            RigSpec spec;

            lua_getfield(L, idx, "nodes");
            if (!lua_istable(L, -1)) {
                lua_pop(L, 1);
                return geode::Err(std::string("'nodes' array is required"));
            }
            int const nodesIdx = lua_gettop(L);
            auto const count = lua_objlen(L, nodesIdx);
            spec.nodes.reserve(count);
            for (lua_Integer i = 1; i <= static_cast<lua_Integer>(count); ++i) {
                lua_rawgeti(L, nodesIdx, i);
                if (!lua_istable(L, -1)) {
                    lua_pop(L, 1);
                    return geode::Err(fmt::format("nodes[{}] must be a table", i));
                }
                int const entry = lua_gettop(L);
                RigNodeSpec node;
                lua_getfield(L, entry, "id");
                if (!lua_isstring(L, -1)) {
                    lua_pop(L, 2);
                    return geode::Err(fmt::format("nodes[{}] requires a string 'id'", i));
                }
                size_t len = 0;
                char const* id = lua_tolstring(L, -1, &len);
                node.id.assign(id, len);
                lua_pop(L, 1);

                node.sprite = optStringField(L, entry, "sprite", method);
                node.parent = optStringField(L, entry, "parent", method);
                if (auto v = optNumberField(L, entry, "x", method)) node.x = static_cast<float>(*v);
                if (auto v = optNumberField(L, entry, "y", method)) node.y = static_cast<float>(*v);
                if (auto v = optNumberField(L, entry, "rot", method))
                    node.rot = static_cast<float>(*v);
                if (auto v = optNumberField(L, entry, "sx", method))
                    node.sx = static_cast<float>(*v);
                if (auto v = optNumberField(L, entry, "sy", method))
                    node.sy = static_cast<float>(*v);
                if (auto v = optNumberField(L, entry, "z", method)) node.z = static_cast<float>(*v);
                node.opacity = optNumberField(L, entry, "opacity", method).transform([](double v) {
                    return static_cast<float>(v);
                });

                lua_pop(L, 1);
                spec.nodes.push_back(std::move(node));
            }
            lua_pop(L, 1);
            return geode::Ok(std::move(spec));
        }

    } // namespace

    LunarRig* LunarRig::create() {
        auto* ret = new LunarRig();
        ret->autorelease();
        return ret;
    }

    void LunarRig::removeChild(cocos2d::CCNode* child, bool cleanup) {
        forgetNode(child);
        CCNode::removeChild(child, cleanup);
    }

    geode::Result<void> LunarRig::registerId(std::string const& id, cocos2d::CCNode* node) {
        if (id.empty()) return geode::Ok();
        if (m_nodes.contains(id)) {
            return geode::Err(fmt::format("duplicate rig node id '{}'", id));
        }
        m_nodes.emplace(id, node);
        return geode::Ok();
    }

    void LunarRig::forgetNode(cocos2d::CCNode* node) {
        std::erase_if(m_nodes, [node](auto const& entry) {
            return entry.second.lock() == node;
        });
    }

    geode::Result<void> LunarRig::addMember(cocos2d::CCNode* node, std::optional<std::string> id) {
        if (!node) return geode::Err(std::string("expected a node"));
        addChild(node);
        if (id) {
            auto reg = registerId(*id, node);
            if (reg.isErr()) {
                node->removeFromParent();
                return reg;
            }
        }
        return geode::Ok();
    }

    geode::Result<cocos2d::CCNode*> LunarRig::addToParent(
        std::string_view parentId, cocos2d::CCNode* child, std::optional<std::string> id
    ) {
        if (!child) return geode::Err(std::string("expected a node"));
        auto const it = m_nodes.find(std::string(parentId));
        if (it == m_nodes.end()) {
            return geode::Err(fmt::format("parent node '{}' not found in rig", parentId));
        }
        auto parent = it->second.lock();
        if (!parent) {
            return geode::Err(fmt::format("parent node '{}' not found in rig", parentId));
        }
        parent->addChild(child);
        if (id) {
            auto reg = registerId(*id, child);
            if (reg.isErr()) {
                child->removeFromParent();
                return geode::Err(reg.unwrapErr());
            }
        }
        return geode::Ok(parent.data());
    }

    cocos2d::CCNode* LunarRig::getNode(std::string_view id) const {
        auto const it = m_nodes.find(std::string(id));
        if (it == m_nodes.end()) return nullptr;
        return it->second.lock();
    }

    geode::Result<void> LunarRig::applySpec(RigSpec const& spec) {
        for (auto const& nodeSpec : spec.nodes) {
            if (nodeSpec.id.empty()) {
                return geode::Err(std::string("rig node id must not be empty"));
            }
            cocos2d::CCNode* parent = this;
            if (nodeSpec.parent) {
                parent = getNode(*nodeSpec.parent);
                if (!parent) {
                    return geode::Err(
                        fmt::format("rig node '{}' parent '{}' not found", nodeSpec.id, *nodeSpec.parent)
                    );
                }
            }

            cocos2d::CCNode* node = nullptr;
            if (nodeSpec.sprite) {
                std::string const resolved =
                    cocos2d::CCFileUtils::get()->fullPathForFilename(nodeSpec.sprite->c_str(), false);
                bool const fileExists = resolved != *nodeSpec.sprite;
                auto* sprite = fileExists ?
                    cocos2d::CCSprite::create(resolved.c_str()) :
                    cocos2d::CCSprite::createWithSpriteFrameName(nodeSpec.sprite->c_str());
                if (!sprite) {
                    geode::log::warn(
                        "[lunar] rig node '{}': failed to create sprite '{}', skipped",
                        nodeSpec.id,
                        *nodeSpec.sprite
                    );
                    continue;
                }
                if (!fileExists && sprite->isUsingFallback()) {
                    geode::log::warn(
                        "[lunar] rig node '{}': sprite '{}' not found, mod sprites must be "
                        "prefixed '<mod-id>/name.png'",
                        nodeSpec.id,
                        *nodeSpec.sprite
                    );
                }
                node = sprite;
            }
            else {
                node = cocos2d::CCNode::create();
            }

            node->setPosition(nodeSpec.x, nodeSpec.y);
            node->setRotation(nodeSpec.rot);
            node->setScaleX(nodeSpec.sx);
            node->setScaleY(nodeSpec.sy);
            node->setZOrder(static_cast<int>(nodeSpec.z));
            if (nodeSpec.opacity) {
                if (auto* rgba = geode::cast::typeinfo_cast<cocos2d::CCRGBAProtocol*>(node)) {
                    rgba->setOpacity(static_cast<GLubyte>(std::clamp(*nodeSpec.opacity, 0.F, 255.F)));
                }
                else {
                    geode::log::warn(
                        "[lunar] rig node '{}': node type does not support opacity, ignored",
                        nodeSpec.id
                    );
                }
            }
            parent->addChild(node);
            auto reg = registerId(nodeSpec.id, node);
            if (reg.isErr()) return reg;
        }
        return geode::Ok();
    }

} // namespace luax::lunar

namespace luax::lunar {
    namespace {

        int rigNew(lua_State* L) {
            Usertype<LunarRig>::pushOwned(L, LunarRig::create());
            return 1;
        }

        int rigAdd(lua_State* L) {
            auto* self = Usertype<LunarRig>::check(L, 1, "LunarRig:add");
            auto* node = Usertype<cocos2d::CCNode>::tryCheck(L, 2);
            if (!node) {
                luaL_error(L, "LunarRig:add expected a CCNode at arg 2");
            }
            std::optional<std::string> id;
            if (lua_gettop(L) >= 3 && !lua_isnil(L, 3)) {
                id = check<std::string>(L, 3, "LunarRig:add");
            }
            auto result = self->addMember(node, std::move(id));
            if (result.isErr()) {
                luaL_error(L, "LunarRig:add: %s", result.unwrapErr().c_str());
            }
            return 0;
        }

        int rigAddTo(lua_State* L) {
            auto* self = Usertype<LunarRig>::check(L, 1, "LunarRig:addTo");
            auto const parentId = check<std::string>(L, 2, "LunarRig:addTo");
            auto* child = Usertype<cocos2d::CCNode>::tryCheck(L, 3);
            if (!child) {
                luaL_error(L, "LunarRig:addTo expected a CCNode at arg 3");
            }
            std::optional<std::string> id;
            if (lua_gettop(L) >= 4 && !lua_isnil(L, 4)) {
                id = check<std::string>(L, 4, "LunarRig:addTo");
            }
            auto result = self->addToParent(parentId, child, std::move(id));
            if (result.isErr()) {
                luaL_error(L, "LunarRig:addTo: %s", result.unwrapErr().c_str());
            }
            return 0;
        }

        int rigGetNode(lua_State* L) {
            auto* self = Usertype<LunarRig>::check(L, 1, "LunarRig:getNode");
            auto const id = check<std::string>(L, 2, "LunarRig:getNode");
            if (auto* node = self->getNode(id)) {
                Usertype<cocos2d::CCNode>::pushBorrowed(L, node);
            }
            else {
                lua_pushnil(L);
            }
            return 1;
        }

        int rigLoad(lua_State* L) {
            auto* self = Usertype<LunarRig>::check(L, 1, "LunarRig:load");
            auto parsed = parseRigSpec(L, 2, "LunarRig:load");
            if (parsed.isErr()) {
                luaL_error(L, "LunarRig:load: %s", parsed.unwrapErr().c_str());
            }
            auto result = self->applySpec(std::move(parsed).unwrap());
            if (result.isErr()) {
                luaL_error(L, "LunarRig:load: %s", result.unwrapErr().c_str());
            }
            lua_pushvalue(L, 1);
            return 1;
        }

    } // namespace

    geode::Result<void> registerLunarRig(lua_State* L) {
        auto const ccNodeTag = Usertype<cocos2d::CCNode>::tag();
        auto registerResult = Usertype<LunarRig>::registerType(L, "LunarRig", {ccNodeTag});
        if (registerResult.isErr()) return registerResult;

        Usertype<LunarRig>::method(L, "add", &rigAdd);
        Usertype<LunarRig>::method(L, "addTo", &rigAddTo);
        Usertype<LunarRig>::method(L, "getNode", &rigGetNode);
        Usertype<LunarRig>::method(L, "load", &rigLoad);

        getOrCreateTable(L, "lunar.rig");
        setTableCFunction(L, -1, "new", &rigNew);
        lua_pop(L, 1);

        return geode::Ok();
    }

} // namespace luax::lunar
