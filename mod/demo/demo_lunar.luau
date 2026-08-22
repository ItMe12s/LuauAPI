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
if not rig then
    print("lunar demo rig failed to load")
    return
end
rig:setID(modId .. "/lunar-demo-rig")
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
        [10] = { events = "epic event" },
        [18] = {
            arm = { rot = -25, easing = "sine_in_out" },
            body = { rot = -25, opacity = 255, easing = "sine_in_out" },
        },
        [26] = {
            events = { "bye" },
            arm = { rot = 25, easing = "sine_in" },
            body = { rot = 179, easing = "sine_in" }
        },
        [36] = {
            arm = { rot = -25, easing = "sine_out" },
            body = { rot = 0, opacity = 255, easing = "sine_out" },
        },
    },
})
if not wave then
    print("lunar demo animation failed to load")
    return
end

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
            if not track then return end
            track:play()
            _G.lunarDemoTrack = track

            local label = nil
            track:bindEvent("epic event", function(name)
                if label then return end
                label = geode.Label.create("hi from " .. name .. "!!!", "bigFont.fnt")
                if not label then return end
                label:setScale(0.5)
                label:setAnchorPoint({ x = 0.5, y = 0.5 })
                label:setPosition({ x = 0, y = 50 })
                rig:addChild(label)
            end)
            track:bindEvent("bye", function()
                if not label then return end
                label:removeFromParent()
                label = nil
            end)

            print("lunar demo playing")
        end)
        return result
    end,
})

print("lunar demo loaded. enter the menu to see it :3")
