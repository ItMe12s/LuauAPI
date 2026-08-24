local State = require("./leditf_State")

local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local state = State.new()
local drawHandle = nil

local WINDOWS = {
	{ id = "Browser", title = "Project Browser", mod = require("./leditf_Browser"), pos = { x = 12, y = 40 }, size = { x = 230, y = 170 } },
	{ id = "RigTree", title = "Rig Tree", mod = require("./leditf_RigTree"), pos = { x = 12, y = 220 }, size = { x = 230, y = 330 } },
	{ id = "Anims", title = "Animations", mod = require("./leditf_Anims"), pos = { x = 252, y = 40 }, size = { x = 190, y = 280 } },
	{ id = "Props", title = "Properties", mod = require("./leditf_Props"), pos = { x = 452, y = 40 }, size = { x = 210, y = 360 } },
	{ id = "Timeline", title = "Timeline", mod = require("./leditf_Timeline"), pos = { x = 672, y = 40 }, size = { x = 580, y = 380 } },
}

local RED = { x = 0.9, y = 0.25, z = 0.25, w = 1 }

local function drawToasts(id)
	local err = state.toasts[id]
	if not err then return end
	imgui.style.with({ colors = { Text = RED } }, function()
		imgui.textWrapped(err)
	end)
end

local function drawWindows()
	for _, w in WINDOWS do
		imgui.window(w.title, function()
			drawToasts(w.id)
			w.mod.draw(state)
		end, {
			pos = w.pos,
			posCond = imgui.Cond.FirstUseEver,
			size = w.size,
			sizeCond = imgui.Cond.FirstUseEver,
			flags = imgui.Flag.Window.NoCollapse,
		})
	end
end

local function ensureDrawHandle()
	if drawHandle then return end
	drawHandle = imgui.onDraw(function()
		if not state.active then return end
		imgui.theme.apply("dark")
		drawWindows()
	end)
end

local App = {}

function App.open()
	if state.active then return end
	local layer = cc2d.CCLayerColor.create({ r = 26, g = 26, b = 30, a = 255 })
	if not layer then return end
	layer:setID(modId .. "/animator-stage")

	local director = cc2d.CCDirector.sharedDirector(); if not director then return end
	local winSize = director:getWinSize()

	local menu = cc2d.CCMenu.create(); if not menu then return end
	menu:setPosition({ x = 0, y = 0 })

	local backLabel = cc2d.CCLabelBMFont.create("Back", "chatFont.fnt")
	if backLabel then
		backLabel:setScale(0.7)
		local back = cc2d.CCMenuItemSpriteExtra.create(backLabel, function()
			App.close()
		end)
		if back then
			back:setPosition({ x = 28, y = winSize.height - 24 })
			menu:addChild(back)
		end
	end

	layer:addChild(menu)

	state.active = true
	ensureDrawHandle()
	geode.pushSceneWithLayer(layer)
end

function App.close()
	state.active = false
	local director = cc2d.CCDirector.sharedDirector()
	if director then director:popScene() end
end

return App
