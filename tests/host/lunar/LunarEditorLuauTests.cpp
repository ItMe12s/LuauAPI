#include "lua_test_helpers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>

namespace {

    std::optional<std::string> readModuleFile(char const* name) {
        std::ifstream in(std::filesystem::path(LEDIT_SOURCE_DIR) / name, std::ios::binary);
        if (!in) {
            return std::nullopt;
        }
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    void loadChunk(lua_State* L, std::string const& source, char const* chunk) {
        auto bytecode = luauapi_test::compile(source);
        REQUIRE(luau_load(L, chunk, bytecode.data(), bytecode.size(), 0) == 0);
        REQUIRE(lua_pcall(L, 0, 1, 0) == 0);
    }

    int editorLoadstring(lua_State* L) {
        size_t len = 0;
        char const* src = luaL_checklstring(L, 1, &len);
        auto bytecode = luax::Runtime::compileSource(std::string_view(src, len));
        if (luau_load(L, "=loadstring", bytecode.data(), bytecode.size(), 0) != 0) {
            char const* msg = lua_tostring(L, -1);
            luaL_error(L, "%s", msg ? msg : "loadstring compile failed");
        }
        return 1;
    }

    extern char const* kFakes;

    struct EditorEnv {
        luauapi_test::LuaStatePtr L;

        EditorEnv() : L(luauapi_test::makeLuaState(true)) {
            lua_State* l = L.get();
            lua_pushcfunction(l, &editorLoadstring, "loadstring");
            lua_setglobal(l, "loadstring");
            REQUIRE(luauapi_test::runScriptVoid(l, R"X(
__MODULES = {}
function require(path)
    local mod = __MODULES[path:match("([%w_]+)$")]
    if mod == nil then error("module not loaded: " .. path) end
    return mod
end
if loadstring == nil and load ~= nil then
    function loadstring(src, chunk)
        return load(src, chunk or "=loadstring")
    end
end
)X"));
            for (char const* name :
                 {"leditb_Ser",
                  "leditb_Undo",
                  "leditb_DocCore",
                  "leditb_DocRig",
                  "leditb_DocDefs",
                  "leditb_Doc",
                  "leditb_Project"}) {
                auto source = readModuleFile((std::string(name) + ".luau").c_str());
                REQUIRE(source.has_value());
                std::string chunk = std::string("@") + name + ".luau";
                loadChunk(l, *source, chunk.c_str());
                lua_getglobal(l, "__MODULES");
                REQUIRE(lua_istable(l, -1));
                lua_pushvalue(l, -2);
                lua_setfield(l, -2, name);
                lua_pop(l, 2);
            }
            for (char const* name :
                 {"leditf_State",
                  "leditf_App",
                  "leditf_MenuBtn",
                  "leditf_Browser",
                  "leditf_RigTree",
                  "leditf_Anims",
                  "leditf_Props",
                  "leditf_Timeline"}) {
                auto source = readModuleFile((std::string(name) + ".luau").c_str());
                REQUIRE(source.has_value());
                auto bytecode = luauapi_test::compile(*source);
                std::string chunk = std::string("@") + name + ".luau";
                INFO(name);
                if (luau_load(l, chunk.c_str(), bytecode.data(), bytecode.size(), 0) != 0) {
                    char const* msg = lua_tostring(l, -1);
                    FAIL(std::string(name) + ": " + (msg ? msg : "unknown"));
                    lua_pop(l, 1);
                }
                lua_pop(l, 1);
            }
        }

        bool run(std::string const& body) const {
            auto bytecode = luauapi_test::compile(kFakes + body);
            if (luau_load(L.get(), "@editor-test", bytecode.data(), bytecode.size(), 0) != 0) {
                FAIL("luau_load: " << (lua_tostring(L.get(), -1) ? lua_tostring(L.get(), -1) : "?"));
                return false;
            }
            if (lua_pcall(L.get(), 0, 1, 0) != 0) {
                FAIL("script error: " << (lua_tostring(L.get(), -1) ? lua_tostring(L.get(), -1) : "?"));
                return false;
            }
            bool const ok = lua_toboolean(L.get(), -1) != 0;
            lua_pop(L.get(), 1);
            return ok;
        }
    };

    char const* kFakes = R"X(
local function makeFakeLunar()
    local state = { failAnim = false }
    local Rig, Track = {}, {}
    Rig.__index = Rig
    Track.__index = Track
    function Rig:load(spec)
        local seen = {}
        for _, n in ipairs(spec.nodes or {}) do
            if seen[n.id] ~= nil then return nil, "duplicate id" end
            seen[n.id] = true
        end
        local orderSeen = {}
        for _, n in ipairs(spec.nodes or {}) do
            if n.parent ~= nil and orderSeen[n.parent] ~= true then
                return nil, "parent must precede child"
            end
            orderSeen[n.id] = true
        end
        self.spec = spec
        return self
    end
    function Rig:loadAnimation(def)
        if state.failAnim then return nil, "forced failure" end
        local t = setmetatable({ def = def, playing = false, paused = false, t = 0, speed = 1 }, Track)
        local fps = def.fps or 30
        local maxChF, maxEvT = 0, 0
        for f, kf in pairs(def.keyframes or {}) do
            local hasCh = false
            for id, pose in pairs(kf) do
                if id ~= "events" then
                    for k in pairs(pose) do
                        if k ~= "easing" then hasCh = true end
                    end
                end
            end
            if hasCh and f > maxChF then maxChF = f end
            if kf.events ~= nil and f / fps > maxEvT then maxEvT = f / fps end
        end
        t.dur = math.max(maxChF / fps, maxEvT)
        return t
    end
    function Track:isPlaying() return self.playing end
    function Track:setSpeed(s) self.speed = s end
    function Track:duration() return self.dur end
    function Track:seek(t) self.t = math.clamp(t, 0, self.dur) end
    function Track:currentTime() return self.t end
    function Track:play() self.playing = true self.paused = false end
    function Track:pause() self.paused = true end
    function Track:unpause() self.paused = false end
    function Track:stop() self.playing = false self.t = 0 end
    return {
        rig = { new = function() return setmetatable({}, Rig) end },
        animation = {
            load = function(def)
                if state.failAnim then return nil, "forced failure" end
                if type(def.fps) == "number" and def.fps <= 0 then return nil, "bad fps" end
                return def
            end,
        },
    }, state
end

local function parentOf(path)
    return path:match("^(.*)/") or ""
end

local function makeFakeFs()
    local files, dirs = {}, { [""] = true }
    local function full(root, path)
        return root .. "/" .. path
    end
    return {
        files = files,
        read = function(root, path)
            local data = files[full(root, path)]
            if data == nil then return nil, "no such file" end
            return data
        end,
        write = function(root, path, data)
            local target = full(root, path)
            if dirs[parentOf(target)] ~= true then return nil, "missing directory" end
            files[target] = data
            return true
        end,
        exists = function(root, path)
            local target = full(root, path)
            return files[target] ~= nil or dirs[target] == true
        end,
        list = function(root, path)
            local base = full(root, path)
            local out = {}
            for name in pairs(files) do
                if parentOf(name) == base then out[#out + 1] = name:sub(#base + 2) end
            end
            for name in pairs(dirs) do
                if name ~= "" and parentOf(name) == base then out[#out + 1] = name:sub(#base + 2) end
            end
            return out
        end,
        mkdir = function(root, path)
            dirs[full(root, path)] = true
            return true
        end,
        remove = function(root, path)
            local target = full(root, path)
            if files[target] ~= nil then
                files[target] = nil
            elseif dirs[target] == true then
                dirs[target] = nil
            end
            return true
        end,
    }
end
)X";

} // namespace

TEST_CASE("Ser emits deterministic, loadstring-round-trippable animations") {
    EditorEnv env;
    REQUIRE(env.run(std::string(kFakes) + R"X(
local Ser = require("./leditb_Ser")
local def = {
    looped = true,
    fps = 12,
    keyframes = {
        [10] = { events = "late", arm = { rot = 90.5 } },
        [0] = { body = { x = 1, easing = "expo_out" }, events = { "a", "b" } },
    },
}
local a = Ser.anim(def)
local b = Ser.anim({ keyframes = def.keyframes, fps = 12, looped = true })
assert(a == b, "insertion order must not matter")
local back = assert(loadstring(a))()
assert(back.fps == 12 and back.looped == true)
assert(back.keyframes[0].body.x == 1 and back.keyframes[0].body.easing == "expo_out")
assert(back.keyframes[0].events[1] == "a" and back.keyframes[0].events[2] == "b")
assert(type(back.keyframes[10].events) == "table" and back.keyframes[10].events[1] == "late")
assert(back.keyframes[10].arm.rot == 90.5)
assert(Ser.anim(back) == a, "round trip must be byte-stable")
return true
)X"));
}

TEST_CASE("Ser escapes strings, formats numbers, rejects non-finite values") {
    EditorEnv env;
    REQUIRE(env.run(std::string(kFakes) + R"X(
local Ser = require("./leditb_Ser")
local tricky = 'he said "hi"\nback\\slash\ttab'
local back = assert(loadstring("return " .. Ser.value(tricky)))()
assert(back == tricky)
assert(Ser.value(0.1) == "0.1")
assert(Ser.value(-0) == "-0" or Ser.value(-0) == "0")
assert(Ser.value(1e300) == "1e+300")
assert(Ser.value(true) == "true" and Ser.value(nil) == "nil")
assert(not pcall(Ser.value, 0 / 0))
assert(not pcall(Ser.value, math.huge))
assert(not pcall(Ser.anim, { fps = 0 / 0 }))
assert(not pcall(Ser.anim, { keyframes = { [-1] = {} } }))
local specSrc = Ser.spec({
    nodes = {
        { id = "arm", parent = "body", x = 50 },
        { id = "body", sprite = "a/b.png", rot = -25 },
    },
})
local spec = assert(loadstring(specSrc))()
assert(spec.nodes[1].id == "arm" and spec.nodes[1].x == 50)
assert(spec.nodes[2].sprite == "a/b.png" and spec.nodes[2].rot == -25)
assert(not pcall(Ser.spec, { nodes = { { x = 1 } } }), "node id required")
return true
)X"));
}

TEST_CASE("Undo coalesces drags and caps depth") {
    EditorEnv env;
    REQUIRE(env.run(std::string(kFakes) + R"X(
local Undo = require("./leditb_Undo")
local u = Undo.new()
local v = 0
u:push("k1", function() v = v - 1 end, function() v = v + 1 end)
u:push(nil, function() v = v - 5 end, function() v = v + 5 end)
assert(v == 0)
assert(u:canUndo() and not u:canRedo())
assert(u:undo() and v == -5)
assert(u:undo() and v == -6)
assert(not u:undo())
assert(u:canRedo())
assert(u:redo() and v == -5)
assert(u:redo() and v == 0)
assert(not u:redo())

local applied = 0
u:push("drag", function() applied = applied - 1 end, function() applied = applied + 1 end)
u:push("drag", function() applied = applied - 1 end, function() applied = applied + 1 end)
u:push("drag", function() applied = applied - 1 end, function() applied = applied + 3 end)
assert(u:undo() and applied == -1, "one drag = one command, original undo kept")
assert(u:redo() and applied == 2, "latest redo wins")

local capped = Undo.new()
local total = 0
for _ = 1, 260 do
    capped:push(nil, function() total = total - 1 end, function() total = total + 1 end)
end
for _ = 1, 260 do
    capped:undo()
end
assert(total == -256, "depth cap keeps the newest 256")
capped:clear()
assert(not capped:canUndo() and not capped:canRedo())
return true
)X"));
}

TEST_CASE("Doc node ops: uniqueness, rename cross-update, cycles, orphans") {
    EditorEnv env;
    REQUIRE(env.run(std::string(kFakes) + R"X(
local Doc = require("./leditb_Doc")
local fl = makeFakeLunar()
local d = Doc.new({ lunar = fl })
assert(d:addNode({ id = "body", sprite = "b.png" }))
assert(d:addNode({ id = "arm", parent = "body", x = 50 }))
assert(not d:addNode({ id = "arm" }), "duplicate id rejected")
assert(not d:addNode({}), "missing id rejected")
assert(d:addAnim("wave") and d:setActive("wave"))

assert(d:renameNode("arm", "claw"))
assert(d.rigSpec.nodes[2].id == "claw" and d.rigSpec.nodes[2].parent == "body")
assert(d:putPose(0, "claw", { rot = 5 }))
assert(d:renameNode("claw", "grip"))
assert(d.animations.wave.keyframes[0].grip ~= nil)
assert(d.animations.wave.keyframes[0].claw == nil, "old pose key gone")

assert(not d:reparent("body", "grip"), "cycle rejected")
assert(d:reparent("grip", nil))

assert(d:putPose(3, "grip", { y = 7 }))
assert(d:deleteNode("grip"))
assert(d.animations.wave.keyframes[3].grip.y == 7, "orphan poses are data-preserving")

assert(not d:setBase("ghost", { x = 1 }), "unknown node rejected")
assert(not d:setBase("body", { nope = 1 }), "unknown channel rejected")
assert(not d:setBase("body", { x = 0 / 0 }), "non-finite rejected")
assert(d:setBase("body", { x = 10, z = 2 }))
assert(d.rigSpec.nodes[1].x == 10 and d.rigSpec.nodes[1].z == 2)

assert(d.lunar == fl)
return true
)X"));
}

TEST_CASE("Doc rebuild tolerates out-of-order parents and parent deletion") {
    EditorEnv env;
    REQUIRE(env.run(std::string(kFakes) + R"X(
local Doc = require("./leditb_Doc")
local d = Doc.new({ lunar = makeFakeLunar() })
d:addAnim("w")
d:setActive("w")

d:addNode({ id = "body" })
d:addNode({ id = "arm" })
d:addNode({ id = "hand" })
assert(d:reparent("arm", "body"))
assert(d:reparent("hand", "arm"))
assert(d.rigSpec.nodes[1].id == "body", "authored order untouched")
assert(d.rig.spec.nodes[3].id == "hand", "loaded rig is topologically ordered")

assert(d:addNode({ id = "thumb", parent = "hand" }))
assert(d.rig ~= nil, "parent-before-child invariant not required by callers")
assert(d:deleteNode("hand"))
assert(d.rigSpec.nodes[3].parent == "arm", "deleted parent's children reparented")
assert(d.rig ~= nil, "rebuild succeeds after parent deletion")
return true
)X"));
}

TEST_CASE("Doc def ops: merge, event-only frames, move semantics, rename") {
    EditorEnv env;
    REQUIRE(env.run(std::string(kFakes) + R"X(
local Doc = require("./leditb_Doc")
local d = Doc.new({ lunar = makeFakeLunar() })
d:addNode({ id = "arm" })
d:addAnim("wave")
assert(d:setActive("wave"))

assert(d:putPose(0, "arm", { rot = -25 }))
assert(d:putPose(0, "arm", { rot = -25, ay = 0.5 }), "merge channels")
local pose = d.animations.wave.keyframes[0].arm
assert(pose.rot == -25 and pose.ay == 0.5)
assert(d:putPose(0, "arm", { rot = 10, easing = "expo_out" }))
pose = d.animations.wave.keyframes[0].arm
assert(pose.rot == 10 and pose.ay == 0.5 and pose.easing == "expo_out")
assert(not d:putPose(-1, "arm", {}), "negative frame rejected")
assert(not d:putPose(0, "arm", { nope = 1 }), "unknown channel rejected")

assert(d:putPose(6, "arm", { x = 1 }))
assert(d:addEvent(6, "hit"))
assert(d:addEvent(6, "hit"), "duplicate event tolerated")
assert(d:clearPose(6, "arm"))
assert(d.animations.wave.keyframes[6] ~= nil, "event-only frame holds duration")
assert(d.animations.wave.keyframes[6].events[1] == "hit")
assert(d:removeEvent(6, "hit"))
assert(d.animations.wave.keyframes[6] == nil, "fully empty frame pruned")

d:addEvent(2, "early")
assert(d:putPose(5, "arm", { x = 50 }))
assert(d:moveFrame(5, 2))
local merged = d.animations.wave.keyframes[2]
assert(merged.arm ~= nil and merged.arm.x == 50, "moved poses win")
assert(merged.events[1] == "early" and #merged.events == 1, "events append deduped")
assert(d.animations.wave.keyframes[5] == nil)
assert(not d:moveFrame(9, 1), "missing source rejected")

assert(d:addEvent(4, "alpha"))
assert(d:renameEvent("alpha", "beta"))
assert(d.animations.wave.keyframes[4].events[1] == "beta")

assert(d:length() == 4 / 30)
assert(d:setFps(60) and d:length() == 4 / 60)
assert(not d:setFps(0))
assert(d:setLooped(true) and d.animations.wave.looped == true)

d:addAnim("copy-me")
assert(d:duplicateAnim("wave", "wave2"))
assert(d.animations.wave2.keyframes[0].arm.rot == 10)
assert(not d:duplicateAnim("wave", "wave2"))
assert(d:renameAnim("wave2", "wave3") and d.animations.wave3 ~= nil)
assert(d:deleteAnim("wave3"))
return true
)X"));
}

TEST_CASE("Doc playback: recompile preserves playhead, failures keep old track") {
    EditorEnv env;
    REQUIRE(env.run(std::string(kFakes) + R"X(
local Doc = require("./leditb_Doc")
local fl, state = makeFakeLunar()
local d = Doc.new({ lunar = fl })
assert(d:addAnim("wave"))
assert(d:setActive("wave"))
assert(d:putPose(0, "arm", { x = 0 }))
assert(d:putPose(20, "arm", { x = 100 }))
assert(d:play())
d.track.t = 0.1
d.track.playing = true
assert(d:scrubToTime(99))
local dur = 20 / 30
assert(math.abs(d.playhead - dur) < 1e-9, "scrub clamps to duration")
assert(d:putPose(21, "arm", { x = 200 }), "edit while playing")
assert(d.track:isPlaying(), "recompile relaunches playing tracks")
assert(math.abs(d.playhead - dur) < 1e-9, "playhead preserved across recompile")
assert(not d:putPose(0, "arm", { x = 0 / 0 }), "non-finite rejected pre-recompile")

state.failAnim = true
assert(not d:removeFrame(0), "validation failure surfaces error")
assert(d.track ~= nil and d.track.def.keyframes[21] ~= nil, "old track kept")
state.failAnim = false

assert(d:pause() and d:unpause())
assert(d:setSpeed(2) and d.track.speed == 2)
assert(d:stop() and d.playhead == 0)

d:addAnim("other")
assert(d:setActive("other"))
assert(d.playhead == 0 and d.track ~= nil)

assert(d:putPose(5, "arm", {}), "empty skeleton key ok")
assert(d.track:duration() == 0, "skeleton-only track has zero duration")
assert(d:scrubToTime(5 / 30))
assert(d.playhead == 5 / 30, "empty track keeps scrub playhead")
assert(d:putPose(5, "arm", { x = 1 }), "autokey over skeleton")
assert(math.abs(d.playhead - 5 / 30) < 1e-9, "playhead survives recompile to real key")
return true
)X"));
}

TEST_CASE("Doc records undo commands automatically") {
    EditorEnv env;
    REQUIRE(env.run(std::string(kFakes) + R"X(
local Doc = require("./leditb_Doc")
local d = Doc.new({ lunar = makeFakeLunar() })

assert(not d:putPose(0, "arm", { x = 1 }), "no active anim rejected")
assert(not d.undoStack:canUndo(), "failed op records nothing")

d:addNode({ id = "arm" })
d:addAnim("wave")
d:setActive("wave")
assert(d:putPose(0, "arm", { x = 1 }, "drag:x"))
assert(d:putPose(0, "arm", { x = 2 }, "drag:x"))
assert(d:putPose(0, "arm", { x = 3 }, "drag:x"))
assert(d.animations.wave.keyframes[0].arm.x == 3)

assert(d:undo())
assert(d.animations.wave.keyframes[0] == nil, "one drag = one command, fully undone")
assert(d:undo() and d.activeAnim == nil, "setActive undone")
assert(d:undo() and d.animations.wave == nil, "addAnim undone")
assert(d:undo() and #d.rigSpec.nodes == 0, "addNode undone")
assert(not d:undo(), "stack empty")

assert(d:redo() and #d.rigSpec.nodes == 1)
assert(d:redo() and d.animations.wave ~= nil)
assert(d:redo() and d.activeAnim == "wave")
assert(d:redo() and d.animations.wave.keyframes[0].arm.x == 3, "redo restores latest drag")

local e = Doc.new({ lunar = makeFakeLunar() })
e:addNode({ id = "a" })
e:addAnim("w")
e:setActive("w")
e:putPose(2, "a", { x = 9 })
e:putPose(4, "a", { y = 1 })
assert(e:deleteAnim("w"))
assert(e.animations.w == nil and e.track == nil)
assert(e:undo())
assert(e.activeAnim == "w" and e.track ~= nil, "deleteAnim undo rebuilds track")
assert(e.animations.w.keyframes[2].a.x == 9)

assert(e:renameNode("a", "b"))
assert(e.animations.w.keyframes[4].b ~= nil)
assert(e:undo())
assert(e.animations.w.keyframes[4].a ~= nil, "rename remap undone")
assert(e:redo() and e.animations.w.keyframes[4].b ~= nil, "rename redone")
return true
)X"));
}

TEST_CASE("Project IO: create, save, reload byte-stable, stale sweep") {
    EditorEnv env;
    REQUIRE(env.run(std::string(kFakes) + R"X(
local Project = require("./leditb_Project")
local fs = makeFakeFs()
local pj = Project.new({ fs = fs, lunar = makeFakeLunar() })

assert(pj:create("demo rig"))
assert(pj:list()[1] == "demo rig")
assert(not pj:create("demo rig"), "duplicate project rejected")
assert(not pj:create("../evil"), "path traversal rejected")
assert(not pj:load("nope"), "missing project rejected")

local doc = assert(pj:load("demo rig"))
doc.name = "demo rig"
assert(doc.rigSpec.nodes ~= nil)
assert(doc:addNode({ id = "root", sprite = "s.png" }))
assert(doc:addAnim("walk"))
assert(doc:setActive("walk"))
doc.animations.walk.keyframes[4] = { root = { x = 12.5 } }
doc.animations.walk.keyframes[0] = { events = "start" }
assert(pj:save(doc))

local walkPath = "save/projects/demo rig/walk.anim.luau"
local before = fs.files[walkPath]
assert(before ~= nil)
assert(fs.files["save/projects/demo rig/demo rig.rig.luau"] ~= nil)

local doc2 = assert(pj:load("demo rig"))
assert(doc2.name == "demo rig")
assert(doc2.rigSpec.nodes[1].id == "root")
assert(doc2.animations.walk.keyframes[4].root.x == 12.5)
assert(doc2.animations.walk.keyframes[0].events[1] == "start")
assert(pj:save(doc2))
assert(fs.files[walkPath] == before, "byte-stable resave")

doc2.animations.walk = nil
assert(pj:save(doc2))
assert(fs.files[walkPath] == nil, "stale anim files swept")

assert(pj:saveAs(doc2, "demo two"))
assert(fs.files["save/projects/demo two/demo two.rig.luau"] ~= nil, "save-as copies under new name")
assert(doc2.name == "demo two")
assert(pj:saveAs(doc2, "demo two"), "save-as onto own name = plain save")
assert(not pj:saveAs(doc2, "demo rig"), "save-as onto existing project rejected")

local doc3 = assert(pj:load("demo two"))
assert(doc3.rigSpec.nodes[1].id == "root")

assert(pj:delete("demo two"))
assert(fs.files["save/projects/demo two/demo two.rig.luau"] == nil, "delete removes files")
assert(pj:load("demo two") == nil, "deleted project unlistable")
assert(pj:delete("demo two"), "delete idempotent")
return true
)X"));
}
