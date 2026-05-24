-- Bootstrap.luau
-- Adds a player stats panel to the main menu.
-- Shows hooks, cocos2d UI, GD singletons, actions, and node hierarchy.
-- If you already know how to make Geode mods and Roblox games,
-- just follow good Luau coding practices.

-- geode.cocos2d has engine classes, geode.gd has game classes.
local cocos2d = geode.cocos2d
local gd = geode.gd

-- geode.hook(targetId, callback) registers a post-hook on a class method.
-- Target format: "geode.<namespace>.<Class>:<method>/<argCount>"
-- Returns a HookHandle with :enable(), :disable(), :remove(), :isEnabled().
local menuHook = geode.hook("geode.gd.MenuLayer:init/0", function(self)
    -- Singletons use .sharedDirector(), .get(), .sharedState(), etc.
    local director = cocos2d.CCDirector.sharedDirector()
    if not director then return end

    local winSize = director:getWinSize()

    -- GameManager.get() always returns a GameManager.
    -- GameStatsManager.sharedState() can return nil.
    local gm = gd.GameManager.get()
    local gmstats = gd.GameStatsManager.sharedState()
    if not gmstats then return end

    -- getStat(key) returns a number. Common stat keys:
    --   "1" = jumps, "2" = attempts, "3" = completed levels,
    --   "4" = online levels completed, "5" = demons, "6" = stars,
    --   "8" = coins, "12" = user coins, "13" = diamonds
    local stars = gmstats:getStat("6")
    local demons = gmstats:getStat("5")
    local coins = gmstats:getStat("13")

    -- CCScale9Sprite stretches a 9-slice sprite to any size, good for panels.
    local bg = cocos2d.CCScale9Sprite.create("square02_small.png")
    if not bg then return end

    -- Positions use { x = ..., y = ... }, sizes use { width = ..., height = ... }.
    bg:setContentSize({ width = 160, height = 80 })
    bg:setOpacity(140)
    bg:setAnchorPoint({ x = 0, y = 1 })
    bg:setPosition({ x = 10, y = winSize.height - 10 })

    -- CCLabelBMFont.create(text, fontFile) makes a bitmap font label.
    -- Common GD fonts: "bigFont.fnt", "chatFont.fnt", "goldFont.fnt"
    local title = cocos2d.CCLabelBMFont.create("Player Stats", "goldFont.fnt")
    if not title then return end
    title:setScale(0.5)
    title:setPosition({ x = 80, y = 62 })
    bg:addChild(title)

    local body = cocos2d.CCLabelBMFont.create(
        string.format("%d stars | %d demons | %d diamonds", stars, demons, coins),
        "chatFont.fnt"
    )
    if not body then return end
    body:setScale(0.6)
    body:setPosition({ x = 80, y = 35 })
    bg:addChild(body)

    -- colorForIdx(idx) returns { r, g, b } for a player color index.
    local playerColor = gm:colorForIdx(gm:getIntGameVariable("0001"))
    title:setColor(playerColor)

    -- Animate the panel sliding in.
    -- CCMoveTo.create(duration, targetPoint) moves to a position.
    -- CCEaseBackOut.create(action) adds overshoot easing.
    -- CCFadeIn.create(duration) fades opacity from 0 to full.
    -- CCSpawn runs two actions at the same time.
    bg:setOpacity(0)
    local targetY = winSize.height - 10
    bg:setPosition({ x = 10, y = targetY + 30 })

    local slideDown = cocos2d.CCMoveTo.create(0.5, { x = 10, y = targetY })
    if not slideDown then return end
    local easeSlide = cocos2d.CCEaseBackOut.create(slideDown)
    if not easeSlide then return end

    local fadeIn = cocos2d.CCFadeIn.create(0.4)
    if not fadeIn then return end

    local combined = cocos2d.CCSpawn.createWithTwoActions(easeSlide, fadeIn)
    if not combined then return end

    bg:runAction(combined)

    -- addChild(node) adds to the node tree. Optional 2nd arg is z-order.
    self:addChild(bg)
end)

-- HookHandle methods:
--   menuHook:isEnabled()  -> bool
--   menuHook:enable()     -> bool
--   menuHook:disable()    -> bool
--   menuHook:remove()     -> bool (permanent, can not re-enable after removal)
print("Bootstrap loaded, menu hook enabled:", menuHook:isEnabled())
