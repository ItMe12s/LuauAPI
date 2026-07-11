#if !defined(LUAUAPI_HOST_TESTS)
    #include "bindings/geode/GeodeBindingSupport.hpp"
    #include "core/Config.hpp"
    #include "framework/Binding.hpp"
    #include "framework/callback/LuaCallback.hpp"
    #include "framework/stack/Stack.hpp"
    #include "framework/stack/TableUtil.hpp"
    #include "framework/stack/TaggedMetatable.hpp"
    #include "framework/stack/Types.hpp"
    #include "framework/stack/UserdataTags.hpp"
    #include "framework/usertype/Usertype.hpp"

    #include <Geode/ui/PopupManager.hpp>
    #include <limits>
    #include <memory>
    #include <optional>
    #include <string>
    #include <utility>

namespace {
    using namespace luax;

    constexpr char const* kManagedPopupMeta = "luax.ManagedPopup";

    struct ManagedPopupBox {
        geode::ManagedPopup popup;
    };

    ManagedPopupBox* checkPopup(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<ManagedPopupBox*>(luaL_checkudata(L, idx, kManagedPopupMeta));
        if (!box) luaL_error(L, "%s expected ManagedPopup at arg %d", method, idx);
        return box;
    }

    void registerManagedPopupMetatable(lua_State* L);

    void pushManagedPopup(lua_State* L, geode::ManagedPopup popup) {
        registerManagedPopupMetatable(L);
        geode_detail::pushOwned(L, detail::managedPopupTag(), ManagedPopupBox{std::move(popup)});
    }

    int popupGetInner(lua_State* L) {
        auto* inner = checkPopup(L, 1, "ManagedPopup:getInner")->popup.getInner();
        if (!inner) {
            lua_pushnil(L);
            return 1;
        }
        Usertype<FLAlertLayer>::pushBorrowed(L, inner);
        return 1;
    }

