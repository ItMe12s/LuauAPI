#include "lua/Config.hpp"
#include "lua/bindings/Binding.hpp"
#include "lua/bindings/framework/LuaCallback.hpp"
#include "lua/bindings/framework/Stack.hpp"
#include "lua/bindings/framework/TableUtil.hpp"
#include "lua/bindings/geode/CurrentMod.hpp"
#include "lua/bindings/geode/JsonConvert.hpp"
#include "lua/module/PathSandbox.hpp"
#include "lua/runtime/Runtime.hpp"

#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Priority.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/string.hpp>
#include <Geode/utils/web.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    using namespace luax;
    namespace web = geode::utils::web;

    constexpr char const* kRequestMeta = "luax.WebRequest";
    constexpr char const* kResponseMeta = "luax.WebResponse";
    constexpr char const* kHandleMeta = "luax.WebHandle";
    constexpr char const* kMultipartMeta = "luax.MultipartForm";
    constexpr char const* kListenerMeta = "luax.WebListenerHandle";

    struct WebRequestBox {
        web::WebRequest request;
        std::shared_ptr<LuaCallback> progress;
    };

    struct WebResponseBox {
        web::WebResponse response;
    };

    struct MultipartBox {
        web::MultipartForm form;
    };

    struct WebTask {
        geode::async::TaskHolder<web::WebResponse> holder;
        std::size_t id = 0;
        bool done = false;

        explicit WebTask(std::size_t taskID) : id(taskID) {}

        ~WebTask() {
            cancel();
        }

        void cancel() {
            if (done) return;
            holder.cancel();
            done = true;
        }
    };

    struct WebHandleBox {
        std::shared_ptr<WebTask> task;
    };

    struct WebListenerState {
        geode::ListenerHandle handle;
        bool connected = true;

        ~WebListenerState() {
            disconnect();
        }

        void disconnect() {
            if (!connected) return;
            handle = geode::ListenerHandle();
            connected = false;
        }
    };

    struct WebListenerBox {
        std::shared_ptr<WebListenerState> state;
    };

    std::vector<std::weak_ptr<WebTask>>& activeTasks() {
        static auto* tasks = new std::vector<std::weak_ptr<WebTask>>();
        return *tasks;
    }

    std::vector<std::weak_ptr<WebListenerState>>& activeListeners() {
        static auto* listeners = new std::vector<std::weak_ptr<WebListenerState>>();
        return *listeners;
    }

    bool s_shutdownHookRegistered = false;

    void clearWebState() {
        for (auto& weak : activeTasks()) {
            if (auto task = weak.lock()) {
                task->cancel();
            }
        }
        activeTasks().clear();
        for (auto& weak : activeListeners()) {
            if (auto listener = weak.lock()) {
                listener->disconnect();
            }
        }
        activeListeners().clear();
        s_shutdownHookRegistered = false;
    }

    void ensureShutdownHook() {
        if (s_shutdownHookRegistered) return;
        auto* rt = Runtime::getIfInitialized();
        if (!rt) return;
        rt->registerShutdownHook(&clearWebState);
        s_shutdownHookRegistered = true;
    }

    void compactWeakState() {
        auto& tasks = activeTasks();
        tasks.erase(
            std::remove_if(
                tasks.begin(),
                tasks.end(),
                [](auto const& weak) {
                    return weak.expired();
                }
            ),
            tasks.end()
        );
        auto& listeners = activeListeners();
        listeners.erase(
            std::remove_if(
                listeners.begin(),
                listeners.end(),
                [](auto const& weak) {
                    return weak.expired();
                }
            ),
            listeners.end()
        );
    }

    WebRequestBox* checkRequestBox(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<WebRequestBox*>(luaL_checkudata(L, idx, kRequestMeta));
        if (!box) luaL_error(L, "%s expected WebRequest at arg %d", method, idx);
        return box;
    }

    web::WebRequest& checkRequest(lua_State* L, int idx, char const* method) {
        return checkRequestBox(L, idx, method)->request;
    }

    web::WebResponse& checkResponse(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<WebResponseBox*>(luaL_checkudata(L, idx, kResponseMeta));
        if (!box) luaL_error(L, "%s expected WebResponse at arg %d", method, idx);
        return box->response;
    }

    MultipartBox* checkMultipartBox(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<MultipartBox*>(luaL_checkudata(L, idx, kMultipartMeta));
        if (!box) luaL_error(L, "%s expected MultipartForm at arg %d", method, idx);
        return box;
    }

    WebHandleBox* checkHandle(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<WebHandleBox*>(luaL_checkudata(L, idx, kHandleMeta));
        if (!box) luaL_error(L, "%s expected WebHandle at arg %d", method, idx);
        return box;
    }

    WebListenerBox* checkListener(lua_State* L, int idx, char const* method) {
        auto* box = static_cast<WebListenerBox*>(luaL_checkudata(L, idx, kListenerMeta));
        if (!box) luaL_error(L, "%s expected WebListenerHandle at arg %d", method, idx);
        return box;
    }

    void pushRequest(lua_State* L, web::WebRequest request) {
        auto* box = static_cast<WebRequestBox*>(lua_newuserdata(L, sizeof(WebRequestBox)));
        new (box) WebRequestBox{std::move(request), nullptr};
        luaL_getmetatable(L, kRequestMeta);
        lua_setmetatable(L, -2);
    }

    void pushResponse(lua_State* L, web::WebResponse response) {
        auto* box = static_cast<WebResponseBox*>(lua_newuserdata(L, sizeof(WebResponseBox)));
        new (box) WebResponseBox{std::move(response)};
        luaL_getmetatable(L, kResponseMeta);
        lua_setmetatable(L, -2);
    }

    void pushMultipart(lua_State* L, web::MultipartForm form = web::MultipartForm()) {
        auto* box = static_cast<MultipartBox*>(lua_newuserdata(L, sizeof(MultipartBox)));
        new (box) MultipartBox{std::move(form)};
        luaL_getmetatable(L, kMultipartMeta);
        lua_setmetatable(L, -2);
    }

    void pushHandle(lua_State* L, std::shared_ptr<WebTask> task) {
        auto* box = static_cast<WebHandleBox*>(lua_newuserdata(L, sizeof(WebHandleBox)));
        new (box) WebHandleBox{std::move(task)};
        luaL_getmetatable(L, kHandleMeta);
        lua_setmetatable(L, -2);
    }

    void pushListener(lua_State* L, std::shared_ptr<WebListenerState> state) {
        auto* box = static_cast<WebListenerBox*>(lua_newuserdata(L, sizeof(WebListenerBox)));
        new (box) WebListenerBox{std::move(state)};
        luaL_getmetatable(L, kListenerMeta);
        lua_setmetatable(L, -2);
    }

    int requestGc(lua_State* L) {
        auto* box = static_cast<WebRequestBox*>(luaL_checkudata(L, 1, kRequestMeta));
        box->~WebRequestBox();
        return 0;
    }

    int responseGc(lua_State* L) {
        auto* box = static_cast<WebResponseBox*>(luaL_checkudata(L, 1, kResponseMeta));
        box->~WebResponseBox();
        return 0;
    }

    int multipartGc(lua_State* L) {
        auto* box = static_cast<MultipartBox*>(luaL_checkudata(L, 1, kMultipartMeta));
        box->~MultipartBox();
        return 0;
    }

    int handleGc(lua_State* L) {
        auto* box = static_cast<WebHandleBox*>(luaL_checkudata(L, 1, kHandleMeta));
        if (box->task) box->task->cancel();
        box->~WebHandleBox();
        return 0;
    }

    int listenerGc(lua_State* L) {
        auto* box = static_cast<WebListenerBox*>(luaL_checkudata(L, 1, kListenerMeta));
        box->~WebListenerBox();
        return 0;
    }

    void setIntField(lua_State* L, char const* name, int value) {
        lua_pushinteger(L, value);
        lua_setfield(L, -2, name);
    }

    void setLongField(lua_State* L, char const* name, long value) {
        lua_pushnumber(L, static_cast<double>(value));
        lua_setfield(L, -2, name);
    }

    int optPriority(lua_State* L, int idx) {
        if (lua_gettop(L) < idx || lua_isnil(L, idx)) return geode::Priority::Normal;
        return check<int>(L, idx, "geode.utils.web listener");
    }

    std::uint64_t checkNonNegativeInteger(lua_State* L, int idx, char const* method) {
        if (!lua_isnumber(L, idx))
            luaL_error(L, "%s expected non-negative integer at arg %d", method, idx);
        auto value = lua_tointeger(L, idx);
        if (value < 0) luaL_error(L, "%s expected non-negative integer at arg %d", method, idx);
        return static_cast<std::uint64_t>(value);
    }

    bool optField(lua_State* L, int tableIdx, char const* key) {
        lua_getfield(L, tableIdx, key);
        bool present = !lua_isnil(L, -1);
        lua_pop(L, 1);
        return present;
    }

    std::optional<std::string> optStringField(
        lua_State* L, int tableIdx, char const* key, char const* method
    ) {
        lua_getfield(L, tableIdx, key);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        auto value = check<std::string>(L, -1, method);
        lua_pop(L, 1);
        return value;
    }

    std::optional<bool> optBoolField(lua_State* L, int tableIdx, char const* key, char const* method) {
        lua_getfield(L, tableIdx, key);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        auto value = check<bool>(L, -1, method);
        lua_pop(L, 1);
        return value;
    }

    std::optional<double> optNumberField(lua_State* L, int tableIdx, char const* key, char const* method) {
        lua_getfield(L, tableIdx, key);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        if (!lua_isnumber(L, -1)) luaL_error(L, "%s expected number field '%s'", method, key);
        auto value = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return value;
    }

    web::HttpVersion checkHttpVersion(lua_State* L, int idx, char const* method) {
        int value = check<int>(L, idx, method);
        switch (value) {
            case static_cast<int>(web::HttpVersion::DEFAULT):
            case static_cast<int>(web::HttpVersion::VERSION_1_0):
            case static_cast<int>(web::HttpVersion::VERSION_1_1):
            case static_cast<int>(web::HttpVersion::VERSION_2_0):
            case static_cast<int>(web::HttpVersion::VERSION_2TLS):
            case static_cast<int>(web::HttpVersion::VERSION_2_PRIOR_KNOWLEDGE):
            case static_cast<int>(web::HttpVersion::VERSION_3):
            case static_cast<int>(web::HttpVersion::VERSION_3ONLY):
                return static_cast<web::HttpVersion>(value);
            default: luaL_error(L, "%s: invalid HttpVersion %d", method, value);
        }
    }

    web::ProxyType checkProxyType(lua_State* L, int idx, char const* method) {
        int value = check<int>(L, idx, method);
        if (value < static_cast<int>(web::ProxyType::HTTP) ||
            value > static_cast<int>(web::ProxyType::SOCKS5H)) {
            luaL_error(L, "%s: invalid ProxyType %d", method, value);
        }
        return static_cast<web::ProxyType>(value);
    }

    web::ProxyOpts checkProxyOpts(lua_State* L, int idx, char const* method) {
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);

        web::ProxyOpts opts;
        opts.address = fieldString(L, idx, "address", method);

        if (auto port = optNumberField(L, idx, "port", method)) {
            if (*port < 0 || *port > 65535)
                luaL_error(L, "%s expected proxy port 0..65535", method);
            opts.port = static_cast<std::uint16_t>(*port);
        }
        if (optField(L, idx, "type")) {
            lua_getfield(L, idx, "type");
            opts.type = checkProxyType(L, -1, method);
            lua_pop(L, 1);
        }
        if (auto auth = optNumberField(L, idx, "auth", method)) {
            opts.auth = static_cast<long>(*auth);
        }
        if (auto username = optStringField(L, idx, "username", method))
            opts.username = std::move(*username);
        if (auto password = optStringField(L, idx, "password", method))
            opts.password = std::move(*password);
        if (auto tunneling = optBoolField(L, idx, "tunneling", method)) opts.tunneling = *tunneling;
        if (auto cert = optBoolField(L, idx, "certVerification", method))
            opts.certVerification = *cert;
        return opts;
    }

    void pushProgress(lua_State* L, web::WebProgress const& p) {
        lua_createtable(L, 0, 6);
        push(L, static_cast<double>(p.downloaded()));
        lua_setfield(L, -2, "downloaded");
        push(L, static_cast<double>(p.downloadTotal()));
        lua_setfield(L, -2, "downloadTotal");
        if (auto progress = p.downloadProgress()) {
            push(L, static_cast<double>(*progress));
        }
        else {
            lua_pushnil(L);
        }
        lua_setfield(L, -2, "downloadProgress");
        push(L, static_cast<double>(p.uploaded()));
        lua_setfield(L, -2, "uploaded");
        push(L, static_cast<double>(p.uploadTotal()));
        lua_setfield(L, -2, "uploadTotal");
        if (auto progress = p.uploadProgress()) {
            push(L, static_cast<double>(*progress));
        }
        else {
            lua_pushnil(L);
        }
        lua_setfield(L, -2, "uploadProgress");
    }

    void setProgressCallback(web::WebRequest& req, std::shared_ptr<LuaCallback> cb) {
        req.onProgress([cb = std::move(cb)](web::WebProgress const& progress) {
            if (!cb || !cb->valid()) return;
            geode::queueInMainThread([cb, progress] {
                if (!cb || !cb->valid()) return;
                struct Ctx {
                    web::WebProgress const* progress;
                } ctx{&progress};
                cb->invoke(
                    1,
                    0,
                    "geode.utils.web.onProgress",
                    kHookScriptDeadlineMs,
                    +[](lua_State* L, void* raw) {
                        pushProgress(L, *static_cast<Ctx*>(raw)->progress);
                    },
                    &ctx
                );
            });
        });
    }

    void applyStringMap(lua_State* L, int idx, char const* field, char const* method, auto&& setter) {
        lua_getfield(L, idx, field);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return;
        }
        luaL_checktype(L, -1, LUA_TTABLE);
        int table = lua_absindex(L, -1);
        lua_pushnil(L);
        while (lua_next(L, table) != 0) {
            auto key = check<std::string>(L, -2, method);
            auto value = check<std::string>(L, -1, method);
            setter(std::move(key), std::move(value));
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }

    std::optional<std::pair<std::uint64_t, std::uint64_t>> optDownloadRange(
        lua_State* L, int idx, char const* method
    ) {
        lua_getfield(L, idx, "downloadRange");
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            return std::nullopt;
        }
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_rawgeti(L, -1, 1);
        auto start = checkNonNegativeInteger(L, -1, method);
        lua_pop(L, 1);
        lua_rawgeti(L, -1, 2);
        auto stop = checkNonNegativeInteger(L, -1, method);
        lua_pop(L, 2);
        if (start > stop) luaL_error(L, "%s expected downloadRange start <= stop", method);
        return std::pair<std::uint64_t, std::uint64_t>{start, stop};
    }

    void applyOptions(lua_State* L, web::WebRequest& req, int idx, char const* method) {
        if (idx <= 0 || lua_gettop(L) < idx || lua_isnil(L, idx)) return;
        idx = lua_absindex(L, idx);
        luaL_checktype(L, idx, LUA_TTABLE);

        applyStringMap(L, idx, "headers", method, [&](std::string name, std::string value) {
            req.header(std::move(name), std::move(value));
        });
        applyStringMap(L, idx, "params", method, [&](std::string name, std::string value) {
            req.param(std::move(name), std::move(value));
        });

        if (auto value = optStringField(L, idx, "method", method)) req.method(std::move(*value));
        if (auto value = optStringField(L, idx, "url", method)) req.url(std::move(*value));
        if (auto value = optStringField(L, idx, "userAgent", method))
            req.userAgent(std::move(*value));
        if (auto value = optStringField(L, idx, "acceptEncoding", method))
            req.acceptEncoding(std::move(*value));
        if (auto value = optNumberField(L, idx, "timeout", method)) {
            if (*value < 0) luaL_error(L, "%s expected timeout >= 0", method);
            req.timeout(std::chrono::seconds(static_cast<std::int64_t>(*value)));
        }
        if (auto value = optDownloadRange(L, idx, method)) req.downloadRange(*value);
        if (auto value = optBoolField(L, idx, "certVerification", method))
            req.certVerification(*value);
        if (auto value = optBoolField(L, idx, "transferBody", method)) req.transferBody(*value);
        if (auto value = optBoolField(L, idx, "followRedirects", method))
            req.followRedirects(*value);
        if (auto value = optBoolField(L, idx, "ignoreContentLength", method))
            req.ignoreContentLength(*value);
        if (auto value = optStringField(L, idx, "caBundle", method))
            req.CABundleContent(std::move(*value));

        lua_getfield(L, idx, "proxy");
        if (!lua_isnil(L, -1)) req.proxyOpts(checkProxyOpts(L, -1, method));
        lua_pop(L, 1);

        lua_getfield(L, idx, "version");
        if (!lua_isnil(L, -1)) req.version(checkHttpVersion(L, -1, method));
        lua_pop(L, 1);

        if (auto value = optStringField(L, idx, "body", method)) {
            geode::ByteVector bytes(value->begin(), value->end());
            req.body(std::move(bytes));
        }
        if (auto value = optStringField(L, idx, "bodyString", method)) req.bodyString(*value);

        lua_getfield(L, idx, "bodyJson");
        if (!lua_isnil(L, -1)) req.bodyJSON(toJson(L, -1, 0));
        lua_pop(L, 1);

        lua_getfield(L, idx, "bodyMultipart");
        if (!lua_isnil(L, -1)) req.bodyMultipart(checkMultipartBox(L, -1, method)->form);
        lua_pop(L, 1);

        lua_getfield(L, idx, "onProgress");
        if (!lua_isnil(L, -1)) {
            luaL_checktype(L, -1, LUA_TFUNCTION);
            setProgressCallback(req, std::make_shared<LuaCallback>(L, -1));
        }
        lua_pop(L, 1);
    }

    bool rootDir(lua_State* L, std::string const& name, std::filesystem::path& out, bool& writable) {
        auto* mod = requireCurrentMod(L);
        if (name == "save") {
            out = mod->getSaveDir();
            writable = true;
            return true;
        }
        if (name == "config") {
            out = mod->getConfigDir();
            writable = true;
            return true;
        }
        if (name == "persistent") {
            out = mod->getPersistentDir();
            writable = true;
            return true;
        }
        if (name == "resources") {
            out = mod->getResourcesDir();
            writable = false;
            return true;
        }
        return false;
    }

    std::optional<std::filesystem::path> resolveSandboxTarget(
        lua_State* L, int rootIdx, int pathIdx, char const* method, bool requireWritable
    ) {
        auto root = check<std::string>(L, rootIdx, method);
        auto rel = check<std::string>(L, pathIdx, method);
        std::filesystem::path dir;
        bool writable = false;
        if (!rootDir(L, root, dir, writable)) {
            luaL_error(
                L,
                "%s: unknown root '%s' (expected save/config/persistent/resources)",
                method,
                root.c_str()
            );
        }
        if (requireWritable && !writable) {
            lua_pushnil(L);
            push(L, std::string("root is read-only"));
            return std::nullopt;
        }
        auto resolved = resolveInsideRoot(dir, rel);
        if (resolved.isErr()) {
            lua_pushnil(L);
            push(L, std::string(resolved.unwrapErr()));
            return std::nullopt;
        }
        return resolved.unwrap();
    }

    std::shared_ptr<WebTask> startRequest(
        lua_State* L, web::WebRequest& req, std::string method, std::string url, int callbackIdx
    ) {
        luaL_checktype(L, callbackIdx, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, callbackIdx);
        auto task = std::make_shared<WebTask>(req.getID());
        activeTasks().push_back(task);
        compactWeakState();
        ensureShutdownHook();

        auto* mod = requireCurrentMod(L);
        auto future = req.send(std::move(method), std::move(url), mod);
        task->holder.spawn(
            "LuauAPI web request", std::move(future), [cb, task](web::WebResponse response) mutable {
                task->done = true;
                if (!cb || !cb->valid()) return;
                struct Ctx {
                    web::WebResponse* response;
                } ctx{&response};
                cb->invoke(
                    1,
                    0,
                    "geode.utils.web request",
                    kHookScriptDeadlineMs,
                    +[](lua_State* L, void* raw) {
                        pushResponse(L, std::move(*static_cast<Ctx*>(raw)->response));
                    },
                    &ctx
                );
            }
        );
        return task;
    }

    int parseRequestCall(lua_State* L, int optionsIdx, int callbackIdx, int& outOptions, int& outCallback) {
        if (lua_isfunction(L, optionsIdx)) {
            outOptions = 0;
            outCallback = optionsIdx;
            return outCallback;
        }
        outOptions = optionsIdx;
        outCallback = callbackIdx;
        luaL_checktype(L, outCallback, LUA_TFUNCTION);
        return outCallback;
    }

    int webOpenLinkInBrowser(lua_State* L) {
        auto url = check<std::string>(L, 1, "geode.utils.web.openLinkInBrowser");
        web::openLinkInBrowser(url);
        return 0;
    }

    int webNewRequest(lua_State* L) {
        pushRequest(L, web::WebRequest());
        return 1;
    }

    int webMultipart(lua_State* L) {
        pushMultipart(L);
        return 1;
    }

    int sendConvenience(lua_State* L, std::string method, char const* context) {
        auto url = check<std::string>(L, 1, context);
        int optionsIdx = 0;
        int callbackIdx = 0;
        parseRequestCall(L, 2, 3, optionsIdx, callbackIdx);
        web::WebRequest req;
        applyOptions(L, req, optionsIdx, context);
        auto task = startRequest(L, req, std::move(method), std::move(url), callbackIdx);
        pushHandle(L, std::move(task));
        return 1;
    }

    int webGet(lua_State* L) {
        return sendConvenience(L, "GET", "geode.utils.web.get");
    }

    int webPost(lua_State* L) {
        return sendConvenience(L, "POST", "geode.utils.web.post");
    }

    int webPut(lua_State* L) {
        return sendConvenience(L, "PUT", "geode.utils.web.put");
    }

    int webPatch(lua_State* L) {
        return sendConvenience(L, "PATCH", "geode.utils.web.patch");
    }

    int webFetch(lua_State* L) {
        web::WebRequest req;
        std::string url;
        std::string method = "GET";
        int optionsIdx = 0;
        int callbackIdx = 0;

        if (lua_istable(L, 1)) {
            optionsIdx = 1;
            callbackIdx = 2;
            luaL_checktype(L, callbackIdx, LUA_TFUNCTION);
            auto maybeUrl = optStringField(L, 1, "url", "geode.utils.web.fetch");
            if (!maybeUrl) luaL_error(L, "geode.utils.web.fetch expected url field");
            url = std::move(*maybeUrl);
            if (auto value = optStringField(L, 1, "method", "geode.utils.web.fetch"))
                method = std::move(*value);
        }
        else {
            url = check<std::string>(L, 1, "geode.utils.web.fetch");
            parseRequestCall(L, 2, 3, optionsIdx, callbackIdx);
            if (optionsIdx > 0) {
                if (auto value = optStringField(L, optionsIdx, "method", "geode.utils.web.fetch")) {
                    method = std::move(*value);
                }
            }
        }

        applyOptions(L, req, optionsIdx, "geode.utils.web.fetch");
        auto task = startRequest(L, req, std::move(method), std::move(url), callbackIdx);
        pushHandle(L, std::move(task));
        return 1;
    }

    int requestChain(lua_State* L, auto&& fn, char const* method) {
        auto& req = checkRequest(L, 1, method);
        fn(req);
        lua_pushvalue(L, 1);
        return 1;
    }

    int requestHeader(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebRequest:header");
        auto value = check<std::string>(L, 3, "WebRequest:header");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.header(std::move(name), std::move(value));
            },
            "WebRequest:header"
        );
    }

    int requestRemoveHeader(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebRequest:removeHeader");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.removeHeader(name);
            },
            "WebRequest:removeHeader"
        );
    }

    int requestParam(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebRequest:param");
        auto value = check<std::string>(L, 3, "WebRequest:param");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.param(std::move(name), std::move(value));
            },
            "WebRequest:param"
        );
    }

    int requestRemoveParam(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebRequest:removeParam");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.removeParam(name);
            },
            "WebRequest:removeParam"
        );
    }

    int requestMethod(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:method");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.method(std::move(value));
            },
            "WebRequest:method"
        );
    }

    int requestUrl(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:url");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.url(std::move(value));
            },
            "WebRequest:url"
        );
    }

    int requestUserAgent(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:userAgent");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.userAgent(std::move(value));
            },
            "WebRequest:userAgent"
        );
    }

    int requestAcceptEncoding(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:acceptEncoding");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.acceptEncoding(std::move(value));
            },
            "WebRequest:acceptEncoding"
        );
    }

    int requestTimeout(lua_State* L) {
        auto seconds = check<int>(L, 2, "WebRequest:timeout");
        if (seconds < 0) luaL_error(L, "WebRequest:timeout expected seconds >= 0");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.timeout(std::chrono::seconds(seconds));
            },
            "WebRequest:timeout"
        );
    }

    int requestDownloadRange(lua_State* L) {
        auto start = checkNonNegativeInteger(L, 2, "WebRequest:downloadRange");
        auto stop = checkNonNegativeInteger(L, 3, "WebRequest:downloadRange");
        if (start > stop) luaL_error(L, "WebRequest:downloadRange expected start <= stop");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.downloadRange({start, stop});
            },
            "WebRequest:downloadRange"
        );
    }

    int requestCertVerification(lua_State* L) {
        auto value = check<bool>(L, 2, "WebRequest:certVerification");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.certVerification(value);
            },
            "WebRequest:certVerification"
        );
    }

    int requestTransferBody(lua_State* L) {
        auto value = check<bool>(L, 2, "WebRequest:transferBody");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.transferBody(value);
            },
            "WebRequest:transferBody"
        );
    }

    int requestFollowRedirects(lua_State* L) {
        auto value = check<bool>(L, 2, "WebRequest:followRedirects");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.followRedirects(value);
            },
            "WebRequest:followRedirects"
        );
    }

    int requestIgnoreContentLength(lua_State* L) {
        auto value = check<bool>(L, 2, "WebRequest:ignoreContentLength");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.ignoreContentLength(value);
            },
            "WebRequest:ignoreContentLength"
        );
    }

    int requestCaBundle(lua_State* L) {
        auto value = check<std::string>(L, 2, "WebRequest:caBundle");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.CABundleContent(std::move(value));
            },
            "WebRequest:caBundle"
        );
    }

    int requestProxy(lua_State* L) {
        auto opts = checkProxyOpts(L, 2, "WebRequest:proxy");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.proxyOpts(std::move(opts));
            },
            "WebRequest:proxy"
        );
    }

    int requestVersion(lua_State* L) {
        auto version = checkHttpVersion(L, 2, "WebRequest:version");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.version(version);
            },
            "WebRequest:version"
        );
    }

    int requestBody(lua_State* L) {
        auto data = check<std::string>(L, 2, "WebRequest:body");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                geode::ByteVector bytes(data.begin(), data.end());
                req.body(std::move(bytes));
            },
            "WebRequest:body"
        );
    }

    int requestBodyString(lua_State* L) {
        auto data = check<std::string>(L, 2, "WebRequest:bodyString");
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.bodyString(data);
            },
            "WebRequest:bodyString"
        );
    }

    int requestBodyJson(lua_State* L) {
        auto value = toJson(L, 2, 0);
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.bodyJSON(value);
            },
            "WebRequest:bodyJson"
        );
    }

    int requestBodyMultipart(lua_State* L) {
        auto& form = checkMultipartBox(L, 2, "WebRequest:bodyMultipart")->form;
        return requestChain(
            L,
            [&](web::WebRequest& req) {
                req.bodyMultipart(form);
            },
            "WebRequest:bodyMultipart"
        );
    }

    int requestOnProgress(lua_State* L) {
        auto* box = checkRequestBox(L, 1, "WebRequest:onProgress");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        box->progress = std::make_shared<LuaCallback>(L, 2);
        setProgressCallback(box->request, box->progress);
        lua_pushvalue(L, 1);
        return 1;
    }

    int requestSendWithMethod(lua_State* L, std::string method, char const* context) {
        auto& req = checkRequest(L, 1, context);
        auto url = check<std::string>(L, 2, context);
        int optionsIdx = 0;
        int callbackIdx = 0;
        parseRequestCall(L, 3, 4, optionsIdx, callbackIdx);
        applyOptions(L, req, optionsIdx, context);
        auto task = startRequest(L, req, std::move(method), std::move(url), callbackIdx);
        pushHandle(L, std::move(task));
        return 1;
    }

    int requestSend(lua_State* L) {
        auto& req = checkRequest(L, 1, "WebRequest:send");
        auto method = check<std::string>(L, 2, "WebRequest:send");
        auto url = check<std::string>(L, 3, "WebRequest:send");
        int optionsIdx = 0;
        int callbackIdx = 0;
        parseRequestCall(L, 4, 5, optionsIdx, callbackIdx);
        applyOptions(L, req, optionsIdx, "WebRequest:send");
        auto task = startRequest(L, req, std::move(method), std::move(url), callbackIdx);
        pushHandle(L, std::move(task));
        return 1;
    }

    int requestGet(lua_State* L) {
        return requestSendWithMethod(L, "GET", "WebRequest:get");
    }

    int requestPost(lua_State* L) {
        return requestSendWithMethod(L, "POST", "WebRequest:post");
    }

    int requestPut(lua_State* L) {
        return requestSendWithMethod(L, "PUT", "WebRequest:put");
    }

    int requestPatch(lua_State* L) {
        return requestSendWithMethod(L, "PATCH", "WebRequest:patch");
    }

    int requestGetID(lua_State* L) {
        push(L, static_cast<double>(checkRequest(L, 1, "WebRequest:getID").getID()));
        return 1;
    }

    int requestGetMethod(lua_State* L) {
        push(L, std::string_view(checkRequest(L, 1, "WebRequest:getMethod").getMethod()));
        return 1;
    }

    int requestGetUrl(lua_State* L) {
        push(L, std::string_view(checkRequest(L, 1, "WebRequest:getUrl").getUrl()));
        return 1;
    }

    int requestGetHeaders(lua_State* L) {
        auto const& headers = checkRequest(L, 1, "WebRequest:getHeaders").getHeaders();
        lua_createtable(L, 0, static_cast<int>(headers.size()));
        for (auto const& [name, values] : headers) {
            lua_createtable(L, static_cast<int>(values.size()), 0);
            int i = 1;
            for (auto const& value : values) {
                push(L, value);
                lua_rawseti(L, -2, i++);
            }
            lua_setfield(L, -2, name.c_str());
        }
        return 1;
    }

    int requestGetUrlParams(lua_State* L) {
        auto const& params = checkRequest(L, 1, "WebRequest:getUrlParams").getUrlParams();
        lua_createtable(L, 0, static_cast<int>(params.size()));
        for (auto const& [name, value] : params) {
            push(L, value);
            lua_setfield(L, -2, name.c_str());
        }
        return 1;
    }

    int requestGetBody(lua_State* L) {
        auto body = checkRequest(L, 1, "WebRequest:getBody").getBody();
        if (!body) {
            lua_pushnil(L);
            return 1;
        }
        lua_pushlstring(L, reinterpret_cast<char const*>(body->data()), body->size());
        return 1;
    }

    int requestGetTimeout(lua_State* L) {
        auto timeout = checkRequest(L, 1, "WebRequest:getTimeout").getTimeout();
        if (!timeout) {
            lua_pushnil(L);
            return 1;
        }
        push(L, static_cast<double>(timeout->count()));
        return 1;
    }

    int requestGetVersion(lua_State* L) {
        push(L, static_cast<int>(checkRequest(L, 1, "WebRequest:getVersion").getHttpVersion()));
        return 1;
    }

    int requestGetProgress(lua_State* L) {
        pushProgress(L, checkRequest(L, 1, "WebRequest:getProgress").getProgress());
        return 1;
    }

    int responseBool(lua_State* L, bool (web::WebResponse::*fn)() const, char const* method) {
        push(L, (checkResponse(L, 1, method).*fn)());
        return 1;
    }

    int responseInfo(lua_State* L) {
        return responseBool(L, &web::WebResponse::info, "WebResponse:info");
    }

    int responseOk(lua_State* L) {
        return responseBool(L, &web::WebResponse::ok, "WebResponse:ok");
    }

    int responseRedirected(lua_State* L) {
        return responseBool(L, &web::WebResponse::redirected, "WebResponse:redirected");
    }

    int responseBadClient(lua_State* L) {
        return responseBool(L, &web::WebResponse::badClient, "WebResponse:badClient");
    }

    int responseBadServer(lua_State* L) {
        return responseBool(L, &web::WebResponse::badServer, "WebResponse:badServer");
    }

    int responseError(lua_State* L) {
        return responseBool(L, &web::WebResponse::error, "WebResponse:error");
    }

    int responseCancelled(lua_State* L) {
        return responseBool(L, &web::WebResponse::cancelled, "WebResponse:cancelled");
    }

    int responseCode(lua_State* L) {
        push(L, checkResponse(L, 1, "WebResponse:code").code());
        return 1;
    }

    int responseText(lua_State* L) {
        auto result = checkResponse(L, 1, "WebResponse:text").string();
        if (result.isErr()) {
            lua_pushnil(L);
            push(L, std::string(result.unwrapErr()));
            return 2;
        }
        push(L, result.unwrap());
        return 1;
    }

    int responseJson(lua_State* L) {
        auto result = checkResponse(L, 1, "WebResponse:json").json();
        if (result.isErr()) {
            lua_pushnil(L);
            push(L, std::string(result.unwrapErr()));
            return 2;
        }
        pushJson(L, result.unwrap(), 0);
        return 1;
    }

    int responseBytes(lua_State* L) {
        auto const& data = checkResponse(L, 1, "WebResponse:bytes").data();
        lua_pushlstring(L, reinterpret_cast<char const*>(data.data()), data.size());
        return 1;
    }

    int responseSaveTo(lua_State* L) {
        auto& response = checkResponse(L, 1, "WebResponse:saveTo");
        auto target = resolveSandboxTarget(L, 2, 3, "WebResponse:saveTo", true);
        if (!target) return 2;

        auto parent = target->parent_path();
        if (!parent.empty()) {
            auto made = geode::utils::file::createDirectoryAll(parent);
            if (made.isErr()) {
                lua_pushnil(L);
                push(L, std::string(made.unwrapErr()));
                return 2;
            }
        }
        auto result = response.into(*target);
        if (result.isErr()) {
            lua_pushnil(L);
            push(L, std::string(result.unwrapErr()));
            return 2;
        }
        push(L, true);
        return 1;
    }

    int responseHeaders(lua_State* L) {
        auto headers = checkResponse(L, 1, "WebResponse:headers").headers();
        lua_createtable(L, static_cast<int>(headers.size()), 0);
        int i = 1;
        for (auto const& header : headers) {
            push(L, header);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    int responseHeader(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebResponse:header");
        auto header = checkResponse(L, 1, "WebResponse:header").header(name);
        if (!header) {
            lua_pushnil(L);
            return 1;
        }
        push(L, std::string_view(*header));
        return 1;
    }

    int responseHeadersNamed(lua_State* L) {
        auto name = check<std::string>(L, 2, "WebResponse:headersNamed");
        auto headers = checkResponse(L, 1, "WebResponse:headersNamed").getAllHeadersNamed(name);
        if (!headers) {
            lua_pushnil(L);
            return 1;
        }
        lua_createtable(L, static_cast<int>(headers->size()), 0);
        int i = 1;
        for (auto const& header : *headers) {
            push(L, header);
            lua_rawseti(L, -2, i++);
        }
        return 1;
    }

    int responseErrorMessage(lua_State* L) {
        push(L, checkResponse(L, 1, "WebResponse:errorMessage").errorMessage());
        return 1;
    }

    int responseVerboseLogs(lua_State* L) {
        push(L, checkResponse(L, 1, "WebResponse:verboseLogs").verboseLogs());
        return 1;
    }

    void setDurationField(lua_State* L, char const* name, auto const& duration) {
        push(L, static_cast<double>(duration.template millis<double>()));
        lua_setfield(L, -2, name);
    }

    int responseTimings(lua_State* L) {
        auto const& timings = checkResponse(L, 1, "WebResponse:timings").timings();
        lua_createtable(L, 0, 8);
        setDurationField(L, "queueWait", timings.queueWait);
        setDurationField(L, "nameLookup", timings.nameLookup);
        setDurationField(L, "connect", timings.connect);
        setDurationField(L, "tlsHandshake", timings.tlsHandshake);
        setDurationField(L, "requestSend", timings.requestSend);
        setDurationField(L, "firstByte", timings.firstByte);
        setDurationField(L, "download", timings.download);
        setDurationField(L, "total", timings.total);
        return 1;
    }

    int multipartParam(lua_State* L) {
        auto* box = checkMultipartBox(L, 1, "MultipartForm:param");
        auto name = check<std::string>(L, 2, "MultipartForm:param");
        auto value = check<std::string>(L, 3, "MultipartForm:param");
        box->form.param(std::move(name), std::move(value));
        lua_pushvalue(L, 1);
        return 1;
    }

    int multipartFile(lua_State* L) {
        auto* box = checkMultipartBox(L, 1, "MultipartForm:file");
        auto name = check<std::string>(L, 2, "MultipartForm:file");
        auto data = check<std::string>(L, 3, "MultipartForm:file");
        auto filename = check<std::string>(L, 4, "MultipartForm:file");
        auto mime = lua_gettop(L) >= 5 && !lua_isnil(L, 5) ?
            check<std::string>(L, 5, "MultipartForm:file") :
            std::string("application/octet-stream");
        auto bytes =
            std::span<uint8_t const>(reinterpret_cast<uint8_t const*>(data.data()), data.size());
        box->form.file(std::move(name), bytes, std::move(filename), std::move(mime));
        lua_pushvalue(L, 1);
        return 1;
    }

    int multipartFileFrom(lua_State* L) {
        auto* box = checkMultipartBox(L, 1, "MultipartForm:fileFrom");
        auto name = check<std::string>(L, 2, "MultipartForm:fileFrom");
        auto target = resolveSandboxTarget(L, 3, 4, "MultipartForm:fileFrom", false);
        if (!target) return 2;
        auto mime = lua_gettop(L) >= 5 && !lua_isnil(L, 5) ?
            check<std::string>(L, 5, "MultipartForm:fileFrom") :
            std::string("application/octet-stream");
        auto result = box->form.file(std::move(name), *target, std::move(mime));
        if (result.isErr()) {
            lua_pushnil(L);
            push(L, std::string(result.unwrapErr()));
            return 2;
        }
        lua_pushvalue(L, 1);
        return 1;
    }

    int multipartGetBoundary(lua_State* L) {
        push(L, checkMultipartBox(L, 1, "MultipartForm:getBoundary")->form.getBoundary());
        return 1;
    }

    int multipartGetHeader(lua_State* L) {
        push(L, checkMultipartBox(L, 1, "MultipartForm:getHeader")->form.getHeader());
        return 1;
    }

    int multipartGetBody(lua_State* L) {
        auto body = checkMultipartBox(L, 1, "MultipartForm:getBody")->form.getBody();
        lua_pushlstring(L, reinterpret_cast<char const*>(body.data()), body.size());
        return 1;
    }

    int handleCancel(lua_State* L) {
        auto* box = checkHandle(L, 1, "WebHandle:cancel");
        bool didCancel = box->task && !box->task->done;
        if (box->task) box->task->cancel();
        push(L, didCancel);
        return 1;
    }

    int handleId(lua_State* L) {
        auto* box = checkHandle(L, 1, "WebHandle:id");
        if (!box->task) {
            lua_pushnil(L);
            return 1;
        }
        push(L, static_cast<double>(box->task->id));
        return 1;
    }

    int listenerDisconnect(lua_State* L) {
        auto* box = checkListener(L, 1, "WebListenerHandle:disconnect");
        if (box->state) box->state->disconnect();
        return 0;
    }

    bool invokeRequestEvent(
        std::shared_ptr<LuaCallback> const& cb, char const* context,
        std::optional<std::string_view> modID, web::WebRequest& request
    ) {
        if (!Runtime::isMainThread() || !cb || !cb->valid()) return false;

        struct Ctx {
            std::optional<std::string_view> modID;
            web::WebRequest* request;
            bool stop = false;
        } ctx{modID, &request, false};

        bool ok = cb->invoke(
            modID ? 2 : 1,
            1,
            context,
            kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                if (c->modID) push(L, *c->modID);
                pushRequest(L, *c->request);
            },
            &ctx,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                c->stop = lua_toboolean(L, -1) != 0;
            },
            &ctx
        );
        return ok && ctx.stop;
    }

    bool invokeResponseEventNow(
        std::shared_ptr<LuaCallback> const& cb, char const* context,
        std::optional<std::string_view> modID, web::WebResponse const& response
    ) {
        if (!cb || !cb->valid()) return false;

        struct Ctx {
            std::optional<std::string_view> modID;
            web::WebResponse response;
            bool stop = false;
        } ctx{modID, response, false};

        bool ok = cb->invoke(
            modID ? 2 : 1,
            1,
            context,
            kHookScriptDeadlineMs,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                if (c->modID) push(L, *c->modID);
                pushResponse(L, std::move(c->response));
            },
            &ctx,
            +[](lua_State* L, void* raw) {
                auto* c = static_cast<Ctx*>(raw);
                c->stop = lua_toboolean(L, -1) != 0;
            },
            &ctx
        );
        return ok && ctx.stop;
    }

    bool invokeResponseEvent(
        std::shared_ptr<LuaCallback> cb, char const* context, std::optional<std::string_view> modID,
        web::WebResponse const& response
    ) {
        if (!cb || !cb->valid()) return false;
        if (Runtime::isMainThread()) {
            return invokeResponseEventNow(cb, context, modID, response);
        }
        std::optional<std::string> ownedModID;
        if (modID) ownedModID = std::string(*modID);
        geode::queueInMainThread(
            [cb = std::move(cb), context, ownedModID = std::move(ownedModID), response]() mutable {
                std::optional<std::string_view> view;
                if (ownedModID) view = std::string_view(*ownedModID);
                (void)invokeResponseEventNow(cb, context, view, response);
            }
        );
        return false;
    }

    void rememberListener(std::shared_ptr<WebListenerState> const& state) {
        activeListeners().push_back(state);
        compactWeakState();
        ensureShutdownHook();
    }

    int webOnRequestIntercept(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 1);
        auto state = std::make_shared<WebListenerState>();
        state->handle = web::WebRequestInterceptEvent().listen(
            [cb](std::string_view modID, web::WebRequest& req) {
                return invokeRequestEvent(cb, "geode.utils.web.onRequestIntercept", modID, req);
            },
            optPriority(L, 2)
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    int webOnRequestInterceptFor(lua_State* L) {
        auto modID = check<std::string>(L, 1, "geode.utils.web.onRequestInterceptFor");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 2);
        auto state = std::make_shared<WebListenerState>();
        state->handle = web::WebRequestInterceptEvent(modID).listen(
            [cb](web::WebRequest& req) {
                return invokeRequestEvent(
                    cb, "geode.utils.web.onRequestInterceptFor", std::nullopt, req
                );
            },
            optPriority(L, 3)
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    int webOnRequestInterceptById(lua_State* L) {
        auto id = static_cast<std::size_t>(
            checkNonNegativeInteger(L, 1, "geode.utils.web.onRequestInterceptById")
        );
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 2);
        auto state = std::make_shared<WebListenerState>();
        state->handle = web::IDBasedWebRequestInterceptEvent(id).listen(
            [cb](web::WebRequest& req) {
                return invokeRequestEvent(
                    cb, "geode.utils.web.onRequestInterceptById", std::nullopt, req
                );
            },
            optPriority(L, 3)
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    int webOnResponse(lua_State* L) {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 1);
        auto state = std::make_shared<WebListenerState>();
        state->handle = web::WebResponseEvent().listen(
            [cb](std::string_view modID, web::WebResponse const& response) {
                return invokeResponseEvent(cb, "geode.utils.web.onResponse", modID, response);
            },
            optPriority(L, 2)
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    int webOnResponseFor(lua_State* L) {
        auto modID = check<std::string>(L, 1, "geode.utils.web.onResponseFor");
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 2);
        auto state = std::make_shared<WebListenerState>();
        state->handle = web::WebResponseEvent(modID).listen(
            [cb](web::WebResponse const& response) {
                return invokeResponseEvent(cb, "geode.utils.web.onResponseFor", std::nullopt, response);
            },
            optPriority(L, 3)
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    int webOnResponseById(lua_State* L) {
        auto id =
            static_cast<std::size_t>(checkNonNegativeInteger(L, 1, "geode.utils.web.onResponseById"));
        luaL_checktype(L, 2, LUA_TFUNCTION);
        auto cb = std::make_shared<LuaCallback>(L, 2);
        auto state = std::make_shared<WebListenerState>();
        state->handle = web::IDBasedWebResponseEvent(id).listen(
            [cb](web::WebResponse const& response) {
                return invokeResponseEvent(
                    cb, "geode.utils.web.onResponseById", std::nullopt, response
                );
            },
            optPriority(L, 3)
        );
        rememberListener(state);
        pushListener(L, std::move(state));
        return 1;
    }

    void registerMethods(lua_State* L, char const* meta, luaL_Reg const* methods, lua_CFunction gc) {
        if (luaL_newmetatable(L, meta)) {
            luaL_register(L, nullptr, methods);
            lua_pushcfunction(L, gc, "__gc");
            lua_setfield(L, -2, "__gc");
            lua_pushvalue(L, -1);
            lua_setfield(L, -2, "__index");
            lua_pushstring(L, "locked");
            lua_setfield(L, -2, "__metatable");
        }
        lua_pop(L, 1);
    }

    void registerMetatables(lua_State* L) {
        luaL_Reg requestMethods[] = {
            {"header", requestHeader},
            {"removeHeader", requestRemoveHeader},
            {"param", requestParam},
            {"removeParam", requestRemoveParam},
            {"method", requestMethod},
            {"url", requestUrl},
            {"userAgent", requestUserAgent},
            {"acceptEncoding", requestAcceptEncoding},
            {"timeout", requestTimeout},
            {"downloadRange", requestDownloadRange},
            {"certVerification", requestCertVerification},
            {"transferBody", requestTransferBody},
            {"followRedirects", requestFollowRedirects},
            {"ignoreContentLength", requestIgnoreContentLength},
            {"caBundle", requestCaBundle},
            {"proxy", requestProxy},
            {"version", requestVersion},
            {"body", requestBody},
            {"bodyString", requestBodyString},
            {"bodyJson", requestBodyJson},
            {"bodyMultipart", requestBodyMultipart},
            {"onProgress", requestOnProgress},
            {"send", requestSend},
            {"get", requestGet},
            {"post", requestPost},
            {"put", requestPut},
            {"patch", requestPatch},
            {"getID", requestGetID},
            {"getMethod", requestGetMethod},
            {"getUrl", requestGetUrl},
            {"getHeaders", requestGetHeaders},
            {"getUrlParams", requestGetUrlParams},
            {"getBody", requestGetBody},
            {"getTimeout", requestGetTimeout},
            {"getVersion", requestGetVersion},
            {"getProgress", requestGetProgress},
            {nullptr, nullptr},
        };
        registerMethods(L, kRequestMeta, requestMethods, &requestGc);

        luaL_Reg responseMethods[] = {
            {"info", responseInfo},
            {"ok", responseOk},
            {"redirected", responseRedirected},
            {"badClient", responseBadClient},
            {"badServer", responseBadServer},
            {"error", responseError},
            {"cancelled", responseCancelled},
            {"code", responseCode},
            {"text", responseText},
            {"json", responseJson},
            {"bytes", responseBytes},
            {"saveTo", responseSaveTo},
            {"headers", responseHeaders},
            {"header", responseHeader},
            {"headersNamed", responseHeadersNamed},
            {"errorMessage", responseErrorMessage},
            {"verboseLogs", responseVerboseLogs},
            {"timings", responseTimings},
            {nullptr, nullptr},
        };
        registerMethods(L, kResponseMeta, responseMethods, &responseGc);

        luaL_Reg multipartMethods[] = {
            {"param", multipartParam},
            {"file", multipartFile},
            {"fileFrom", multipartFileFrom},
            {"getBoundary", multipartGetBoundary},
            {"getHeader", multipartGetHeader},
            {"getBody", multipartGetBody},
            {nullptr, nullptr},
        };
        registerMethods(L, kMultipartMeta, multipartMethods, &multipartGc);

        luaL_Reg handleMethods[] = {
            {"cancel", handleCancel},
            {"id", handleId},
            {nullptr, nullptr},
        };
        registerMethods(L, kHandleMeta, handleMethods, &handleGc);

        luaL_Reg listenerMethods[] = {
            {"disconnect", listenerDisconnect},
            {nullptr, nullptr},
        };
        registerMethods(L, kListenerMeta, listenerMethods, &listenerGc);
    }

    void registerConstants(lua_State* L) {
        lua_createtable(L, 0, 8);
        setIntField(L, "DEFAULT", static_cast<int>(web::HttpVersion::DEFAULT));
        setIntField(L, "VERSION_1_0", static_cast<int>(web::HttpVersion::VERSION_1_0));
        setIntField(L, "VERSION_1_1", static_cast<int>(web::HttpVersion::VERSION_1_1));
        setIntField(L, "VERSION_2_0", static_cast<int>(web::HttpVersion::VERSION_2_0));
        setIntField(L, "VERSION_2TLS", static_cast<int>(web::HttpVersion::VERSION_2TLS));
        setIntField(
            L, "VERSION_2_PRIOR_KNOWLEDGE", static_cast<int>(web::HttpVersion::VERSION_2_PRIOR_KNOWLEDGE)
        );
        setIntField(L, "VERSION_3", static_cast<int>(web::HttpVersion::VERSION_3));
        setIntField(L, "VERSION_3ONLY", static_cast<int>(web::HttpVersion::VERSION_3ONLY));
        lua_setfield(L, -2, "HttpVersion");

        lua_createtable(L, 0, 7);
        setIntField(L, "HTTP", static_cast<int>(web::ProxyType::HTTP));
        setIntField(L, "HTTPS", static_cast<int>(web::ProxyType::HTTPS));
        setIntField(L, "HTTPS2", static_cast<int>(web::ProxyType::HTTPS2));
        setIntField(L, "SOCKS4", static_cast<int>(web::ProxyType::SOCKS4));
        setIntField(L, "SOCKS4A", static_cast<int>(web::ProxyType::SOCKS4A));
        setIntField(L, "SOCKS5", static_cast<int>(web::ProxyType::SOCKS5));
        setIntField(L, "SOCKS5H", static_cast<int>(web::ProxyType::SOCKS5H));
        lua_setfield(L, -2, "ProxyType");

        lua_createtable(L, 0, 4);
        setIntField(
            L,
            "CURL_INITIALIZATION_ERROR",
            static_cast<int>(web::GeodeWebError::CURL_INITIALIZATION_ERROR)
        );
        setIntField(L, "REQUEST_CANCELLED", static_cast<int>(web::GeodeWebError::REQUEST_CANCELLED));
        setIntField(L, "QUEUE_FULL", static_cast<int>(web::GeodeWebError::QUEUE_FULL));
        setIntField(L, "CHANNEL_CLOSED", static_cast<int>(web::GeodeWebError::CHANNEL_CLOSED));
        lua_setfield(L, -2, "Error");

        lua_createtable(L, 0, 11);
        setLongField(L, "BASIC", web::http_auth::BASIC);
        setLongField(L, "DIGEST", web::http_auth::DIGEST);
        setLongField(L, "DIGEST_IE", web::http_auth::DIGEST_IE);
        setLongField(L, "BEARER", web::http_auth::BEARER);
        setLongField(L, "NEGOTIATE", web::http_auth::NEGOTIATE);
        setLongField(L, "NTLM", web::http_auth::NTLM);
        setLongField(L, "NTLM_WB", web::http_auth::NTLM_WB);
        setLongField(L, "ANY", web::http_auth::ANY);
        setLongField(L, "ANYSAFE", web::http_auth::ANYSAFE);
        setLongField(L, "ONLY", web::http_auth::ONLY);
        setLongField(L, "AWS_SIGV4", web::http_auth::AWS_SIGV4);
        lua_setfield(L, -2, "HttpAuth");
    }
} // namespace

namespace luax {
    geode::Result<void> registerGeodeWeb(lua_State* L) {
        registerMetatables(L);

        getOrCreateTable(L, "geode.utils.web");
        setTableCFunction(L, -1, "openLinkInBrowser", &webOpenLinkInBrowser);
        setTableCFunction(L, -1, "newRequest", &webNewRequest);
        setTableCFunction(L, -1, "multipart", &webMultipart);
        setTableCFunction(L, -1, "get", &webGet);
        setTableCFunction(L, -1, "post", &webPost);
        setTableCFunction(L, -1, "put", &webPut);
        setTableCFunction(L, -1, "patch", &webPatch);
        setTableCFunction(L, -1, "fetch", &webFetch);
        setTableCFunction(L, -1, "onRequestIntercept", &webOnRequestIntercept);
        setTableCFunction(L, -1, "onRequestInterceptFor", &webOnRequestInterceptFor);
        setTableCFunction(L, -1, "onRequestInterceptById", &webOnRequestInterceptById);
        setTableCFunction(L, -1, "onResponse", &webOnResponse);
        setTableCFunction(L, -1, "onResponseFor", &webOnResponseFor);
        setTableCFunction(L, -1, "onResponseById", &webOnResponseById);
        registerConstants(L);
        lua_pop(L, 1);
        return geode::Ok();
    }
} // namespace luax

#if !defined(LUAUAPI_HOST_TESTS)
LUAX_BINDING(geode_web_lib, registerGeodeWeb)
#endif
