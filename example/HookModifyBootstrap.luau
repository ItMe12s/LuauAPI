-- HookModifyBootstrap.luau
-- Modify-style hook examples using hook targets generated for GD methods.

local setIntHook = geode.modify("geode.gd.GameManager:setIntGameVariable/2", {
    priority = -10,
    before = function(self, key, value)
        if key == "luauapi.demo.count" then
            return {
                args = {
                    "luauapi.demo.count.redirected",
                    value + 1,
                },
            }
        end
    end,
    after = function(self, key, value)
        if key == "luauapi.demo.count.redirected" then
            print("redirected int variable", value)
        end
    end,
})

local skipHook = geode.hook("geode.gd.GameManager:getGameVariable/1", {
    before = function(self, key)
        if key == "luauapi.demo.alwaysTrue" then
            return geode.skip(true)
        end
    end,
    after = function(self, key, result)
        if key == "luauapi.demo.alwaysTrue" then
            print("skip result still reached after hook", result)
        end
        return result
    end,
})

local intOverrideHook = geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    after = function(self, key, value)
        if key == "luauapi.demo.answer" then
            return 42
        end
        return value
    end,
})

local menuHook = geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        self.m_fields.modifyMenus = (self.m_fields.modifyMenus or 0) + 1
        local pos = self.m_obPosition
        print("menu m_fields", self.m_fields.modifyMenus, pos.x, pos.y)
        return result
    end,
})

print(
    "Modify hooks loaded:",
    setIntHook:isEnabled(),
    skipHook:isEnabled(),
    intOverrideHook:isEnabled(),
    menuHook:isEnabled()
)
