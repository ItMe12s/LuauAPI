-- MainThreadBootstrap.luau
-- Loaded with lua::runFile on main thread.
-- Owns immediate main-thread UI hook setup.

local cocos2d = geode.cocos2d
local gd = geode.gd

local menuHook = geode.hook("geode.gd.MenuLayer:init/0", function(self)
    local director = cocos2d.CCDirector.sharedDirector()
    if not director then return end

    local winSize = director:getWinSize()
    local gm = gd.GameManager.get()
    local stats = gd.GameStatsManager.sharedState()
    if not stats then return end

    local stars = stats:getStat("6")
    local demons = stats:getStat("5")
    local diamonds = stats:getStat("13")

    local bg = cocos2d.CCScale9Sprite.create("square02_small.png")
    if not bg then return end

    bg:setContentSize({ width = 170, height = 84 })
    bg:setOpacity(140)
    bg:setAnchorPoint({ x = 0, y = 1 })
    bg:setPosition({ x = 10, y = winSize.height + 24 })

    local title = cocos2d.CCLabelBMFont.create("Main Thread Stats", "goldFont.fnt")
    if not title then return end
    title:setScale(0.46)
    title:setPosition({ x = 85, y = 65 })
    title:setColor(gm:colorForIdx(gm:getIntGameVariable("0001")))
    bg:addChild(title)

    local body = cocos2d.CCLabelBMFont.create(
        string.format("%d stars | %d demons | %d diamonds", stars, demons, diamonds),
        "chatFont.fnt"
    )
    if not body then return end
    body:setScale(0.56)
    body:setPosition({ x = 85, y = 37 })
    bg:addChild(body)

    local targetY = winSize.height - 10
    local slide = cocos2d.CCMoveTo.create(0.45, { x = 10, y = targetY })
    if not slide then return end
    local ease = cocos2d.CCEaseBackOut.create(slide)
    if not ease then return end
    bg:runAction(ease)

    self:addChild(bg)
end)

print("Main thread script loaded:", menuHook:isEnabled())
