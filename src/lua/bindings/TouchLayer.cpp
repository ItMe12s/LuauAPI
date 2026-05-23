#include "../Binding.hpp"
#include "../Runtime.hpp"
#include "internal/LuaRef.hpp"
#include "internal/Ref.hpp"
#include "internal/Stack.hpp"
#include "internal/TableUtil.hpp"
#include "internal/Usertype.hpp"
#include "internal/Validate.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/cocos.hpp>
#include <cocos2d.h>
#include <lua.h>
#include <lualib.h>

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
    using namespace geode::prelude;
    using namespace luax;

    class LuaTouchLayer : public cocos2d::CCLayer {
    public:
        struct Target {
            int id = 0;
            geode::Ref<cocos2d::CCSprite> sprite = nullptr;
            LuaRef callback;
        };

        struct Claim {
            int targetId = 0;
        };

        static LuaTouchLayer* create() {
            auto ret = new LuaTouchLayer();
            if (ret && ret->init()) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }

        bool init() override {
            if (!CCLayer::init()) return false;
            setTouchEnabled(true);
            setTouchMode(cocos2d::kCCTouchesOneByOne);
            setTouchPriority(resolveDefaultTouchPriority());
            return true;
        }

        ~LuaTouchLayer() override {
            clearTapTargets();
        }

        void onExit() override {
            m_claimedTargets.clear();
            m_claimedTouchPtrs.clear();
            CCLayer::onExit();
        }

        void registerWithTouchDispatcher() override {
            if (!m_hasManualTouchPriority) {
                setTouchPriority(resolveDefaultTouchPriority());
            }
            cocos2d::CCTouchDispatcher::get()->addTargetedDelegate(this, getTouchPriority(), true);
        }

        bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent*) override {
            auto touchId = touch->getID();
            m_claimedTargets.erase(touchId);

            auto found = findHitTarget(touch);
            if (!found) return false;

            Claim claim = { found->id };
            m_claimedTargets[touchId] = claim;
            m_claimedTouchPtrs[touch] = claim;
            callTarget(*found, touch);
            return true;
        }

        void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent*) override {
            auto touchId = touch->getID();
            clearClaim(touchId, touch);
        }

        void ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent*) override {
            m_claimedTargets.erase(touch->getID());
            m_claimedTouchPtrs.erase(touch);
        }

        int addTapTarget(cocos2d::CCSprite* sprite, LuaRef callback) {
            auto id = m_nextTargetId++;
            m_targets.push_back(std::make_unique<Target>(Target{ id, sprite, std::move(callback) }));
            return id;
        }

        bool removeTapTarget(int id) {
            auto it = std::find_if(m_targets.begin(), m_targets.end(), [id](auto const& target) {
                return target->id == id;
            });
            if (it == m_targets.end()) return false;

            m_targets.erase(it);
            eraseClaimedTarget(id);
            return true;
        }

        void clearTapTargets() {
            m_targets.clear();
            m_claimedTargets.clear();
            m_claimedTouchPtrs.clear();
        }

        void setManualTouchPriority(int priority) {
            m_hasManualTouchPriority = true;
            setTouchPriority(priority);
            if (cocos2d::CCTouchDispatcher::get()->findHandler(this)) {
                cocos2d::CCTouchDispatcher::get()->setPriority(priority, this);
            }
        }

    private:
        static int resolveDefaultTouchPriority() {
            auto dispatcher = cocos2d::CCTouchDispatcher::get();
            if (!dispatcher) return -501;

            return std::min(dispatcher->getTargetPrio() - 1, -501);
        }

        static bool nodeVisible(cocos2d::CCNode* node) {
            for (auto current = node; current; current = current->getParent()) {
                if (!current->isVisible()) return false;
            }
            return true;
        }

        static bool nodeContentContainsTouch(cocos2d::CCNode* node, cocos2d::CCPoint const& world) {
            auto size = node->getContentSize();
            if (size.width <= 0.f || size.height <= 0.f) return false;

            auto local = node->convertToNodeSpace(world);
            return cocos2d::CCRect(0.f, 0.f, size.width, size.height).containsPoint(local);
        }

        static bool nodeTreeContainsTouch(cocos2d::CCNode* node, cocos2d::CCPoint const& world) {
            if (!node || !node->isVisible()) return false;
            if (nodeContentContainsTouch(node, world)) return true;

            auto children = node->getChildren();
            if (!children) return false;

            for (int i = static_cast<int>(children->count()) - 1; i >= 0; --i) {
                auto* child = geode::cast::typeinfo_cast<cocos2d::CCNode*>(children->objectAtIndex(static_cast<unsigned int>(i)));
                if (nodeTreeContainsTouch(child, world)) return true;
            }
            return false;
        }

        static bool targetContainsTouch(Target const& target, cocos2d::CCTouch* touch) {
            auto* sprite = target.sprite.data();
            if (!sprite || !sprite->getParent() || !nodeVisible(sprite)) return false;

            return nodeTreeContainsTouch(sprite, touch->getLocation());
        }

        Target* findHitTarget(cocos2d::CCTouch* touch) {
            for (auto it = m_targets.rbegin(); it != m_targets.rend(); ++it) {
                if (targetContainsTouch(**it, touch)) return it->get();
            }
            return nullptr;
        }

        void callTarget(Target const& target, cocos2d::CCTouch* touch) {
            auto& runtime = luax::Runtime::instance();
            if (!runtime.ready()) return;

            auto* L = runtime.state();
            if (!target.callback.push()) return;
            if (!lua_isfunction(L, -1)) {
                lua_pop(L, 1);
                return;
            }

            auto* sprite = target.sprite.data();
            auto world = touch->getLocation();
            auto local = sprite ? sprite->convertToNodeSpace(world) : cocos2d::CCPointZero;

            lua_createtable(L, 0, 5);
            lua_pushnumber(L, world.x);    lua_setfield(L, -2, "x");
            lua_pushnumber(L, world.y);    lua_setfield(L, -2, "y");
            lua_pushnumber(L, local.x);    lua_setfield(L, -2, "localX");
            lua_pushnumber(L, local.y);    lua_setfield(L, -2, "localY");
            lua_pushinteger(L, target.id); lua_setfield(L, -2, "targetId");

            runtime.protectedCall(1, 0, "TouchLayer tap callback", 50);
        }

        void eraseClaimedTarget(int id) {
            for (auto it = m_claimedTargets.begin(); it != m_claimedTargets.end();) {
                if (it->second.targetId == id) {
                    it = m_claimedTargets.erase(it);
                } else {
                    ++it;
                }
            }

            for (auto it = m_claimedTouchPtrs.begin(); it != m_claimedTouchPtrs.end();) {
                if (it->second.targetId == id) {
                    it = m_claimedTouchPtrs.erase(it);
                } else {
                    ++it;
                }
            }
        }

        void clearClaim(int touchId, cocos2d::CCTouch* touch) {
            auto byId = m_claimedTargets.find(touchId);
            if (byId != m_claimedTargets.end()) {
                m_claimedTargets.erase(byId);
                m_claimedTouchPtrs.erase(touch);
                return;
            }

            auto byPtr = m_claimedTouchPtrs.find(touch);
            if (byPtr == m_claimedTouchPtrs.end()) return;

            auto claim = byPtr->second;
            m_claimedTouchPtrs.erase(byPtr);
            eraseClaimedTouchId(claim.targetId);
        }

        void eraseClaimedTouchId(int targetId) {
            for (auto it = m_claimedTargets.begin(); it != m_claimedTargets.end();) {
                if (it->second.targetId == targetId) {
                    it = m_claimedTargets.erase(it);
                } else {
                    ++it;
                }
            }
        }

        bool m_hasManualTouchPriority = false;
        int m_nextTargetId = 1;
        std::unordered_map<int, Claim> m_claimedTargets;
        std::unordered_map<cocos2d::CCTouch*, Claim> m_claimedTouchPtrs;
        std::vector<std::unique_ptr<Target>> m_targets;
    };

    int touchlayer_create(lua_State* L) {
        assertMainThread();
        Usertype<LuaTouchLayer>::pushOwned(L, LuaTouchLayer::create());
        return 1;
    }

    int touchlayer_addTapTarget(lua_State* L) {
        auto self = Usertype<LuaTouchLayer>::check(L, 1, "TouchLayer:addTapTarget");
        auto sprite = Usertype<cocos2d::CCSprite>::check(L, 2, "TouchLayer:addTapTarget");
        assertMainThread();
        requireSprite(sprite, "TouchLayer:addTapTarget");

        if (!lua_isfunction(L, 3)) {
            luaL_error(L, "TouchLayer:addTapTarget expected function at arg 3");
        }

        auto targetId = self->addTapTarget(sprite, LuaRef(L, 3));
        push(L, targetId);
        return 1;
    }

    int touchlayer_removeTapTarget(lua_State* L) {
        auto self = Usertype<LuaTouchLayer>::check(L, 1, "TouchLayer:removeTapTarget");
        assertMainThread();
        push(L, self->removeTapTarget(check<int>(L, 2, "TouchLayer:removeTapTarget")));
        return 1;
    }

    int touchlayer_clearTapTargets(lua_State* L) {
        auto self = Usertype<LuaTouchLayer>::check(L, 1, "TouchLayer:clearTapTargets");
        assertMainThread();
        self->clearTapTargets();
        return 0;
    }

    int touchlayer_setTouchPriority(lua_State* L) {
        auto self = Usertype<LuaTouchLayer>::check(L, 1, "TouchLayer:setTouchPriority");
        assertMainThread();
        self->setManualTouchPriority(check<int>(L, 2, "TouchLayer:setTouchPriority"));
        return 0;
    }

    void bindTouchLayer(lua_State* L) {
        Usertype<LuaTouchLayer>::registerType(L, "TouchLayer", { Usertype<cocos2d::CCLayer>::tag() });
        Usertype<LuaTouchLayer>::method(L, "addTapTarget",     &touchlayer_addTapTarget);
        Usertype<LuaTouchLayer>::method(L, "removeTapTarget",  &touchlayer_removeTapTarget);
        Usertype<LuaTouchLayer>::method(L, "clearTapTargets",  &touchlayer_clearTapTargets);
        Usertype<LuaTouchLayer>::method(L, "setTouchPriority", &touchlayer_setTouchPriority);

        getOrCreateTable(L, "geode");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, &touchlayer_create, "TouchLayer.create");
        lua_setfield(L, -2, "create");
        lua_setfield(L, -2, "TouchLayer");
        lua_pop(L, 1);
    }

    LUAX_BINDING_PRIORITY(TouchLayer, bindTouchLayer, 20)
}
