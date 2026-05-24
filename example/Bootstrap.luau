-- Assuming you already know this (just finish volume 1) https://docs.geode-sdk.org/handbook/vol1/chap1_7/

local cocos2d = geode.cocos2d

local menuLayerInit = geode.hook("geode.gd.MenuLayer:init/0", function(self)
    local director = cocos2d.CCDirector.sharedDirector()
    if not director then return end

    local winSize = director:getWinSize()
    local label = cocos2d.CCLabelBMFont.create("Hello, World!", "bigFont.fnt")
    if not label then return end

    label:setPosition({ x = winSize.width / 2, y = winSize.height / 2 })
    self:addChild(label)
end)

print("Bootstrap.luau loaded", menuLayerInit:isEnabled())
