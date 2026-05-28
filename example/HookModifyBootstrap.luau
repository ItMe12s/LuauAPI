-- Centered menu label, FPS always on, level % always shown, auto-checkpoints locked on.

local cocos2d = geode.cocos2d

local AUTO_CHECKPOINTS_KEY = "0027" -- gv_0027 autoCheckpoints
local SHOW_FPS_KEY = "0115" -- gv_0115 Show FPS
local SHOW_PERCENTAGE_KEY = "0040" -- gv_0040 showPercentage

local rewroteAutoCheckpoints = false

local autoCheckpointHook = geode.modify("geode.gd.GameManager:setIntGameVariable/2", {
    priority = -10,
    before = function(self, key, value)
        if key == AUTO_CHECKPOINTS_KEY and value == 0 then
            rewroteAutoCheckpoints = true
            return { args = { key, 1 } }
        end
    end,
    after = function(self, key, value)
        if key == AUTO_CHECKPOINTS_KEY and rewroteAutoCheckpoints then
            rewroteAutoCheckpoints = false
            print("blocked auto-checkpoints disable, kept enabled")
        end
    end,
})

local fpsKeyHook = geode.hook("geode.gd.GameManager:getGameVariable/1", {
    before = function(self, key)
        if key == SHOW_FPS_KEY then
            return geode.skip(true)
        end
    end,
})

local percentageHook = geode.hook("geode.gd.GameManager:getIntGameVariable/1", {
    after = function(self, key, value)
        if key == SHOW_PERCENTAGE_KEY then
            return 1
        end
        return value
    end,
})

-- https://docs.geode-sdk.org/handbook/vol1/chap1_7
local menuHook = geode.modify("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        if not result then return result end

        local director = cocos2d.CCDirector.sharedDirector()
        if not director then return result end

        local winSize = director:getWinSize()
        local label = cocos2d.CCLabelBMFont.create("Hello, World!", "bigFont.fnt")
        if not label then return result end

        label:setPosition({ x = winSize.width / 2, y = winSize.height / 2 })
        self:addChild(label)

        return result
    end,
})

print(
    "Modify hooks loaded:",
    autoCheckpointHook:isEnabled(),
    fpsKeyHook:isEnabled(),
    percentageHook:isEnabled(),
    menuHook:isEnabled()
)
