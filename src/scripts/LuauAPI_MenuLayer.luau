local LuauAPIMenuLayer = {}

local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

function LuauAPIMenuLayer.init()
    local isDeveloperModeEnabled = geode.Mod.getSettingValue("enable-developer-mode")

    geode.hook("geode.gd.MenuLayer:init/0", {
        after = function(self: MenuLayer, result: boolean)
            task.defer(function()
                local parent = self:getParent(); if not parent then return end
                local crashedLayer = parent:getChildByID("FLAlertLayer"); if not crashedLayer then return end
                local mainLayer = crashedLayer:getChildByID("main-layer"); if not mainLayer then return end
                local title = mainLayer:getChildByID("title") :: CCLabelBMFont?; if not title then return end
                local titleString = title:getString(); if not titleString then return end
                if titleString ~= "Crashed" then return end

                local director = cc2d.CCDirector.sharedDirector(); if not director then return end
                local winSize = director:getWinSize();
                local crashHint = if geode.Mod.getSettingValue("enable-crash-sidecar")
                    then "luauapi-last-context.txt was overwritten on restart!"
                    else 'Turn on "Enable Crash Context File" in LuauAPI settings to save Luau crash context.'
                local luauCrashLabel = cc2d.CCLabelBMFont.create(crashHint, "chatFont.fnt")
                if not luauCrashLabel then return end
                luauCrashLabel:setID(modId .. "/luau-crash-label")
                luauCrashLabel:setZOrder(999)
                luauCrashLabel:setScale(0.67)
                luauCrashLabel:setAnchorPoint({ x = 0.5, y = 0.5 })
                luauCrashLabel:setPosition({ x = winSize.width / 2, y = 80 })
                crashedLayer:addChild(luauCrashLabel)
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
end

return LuauAPIMenuLayer
