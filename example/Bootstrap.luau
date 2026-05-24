-- Assuming you already know this (just finish volume 1) https://docs.geode-sdk.org/handbook/vol1/chap1_7/

local menuLayerInit = geode.hook("geode.gd.MenuLayer:init/0", function(ctx)
    if not ctx.result then return false end

    local director = geode.cocos2d.CCDirector.sharedDirector()
    if not director then return ctx.result end

    local winSize = director:getWinSize()
    local label = geode.cocos2d.CCLabelBMFont.create("Hello, World!", "bigFont.fnt")
    if not label then return ctx.result end

    label:setPosition({ x = winSize.width / 2, y = winSize.height / 2 })
    ctx.self:addChild(label)

    return true
end)

print("Bootstrap.luau loaded", menuLayerInit:isEnabled())