    bool optBool(lua_State* L, int idx, bool fallback, char const* method) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return fallback;
        return check<bool>(L, idx, method);
    }

    int popupSetPersistent(lua_State* L) {
        checkPopup(L, 1, "ManagedPopup:setPersistent")
            ->popup.setPersistent(optBool(L, 2, true, "ManagedPopup:setPersistent"));
        return 0;
    }

    int popupSetPriority(lua_State* L) {
        checkPopup(L, 1, "ManagedPopup:setPriority")
            ->popup.setPriority(optBool(L, 2, true, "ManagedPopup:setPriority"));
        return 0;
    }

    int popupBlockClosingFor(lua_State* L) {
        double secondsValue = geode_detail::checkFinite(L, 2, "ManagedPopup:blockClosingFor");
        if (secondsValue < 0.0 || secondsValue > std::numeric_limits<float>::max()) {
            luaL_error(L, "ManagedPopup:blockClosingFor expected non-negative seconds");
        }
        float seconds = static_cast<float>(secondsValue);
        checkPopup(L, 1, "ManagedPopup:blockClosingFor")->popup.blockClosingFor(seconds);
        return 0;
    }

    int popupShowInstant(lua_State* L) {
        checkPopup(L, 1, "ManagedPopup:showInstant")->popup.showInstant();
        return 0;
    }

    int popupShowQueue(lua_State* L) {
        checkPopup(L, 1, "ManagedPopup:showQueue")->popup.showQueue();
        return 0;
    }

    int popupIsShown(lua_State* L) {
        lua_pushboolean(L, checkPopup(L, 1, "ManagedPopup:isShown")->popup.isShown());
        return 1;
    }

    int popupShouldPreventClosing(lua_State* L) {
        lua_pushboolean(
            L, checkPopup(L, 1, "ManagedPopup:shouldPreventClosing")->popup.shouldPreventClosing()
        );
        return 1;
    }

    void registerManagedPopupMetatable(lua_State* L) {
        constexpr luaL_Reg methods[] = {
            {"getInner", &popupGetInner},
            {"setPersistent", &popupSetPersistent},
            {"setPriority", &popupSetPriority},
            {"blockClosingFor", &popupBlockClosingFor},
            {"showInstant", &popupShowInstant},
            {"showQueue", &popupShowQueue},
            {"isShown", &popupIsShown},
            {"shouldPreventClosing", &popupShouldPreventClosing},
            {nullptr, nullptr},
        };
        registerTaggedMetatable(
            L,
            kManagedPopupMeta,
            detail::managedPopupTag(),
            methods,
            std::nullopt,
            &geode_detail::destroyOwned<ManagedPopupBox>,
            "ManagedPopup"
        );
    }

    std::string optString(lua_State* L, int idx, char const* fallback, char const* method) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return fallback ? fallback : "";
        return check<std::string>(L, idx, method);
    }

    std::optional<std::string> optNullableString(lua_State* L, int idx, char const* method) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return std::nullopt;
        return check<std::string>(L, idx, method);
    }

    float optWidth(lua_State* L, int idx, char const* method) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return geode::PopupManager::DEFAULT_WIDTH;
        double width = geode_detail::checkFinite(L, idx, method);
        if (width <= 0.0 || width > std::numeric_limits<float>::max()) {
            luaL_error(L, "%s expected a positive finite float width", method);
        }
        return static_cast<float>(width);
    }

    int managerAlert(lua_State* L) {
        auto title = check<std::string>(L, 1, "geode.PopupManager.alert");
        auto content = check<std::string>(L, 2, "geode.PopupManager.alert");
        auto btn1 = optString(L, 3, "Ok", "geode.PopupManager.alert");
        auto btn2 = optNullableString(L, 4, "geode.PopupManager.alert");
        auto popup = geode::PopupManager::get().alert(
            title,
            content,
            btn1,
            btn2 ? geode::ZStringView(*btn2) : geode::ZStringView(nullptr),
            optWidth(L, 5, "geode.PopupManager.alert")
        );
        pushManagedPopup(L, std::move(popup));
        return 1;
    }

    int managerQuickPopup(lua_State* L) {
        auto title = check<std::string>(L, 1, "geode.PopupManager.quickPopup");
        auto content = check<std::string>(L, 2, "geode.PopupManager.quickPopup");
        auto btn1 = optString(L, 3, "Ok", "geode.PopupManager.quickPopup");
        auto btn2 = optNullableString(L, 4, "geode.PopupManager.quickPopup");
        std::shared_ptr<LuaCallback> callback;
        if (lua_gettop(L) >= 5 && !lua_isnil(L, 5)) {
            luaL_checktype(L, 5, LUA_TFUNCTION);
            callback = std::make_shared<LuaCallback>(L, 5);
        }
        auto popup = geode::PopupManager::get().quickPopup(
            title,
            content,
            btn1,
            btn2 ? geode::ZStringView(*btn2) : geode::ZStringView(nullptr),
            callback ?
                geode::Function<void(FLAlertLayer*, bool)>([callback](FLAlertLayer* layer, bool btn2) {
                    struct Args {
                        FLAlertLayer* layer;
                        bool btn2;
                    } args{layer, btn2};
                    if (!callback->invoke(
                            2,
                            0,
                            "geode.PopupManager.quickPopup",
                            kHookScriptDeadlineMs,
                            +[](lua_State* L, void* raw) {
                                auto* args = static_cast<Args*>(raw);
                                if (args->layer)
                                    Usertype<FLAlertLayer>::pushBorrowed(L, args->layer);
                                else lua_pushnil(L);
                                lua_pushboolean(L, args->btn2);
                            },
                            &args
                        )) {
                        logCallbackFailure("geode.PopupManager.quickPopup");
                    }
                }) :
                geode::Function<void(FLAlertLayer*, bool)>{},
            optWidth(L, 6, "geode.PopupManager.quickPopup")
        );
        pushManagedPopup(L, std::move(popup));
        return 1;
    }

    int managerManage(lua_State* L) {
        auto* alert = Usertype<FLAlertLayer>::check(L, 1, "geode.PopupManager.manage");
        if (geode::PopupManager::get().isManaged(alert)) {
            luaL_error(L, "geode.PopupManager.manage: popup is already managed");
        }
        pushManagedPopup(L, geode::PopupManager::get().manage(alert));
        return 1;
    }

    int managerIsManaged(lua_State* L) {
        auto* alert = Usertype<FLAlertLayer>::check(L, 1, "geode.PopupManager.isManaged");
        lua_pushboolean(L, geode::PopupManager::get().isManaged(alert));
        return 1;
    }

    int managerHasPending(lua_State* L) {
        (void)L;
        lua_pushboolean(L, geode::PopupManager::get().hasPendingPopups());
        return 1;
    }

} // namespace

namespace luax {
    geode::Result<void> registerGeodePopup(lua_State* L) {
        registerManagedPopupMetatable(L);
        getOrCreateTable(L, "geode.PopupManager");
        lua_pushnumber(L, geode::PopupManager::DEFAULT_WIDTH);
        lua_setfield(L, -2, "DEFAULT_WIDTH");
        setTableCFunction(L, -1, "alert", &managerAlert);
        setTableCFunction(L, -1, "quickPopup", &managerQuickPopup);
        setTableCFunction(L, -1, "manage", &managerManage);
        setTableCFunction(L, -1, "isManaged", &managerIsManaged);
        setTableCFunction(L, -1, "hasPendingPopups", &managerHasPending);
        lua_pop(L, 1);

        return geode::Ok();
    }
} // namespace luax

LUAX_BINDING(geode_popup_lib, luax::registerGeodePopup)
#endif
