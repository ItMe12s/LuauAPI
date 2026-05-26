-- AsyncBootstrap.luau
-- Loaded with lua::runFileAsync.
-- File read and compile happen off-thread, then script executes on main thread.

local cocos2d = geode.cocos2d

local badgeHook = geode.hook("geode.gd.MenuLayer:init/0", function(self)
    local director = cocos2d.CCDirector.sharedDirector()
    if not director then return end

    local winSize = director:getWinSize()
    local label = cocos2d.CCLabelBMFont.create("Async Luau ready", "chatFont.fnt")
    if not label then return end

    label:setScale(0.52)
    label:setOpacity(0)
    label:setAnchorPoint({ x = 1, y = 0 })
    label:setPosition({ x = winSize.width - 10, y = 10 })

    local fade = cocos2d.CCFadeIn.create(0.35)
    if fade then
        label:runAction(fade)
    end

    self:addChild(label)
end)

print("Async script loaded:", badgeHook:isEnabled())
