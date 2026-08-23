#include "bindings/lunar/LunarRig.hpp"

#include "bindings/geode/CurrentMod.hpp"
#include "bindings/lunar/LunarAnimation.hpp"
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
            LuaStackGuard const guard(L);
            RigSpec spec;

            lua_getfield(L, idx, "nodes");
            if (!lua_istable(L, -1)) {
                return geode::Err(std::string("'nodes' array is required"));
            }
            int const nodesIdx = lua_gettop(L);
            auto const count = lua_objlen(L, nodesIdx);
            spec.nodes.reserve(count);

            for (lua_Integer i = 1; i <= static_cast<lua_Integer>(count); ++i) {
                lua_rawgeti(L, nodesIdx, i);
                if (!lua_istable(L, -1)) {
                    return geode::Err(fmt::format("nodes[{}] must be a table", i));
                }
                int const entry = lua_gettop(L);
                RigNodeSpec node;
                lua_getfield(L, entry, "id");
                if (!lua_isstring(L, -1)) {
                    return geode::Err(fmt::format("nodes[{}] requires a string 'id'", i));
                }
                size_t len = 0;
                char const* id = lua_tolstring(L, -1, &len);
                node.id.assign(id, len);
                lua_pop(L, 1);

                node.sprite = optStringField(L, entry, "sprite", method);
                node.parent = optStringField(L, entry, "parent", method);

                auto num = [&](char const* key) -> geode::Result<std::optional<float>> {
                    if (auto v = optNumberField(L, entry, key, method)) {
                        if (!std::isfinite(*v)) {
                            return geode::Err(fmt::format("field '{}' must be finite", key));
                        }
                        return geode::Ok(static_cast<float>(*v));
                    }
                    return geode::Ok(std::nullopt);
                };
                if (auto v = num("x"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.x = v.unwrap().value_or(node.x);
                if (auto v = num("y"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.y = v.unwrap().value_or(node.y);
                if (auto v = num("rot"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.rot = v.unwrap().value_or(node.rot);
                if (auto v = num("sx"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.sx = v.unwrap().value_or(node.sx);
                if (auto v = num("sy"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.sy = v.unwrap().value_or(node.sy);
                if (auto v = num("z"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.z = v.unwrap().value_or(node.z);
                if (auto v = num("opacity"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.opacity = v.unwrap();
                if (auto v = num("ax"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.ax = v.unwrap();
                if (auto v = num("ay"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.ay = v.unwrap();
                if (auto v = num("skx"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.skx = v.unwrap();
                if (auto v = num("sky"); v.isErr()) return geode::Err(v.unwrapErr());
                else node.sky = v.unwrap();

                spec.nodes.push_back(std::move(node));
            }
            return geode::Ok(std::move(spec));
        }

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

        int rigListNodes(lua_State* L) {
            auto* self = Usertype<LunarRig>::check(L, 1, "LunarRig:listNodes");
            std::vector<std::string> ids;
            for (auto const& [id, ref] : self->nodes()) {
                if (ref.lock()) ids.push_back(id);
            }
            std::sort(ids.begin(), ids.end());
            lua_createtable(L, static_cast<int>(ids.size()), 0);
            for (std::size_t i = 0; i < ids.size(); ++i) {
                push(L, ids[i]);
                lua_rawseti(L, -2, static_cast<int>(i) + 1);
            }
            return 1;
        }

        int rigGetPose(lua_State* L) {
            auto* self = Usertype<LunarRig>::check(L, 1, "LunarRig:getPose");
            auto const id = check<std::string>(L, 2, "LunarRig:getPose");
            auto* node = self->getNode(id);
            if (!node) {
                lua_pushnil(L);
                return 1;
            }
            lua_createtable(L, 0, 11);
            auto set = [&](char const* key, float value) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
                lua_setfield(L, -2, key);
            };
            set("x", node->getPositionX());
            set("y", node->getPositionY());
            set("rot", node->getRotation());
            set("sx", node->getScaleX());
            set("sy", node->getScaleY());
            if (auto* rgba = geode::cast::typeinfo_cast<cocos2d::CCRGBAProtocol*>(node)) {
                set("opacity", static_cast<float>(rgba->getOpacity()));
            }
            set("z", static_cast<float>(node->getZOrder()));
            auto const anchor = node->getAnchorPoint();
            set("ax", anchor.x);
            set("ay", anchor.y);
            set("skx", node->getSkewX());
            set("sky", node->getSkewY());
            return 1;
        }

        int rigLoad(lua_State* L) {
            auto* self = Usertype<LunarRig>::check(L, 1, "LunarRig:load");
            auto parsed = parseRigSpec(L, 2, "LunarRig:load");
            if (auto err = returnIfErr(L, parsed)) return *err;
            auto result = self->applySpec(std::move(parsed).unwrap());
            if (auto err = returnIfErr(L, result)) return *err;
            lua_pushvalue(L, 1);
            return 1;
        }

        int rigLoadAnimation(lua_State* L) {
            auto* rig = Usertype<LunarRig>::check(L, 1, "LunarRig:loadAnimation");
            LunarAnimationDef* def = Usertype<LunarAnimationDef>::tryCheck(L, 2);
            if (!def) {
                if (lua_istable(L, 2)) {
                    auto parsed = parseAnimTable(L, 2, "LunarRig:loadAnimation");
                    if (auto err = returnIfErr(L, parsed)) return *err;
                    def = std::move(parsed).unwrap();
                }
                else {
                    luaL_error(L, "LunarRig:loadAnimation expected an animation or table at arg 2");
                }
            }
            auto compiled = compileAnimation(def->keyframes(), def->fps(), def->looped());
            if (auto err = returnIfErr(L, compiled)) return *err;
            Usertype<LunarTrack>::pushOwned(L, LunarTrack::create(rig, std::move(compiled).unwrap()));
            return 1;
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
        std::vector<cocos2d::CCNode*> created;
        std::vector<std::string> registeredIds;
        auto fail = [&](std::string&& msg) -> geode::Result<void> {
            for (auto* node : created)
                node->removeFromParent();
            for (auto const& id : registeredIds)
                m_nodes.erase(id);
            return geode::Err(std::move(msg));
        };
        for (auto const& nodeSpec : spec.nodes) {
            if (nodeSpec.id.empty()) {
                return fail("rig node id must not be empty");
            }
            cocos2d::CCNode* parent = this;
            if (nodeSpec.parent) {
                parent = getNode(*nodeSpec.parent);
                if (!parent) {
                    return fail(
                        fmt::format("rig node '{}' parent '{}' not found", nodeSpec.id, *nodeSpec.parent)
                    );
                }
            }

            cocos2d::CCNode* node = nullptr;
            if (nodeSpec.sprite) {
                auto* fileUtils = cocos2d::CCFileUtils::get();
                std::string const resolved =
                    fileUtils->fullPathForFilename(nodeSpec.sprite->c_str(), false);
                bool const fileExists = fileUtils->isFileExist(resolved);
                auto* sprite = fileExists ?
                    cocos2d::CCSprite::create(resolved.c_str()) :
                    cocos2d::CCSprite::createWithSpriteFrameName(nodeSpec.sprite->c_str());
                if (!sprite) {
                    geode::log::warn(
                        "rig node '{}': failed to create sprite '{}', skipped",
                        nodeSpec.id,
                        *nodeSpec.sprite
                    );
                    continue;
                }
                if (!fileExists && sprite->isUsingFallback()) {
                    geode::log::warn(
                        "rig node '{}': sprite '{}' not found, mod sprites must be "
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
            if (nodeSpec.ax || nodeSpec.ay) {
                auto const cur = node->getAnchorPoint();
                node->setAnchorPoint({nodeSpec.ax.value_or(cur.x), nodeSpec.ay.value_or(cur.y)});
            }
            if (nodeSpec.skx) node->setSkewX(*nodeSpec.skx);
            if (nodeSpec.sky) node->setSkewY(*nodeSpec.sky);
            if (nodeSpec.opacity && !setNodeOpacity(node, *nodeSpec.opacity)) {
                geode::log::warn(
                    "rig node '{}': node type does not support opacity, ignored", nodeSpec.id
                );
            }
            if (auto* mod = currentMod()) {
                node->setID(fmt::format("{}/{}", mod->getID(), nodeSpec.id));
            }
            parent->addChild(node);
            created.push_back(node);
            auto reg = registerId(nodeSpec.id, node);
            if (reg.isErr()) {
                return fail(std::move(reg).unwrapErr());
            }
            registeredIds.push_back(nodeSpec.id);
        }
        return geode::Ok();
    }

    geode::Result<void> registerLunarRig(lua_State* L) {
        auto const ccNodeTag = Usertype<cocos2d::CCNode>::tag();
        auto registerResult = Usertype<LunarRig>::registerType(L, "LunarRig", {ccNodeTag});
        if (registerResult.isErr()) return registerResult;

        Usertype<LunarRig>::method(L, "add", &rigAdd);
        Usertype<LunarRig>::method(L, "addTo", &rigAddTo);
        Usertype<LunarRig>::method(L, "getNode", &rigGetNode);
        Usertype<LunarRig>::method(L, "listNodes", &rigListNodes);
        Usertype<LunarRig>::method(L, "getPose", &rigGetPose);
        Usertype<LunarRig>::method(L, "load", &rigLoad);
        Usertype<LunarRig>::method(L, "loadAnimation", &rigLoadAnimation);

        getOrCreateTable(L, "lunar.rig");
        setTableCFunction(L, -1, "new", &rigNew);
        lua_pop(L, 1);

        return geode::Ok();
    }

} // namespace luax::lunar
