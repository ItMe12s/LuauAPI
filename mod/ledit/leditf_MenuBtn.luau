local MenuBtn = {}

local App = require("./leditf_App")
local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

function MenuBtn.addButton(self: MenuLayer)
	local bottomMenu = self:getChildByID("bottom-menu"); if not bottomMenu then return end

	local sprite = cc2d.CCSprite.createWithSpriteFrameName("edit_eAnimateBtn_001.png"); if not sprite then return end

	local btn = cc2d.CCMenuItemSpriteExtra.create(sprite, function()
		App.open()
	end); if not btn then return end
	btn:setID(modId .. "/animator-button")

	bottomMenu:addChild(btn)
	bottomMenu:updateLayout(true)
end

return MenuBtn
