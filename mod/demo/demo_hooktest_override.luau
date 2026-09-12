local gd = geode.gd
local gm = gd.GameManager.get(); assert(gm)

local failures = 0
local function check(name, cond)
    if cond then
        print("PASS " .. name)
    else
        failures += 1
        warn("FAIL " .. name)
    end
end

local key = "luauapi_test_ovr"
gm:setIntGameVariable(key, 1)

local h1 = geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    before = function(self, k): any?
        if k == key then
            return { args = { "luauapi_test_ovr2" } }
        end
        return nil
    end,
    after = function(self, k, v)
        return v
    end,
})
gm:setIntGameVariable("luauapi_test_ovr2", 99)
check("positional override read other key", gm:getIntGameVariable(key) == 99)
h1:remove()

local h2 = geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    before = function(self, k, v): any?
        if k == key then
            return { args = { key = "luauapi_test_named", value = 42 } }
        end
        return nil
    end,
})
gm:setIntGameVariable(key, 0)
check("named override wrote other key", gm:getIntGameVariable("luauapi_test_named") == 42)
h2:remove()

local h3 = geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    after = function(self, k, v)
        if k == key then
            return 1234
        end
        return v
    end,
})
check("after return override", gm:getIntGameVariable(key) == 1234)
h3:remove()

local originalRan = false
local h4 = geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    before = function(self, k, v): any?
        if k == key then
            return geode.skip()
        end
        return nil
    end,
    after = function(self, k, v): any?
        if k == key then
            originalRan = true
        end
        return nil
    end,
})
gm:setIntGameVariable(key, 7)
check("skip suppressed original", gm:getIntGameVariable(key) ~= 7)
check("skip suppressed after", not originalRan)
h4:remove()

local h5 = geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    before = function(self, k): any?
        if k == key then
            return geode.skip(777)
        end
        return nil
    end,
})
check("skip with value", gm:getIntGameVariable(key) == 777)
h5:remove()

gm:setIntGameVariable(key, 0)
gm:setIntGameVariable("luauapi_test_ovr2", 0)
gm:setIntGameVariable("luauapi_test_named", 0)

if failures == 0 then
    print("ALL PASS")
else
    warn("" .. failures .. " FAILURES")
end