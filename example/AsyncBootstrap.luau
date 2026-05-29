-- Loaded with runFileAsync.
-- VM execution still happens on main thread.

local cocos2d = geode.cocos2d

local badgeHook = geode.hook("geode.gd.MenuLayer:init/0", {
    after = function(self, result)
        local fields = geode.fields(self)
        fields.badges = (fields.badges or 0) + 1
        local origin = self.m_obPosition

        local director = cocos2d.CCDirector.sharedDirector()
        if not director then return result end

        local winSize = director:getWinSize()
        local label = cocos2d.CCLabelBMFont.create("Async Luau ready", "chatFont.fnt")
        if not label then return result end

        label:setScale(0.52)
        label:setOpacity(0)
        label:setAnchorPoint({ x = 1, y = 0 })
        label:setPosition({ x = winSize.width - 10 + origin.x, y = 10 + origin.y })

        local fade = cocos2d.CCFadeIn.create(0.35)
        if fade then
            label:runAction(fade)
        end

        self:addChild(label)
        return result
    end,
})

print("Async script loaded:", badgeHook:isEnabled())
