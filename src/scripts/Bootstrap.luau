local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local isDeveloperModeEnabled = geode.Mod.getSettingValue("enable-developer-mode")

geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self: MenuLayer, result: boolean?)
        task.defer(function()
            local parent = self:getParent(); if not parent then return end
            local crashedLayer = parent:getChildByID("FLAlertLayer"); if not crashedLayer then return end
            local mainLayer = parent:getChildByID("MainLayer"); if not mainLayer then return end
            local title: CCLabelBMFont? = mainLayer:getChildByID("title"); if not title then return end

            if crashedLayer then
                local luauCrashLabel = cc2d.CCLabelBMFont.create("Yo", "bigFont.fnt")
                if not luauCrashLabel then return end
                luauCrashLabel:setID(modId .. "/luau-crash-label")
                luauCrashLabel:setZOrder(999)
                luauCrashLabel:setAnchorPoint({ x = 0.5, y = 0.5 })
                luauCrashLabel:setPosition({ x = 0, y = 50 })
                crashedLayer:addChild(luauCrashLabel)
            end
        end)

        if not isDeveloperModeEnabled then return result end
        local label = cc2d.CCLabelBMFont.create("LuauAPI " .. geode.Mod.getVersion() .. " Developer Mode", "chatFont.fnt")
        if not label then return end
        label:setID(modId .. "/developer-mode-label")
        label:setZOrder(999)
        label:setOpacity(128)
        label:setScale(0.3)
        label:setAnchorPoint({ x = 0, y = 0 })
        label:setPosition({ x = 4, y = 4 })
        self:addChild(label)
        return result
    end,
})

if not isDeveloperModeEnabled then return end
warn("Developer Mode enabled! use with care :3")

local Executor = require("./Executor")
if geode.Mod.getSettingValue("enable-executor") then
    Executor.init()
end
