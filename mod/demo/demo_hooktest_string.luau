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

local strKey = "luauapi_test_str"

local h1 = geode.hook("geode.gd.GameManager:setGameVariable/2", {
    before = function(self, k, v): any?
        if k == strKey then
            return { args = { key = strKey .. "_override", value = true } }
        end
        return nil
    end,
})
gm:setGameVariable(strKey, false)
check("string override stored under new key", gm:getGameVariable(strKey .. "_override") == true)
h1:remove()

local fired = 0
local h2 = geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    before = function(self, k): any?
        if k == strKey then
            fired += 1
        end
        return nil
    end,
})

gm:getIntGameVariable(strKey)
check("hook fires while enabled", fired == 1)
check("handle isEnabled", h2:isEnabled() == true)

local ok = h2:disable()
check("disable ok", ok == true)
gm:getIntGameVariable(strKey)
check("hook silent while disabled", fired == 1)

ok = h2:enable()
check("enable ok", ok == true)
gm:getIntGameVariable(strKey)
check("hook fires after re-enable", fired == 2)

local removed = h2:remove()
check("remove ok", removed == true)
gm:getIntGameVariable(strKey)
check("hook gone after remove", fired == 2)

if failures == 0 then
    print("ALL PASS")
else
    warn("" .. failures .. " FAILURES")
end