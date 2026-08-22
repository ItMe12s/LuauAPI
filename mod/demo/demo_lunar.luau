local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

if _G.lunarDemoStarted then
    print("lunar demo already running")
    return
end
_G.lunarDemoStarted = true

local bodySprite = modId .. "/TestSprite1.png"
local armSprite = modId .. "/TestSprite2.png"

local rig = lunar.rig.new():load({
    nodes = {
        { id = "body", sprite = bodySprite },
        { id = "arm", sprite = armSprite, parent = "body", x = 70, y = 40 },
    },
})
_G.lunarDemoRig = rig

local wave = lunar.animation.load({
    fps = 12,
    looped = true,
    keyframes = {
        [0] = {
            arm = { rot = -25 },
            body = { rot = 0, opacity = 255 },
        },
        [6] = {
            arm = { rot = 25, easing = "bounce_out" },
            body = { opacity = 180, easing = "expo_out" },
        },
        [18] = {
            arm = { rot = -25, easing = "sine_in_out" },
            body = { rot = -25, opacity = 255, easing = "sine_in_out" },
        },
        [24] = {
            arm = { rot = 25, easing = "sine_in" },
            body = { rot = 179, easing = "sine_in" }
        },
        [36] = {
            arm = { rot = -25, easing = "sine_out" },
            body = { rot = 0, opacity = 255, easing = "sine_out" },
        },
    },
})

geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        task.defer(function()
            local director = cc2d.CCDirector.sharedDirector()
            if not director then return end
            local winSize = director:getWinSize()

            rig:setPosition({ x = winSize.width / 2, y = winSize.height / 2 })
            rig:setZOrder(999)
            self:addChild(rig)

            local track = rig:loadAnimation(wave)
            track:play()
            _G.lunarDemoTrack = track

            print("lunar demo playing")
        end)
        return result
    end,
})

print("lunar demo loaded. enter the menu to see it :3")
