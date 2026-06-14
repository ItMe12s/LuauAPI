local cocos = geode.cocos2d
local modId = geode.Mod.getID()

local handle = nil

local function helloWorld(self)
    local director = cocos.CCDirector.sharedDirector()
    if not director then return end
    local winSize = director:getWinSize()
    local label = cocos.CCLabelBMFont.create("Hello, World!", "bigFont.fnt")
    if not label then return end
    label:setID(modId .. "/hello-label")
    label:setPosition({ x = winSize.width / 2, y = winSize.height / 2 })
    self:addChild(label)
    if handle then
        handle:remove()
    end
end

handle = geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        helloWorld(self)
        return result
    end,
})