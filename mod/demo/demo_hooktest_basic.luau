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

local intKey = "luauapi_test_int"
gm:setIntGameVariable(intKey, 0)

local h1 = geode.hook("geode.gd.GameManager:setIntGameVariable/2", {
    before = function(self, key, value): any?
        if key == intKey then
            check("void before saw key", key == intKey)
            check("void before saw value", value == 5)
        end
        return nil
    end,
    after = function(self, key, value): any?
        if key == intKey then
            check("void after saw key", key == intKey)
            check("void after saw value", value == 5)
        end
        return nil
    end,
})

gm:setIntGameVariable(intKey, 5)
check("void original ran", gm:getIntGameVariable(intKey) == 5)
h1:remove()

local h2 = geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    before = function(self, key): any?
        if key == intKey then
            check("int before saw key", key == intKey)
        end
        return nil
    end,
    after = function(self, key, value)
        if key == intKey then
            check("int after saw key", key == intKey)
            check("int after saw value", value == 5)
        end
        return value
    end,
})

gm:setIntGameVariable(intKey, 5)
check("int original ran", gm:getIntGameVariable(intKey) == 5)
h2:remove()

local h3 = geode.hook("geode.gd.GameManager:getMenuMusicFile/0", {
    before = function(self): any?
        check("zero-arg before ran", self ~= nil)
        return nil
    end,
    after = function(self, value)
        check("zero-arg after saw string", type(value) == "string")
        return value
    end,
})

gm:getMenuMusicFile()
h3:remove()

gm:setIntGameVariable(intKey, 0)

if failures == 0 then
    print("ALL PASS")
else
    warn("" .. failures .. " FAILURES")
end