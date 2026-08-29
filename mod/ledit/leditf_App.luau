local State = require("./leditf_State")
local Doc = require("./leditb_Doc")

local cc2d = geode.cocos2d
local modId = geode.Mod.getID()

local state = State.new()
local drawHandle = nil
local layerRef: CCLayerColor? = nil
local attachedRig: CCNode? = nil

local RED = { x = 0.9, y = 0.25, z = 0.25, w = 1 }
local TOAST_LIFE = 4

local function drawToasts(id)
	local t = state.toasts[id]
	if not t then return end
	if os.clock() - t.at > TOAST_LIFE then
		state.toasts[id] = nil
		return
	end
	imgui.style.with({ colors = { Text = RED } }, function()
		imgui.textWrapped(t.msg)
	end)
end

local function drawStatus(st)
	local doc = st.doc
	local warn = if doc ~= nil then doc.rigWarn else nil
	if st.tickError == nil and warn == nil then
		imgui.text("ok")
		return
	end
	imgui.style.with({ colors = { Text = RED } }, function()
		if st.tickError ~= nil then
			imgui.textWrapped("tick: " .. st.tickError)
		end
		if warn ~= nil then
			imgui.textWrapped(warn)
		end
	end)
end

local WINDOWS: { { id: string, title: string, pos: any, size: any, draw: any } } = {
	{ id = "Browser", title = "Project Browser", draw = require("./leditf_Browser").draw, pos = { x = 12, y = 40 }, size = { x = 230, y = 360 } },
	{ id = "RigTree", title = "Rig Tree", draw = require("./leditf_RigTree").draw, pos = { x = 12, y = 220 }, size = { x = 230, y = 330 } },
	{ id = "Anims", title = "Animations", draw = require("./leditf_Anims").draw, pos = { x = 252, y = 40 }, size = { x = 190, y = 280 } },
	{ id = "Props", title = "Properties", draw = require("./leditf_Props").draw, pos = { x = 452, y = 40 }, size = { x = 210, y = 360 } },
	{ id = "Timeline", title = "Timeline", draw = require("./leditf_Timeline").draw, pos = { x = 672, y = 40 }, size = { x = 580, y = 380 } },
	{ id = "Status", title = "Status", draw = drawStatus, pos = { x = 672, y = 436 }, size = { x = 280, y = 110 } },
}

local function drawWindows()
	for _, w in WINDOWS do
		imgui.window(w.title, function()
			drawToasts(w.id)
			local ok, err = pcall(w.draw, state)
			if not ok then
				warn("[ledit] " .. w.id .. ": " .. tostring(err))
				state:toast(w.id, tostring(err))
			end
		end, {
			pos = w.pos,
			posCond = imgui.Cond.FirstUseEver,
			size = w.size,
			sizeCond = imgui.Cond.FirstUseEver,
			flags = imgui.Flag.Window.NoCollapse,
		})
	end
	return nil
end

local lastTrack: any = nil
local function bindMarkers(doc: any)
	local def = if doc.activeAnim ~= nil then doc.animations[doc.activeAnim] else nil
	if def == nil or doc.track == nil then return end
	local seen = {}
	for _, kf in pairs(def.keyframes) do
		local evs = kf.events
		if evs ~= nil then
			local names = if type(evs) == "table" then evs else { evs }
			for _, name in ipairs(names) do
				if seen[name] == nil then
					seen[name] = true
					doc.track:bindEvent(name, function(n)
						state:toast("Timeline", "event: " .. tostring(n))
					end)
				end
			end
		end
	end
end

local function tickBody()
	local doc = state.doc
	if doc and doc.rig and layerRef and doc.rig ~= attachedRig then
		if attachedRig then attachedRig:removeFromParent() end
		local director = cc2d.CCDirector.sharedDirector()
		if director then
			local s = director:getWinSize()
			doc.rig:setPosition({ x = s.width / 2, y = s.height / 2 })
		end
		layerRef:addChild(doc.rig)
		attachedRig = doc.rig
	end

	if doc ~= nil and doc.track ~= lastTrack then
		lastTrack = doc.track
		bindMarkers(doc)
	end

	if doc.track ~= nil then
		state.playing = doc.track:isPlaying()
		doc.playhead = doc.track:currentTime()
	else
		state.playing = false
	end
	return nil
end

local function tick()
	local ok, err = pcall(tickBody)
	state.tickError = if ok then nil else tostring(err)
end

local function ensureDrawHandle()
	if drawHandle then return end
	drawHandle = imgui.onDraw(function()
		if not state.active then return end
		imgui.theme.apply("dark")
		tick()
		local ok, err = pcall(drawWindows)
		if not ok then
			warn("[ledit] ui error: " .. tostring(err))
		end
	end)
end

local App = {}

function App.open()
	if state.active then return end
	local layer = cc2d.CCLayerColor.create({ r = 26, g = 26, b = 30, a = 255 })
	if not layer then return end
	layer:setID(modId .. "/animator-stage")

	state.doc = state.doc or Doc.new()

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

	layerRef = layer
	attachedRig = nil
	lastTrack = nil

	state.active = true
	ensureDrawHandle()
	geode.pushSceneWithLayer(layer)
end

function App.close()
	state.active = false
	state.playing = false
	if state.doc then state.doc:stop() end
	layerRef = nil
	attachedRig = nil
	local director = cc2d.CCDirector.sharedDirector()
	if director then director:popScene() end
end

return App
