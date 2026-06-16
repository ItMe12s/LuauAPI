local isDeveloperModeEnabled = geode.Mod.getSettingValue("enable-developer-mode")
if not isDeveloperModeEnabled then return end

geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        local label = geode.cocos2d.CCLabelBMFont.create("LuauAPI " .. geode.Mod.getVersion() .. " Developer Mode", "chatFont.fnt")
        if not label then return end
        label:setID(geode.Mod.getID() .. "/developer-mode-label")
        label:setZOrder(999)
        label:setOpacity(128)
        label:setScale(0.3)
        label:setAnchorPoint({ x = 0, y = 0 })
        label:setPosition({ x = 4, y = 4 })
        self:addChild(label)
        return result
    end,
})

local Executor = require("./Executor")
if geode.Mod.getSettingValue("enable-executor") then
    Executor.init()
end
