local KEY_COLOR = { x = 0.95, y = 0.75, z = 0.2, w = 1 }
local CELL = { x = 15, y = 15 }
local HDR_DUMMY = { x = 8, y = 8 }
local PAGE = 30
local Ser = require("./leditb_Ser")
local State = require("./leditf_State")
local tip = State.tip

local function isTick(f: number): boolean
	return f == 0 or f == 1 or f == 5 or (f >= 10 and f % 5 == 0)
end

local function colLabel(f: number): string
	return "c" .. tostring(f)
end

local Timeline = {}

local function numBuf(state: any, key: string, label: string, initial: number): (number, boolean)
	local cur = state.buffers[key]
	if type(cur) ~= "number" then
		cur = initial
	end
	local v = imgui.inputInt(label, cur)
	state.buffers[key] = v
	return v, v ~= cur
end

local function seek(state: any, f: number)
	if state.doc == nil or state.doc.track == nil then
		return
	end
	state:act("Timeline", function()
		return state.doc:scrubToTime(f / state:fps())
	end)
end

local function jumpTo(state: any, f: number)
	state.tlPage = math.floor(f / PAGE)
end

local function hasChannels(def: any): boolean
	for _, kf in pairs(def.keyframes) do
		for id, pose in pairs(kf) do
			if id ~= "events" then
				for k in pairs(pose) do
					if k ~= "easing" then return true end
				end
			end
		end
	end
	return false
end

function Timeline.draw(state: any)
	local doc = state.doc
	if doc == nil then return end
	local def = if doc.activeAnim ~= nil then doc.animations[doc.activeAnim] else nil

	imgui.text(("%s  f:%d  %.2fs / %.2fs"):format(
		tostring(doc.activeAnim or "-"), state:frameOf(), doc.playhead, doc:length()))
	imgui.spacing()
	if imgui.button("Play") then
		if def == nil or not hasChannels(def) then
			state:toast("Timeline", "No channel keys yet, drag a property field.")
		else
			state:act("Timeline", function()
				return doc:play()
			end)
		end
	end
	tip("play from start")
	imgui.sameLine()
	if imgui.button("Pause") then
		state:act("Timeline", function()
			return doc:pause()
		end)
	end
	tip("pause playback")
	imgui.sameLine()
	if imgui.button("Continue") then
		state:act("Timeline", function()
			return doc:unpause()
		end)
	end
	tip("resume playback")
	imgui.sameLine()
	if imgui.button("Stop") then
		state:act("Timeline", function()
			return doc:stop()
		end)
	end
	tip("stop and rewind")
	imgui.sameLine()
	local sp, spChanged = state:drag("Speed", doc.speed, { speed = 0.01, min = 0.05, max = 5, fmt = "%.2f" })
	if spChanged then
		state:act("Timeline", function()
			return doc:setSpeed(sp)
		end)
	end
	tip("playback speed")

	if imgui.button("Undo") then
		state:act("Timeline", function()
			return doc:undo()
		end)
	end
	tip("undo last change")
	imgui.sameLine()
	if imgui.button("Redo") then
		state:act("Timeline", function()
			return doc:redo()
		end)
	end
	tip("redo last change")

	-- Markers
	if def == nil then return end
	imgui.separatorText("Markers")
	local sawMarker = false
	local evs = state:cached("tlevs", doc, function()
		local out = {}
		for f, kf in pairs(def.keyframes) do
			if kf.events ~= nil then
				out[#out + 1] = { f = f, events = kf.events }
			end
		end
		table.sort(out, function(a, b) return a.f < b.f end)
		return out
	end)
	for _, ev in ipairs(evs) do
		for _, name in ipairs(Ser.eventArray(ev.events)) do
			sawMarker = true
			if imgui.selectable(name .. " @ " .. ev.f, state.selFrame == ev.f) and state.selFrame ~= ev.f then
				state.selFrame = ev.f
				jumpTo(state, ev.f)
				seek(state, ev.f)
			end
			tip("jump to marker")
			imgui.sameLine()
			if imgui.button("Delete Marker##mk:" .. name .. ":" .. ev.f) then
				state:act("Timeline", function()
					return doc:removeEvent(ev.f, name)
				end)
			end
			tip("remove marker")
		end
	end
	if not sawMarker then
		imgui.text("(none)")
	end
	if not state.buffers.mkframeDirty then
		state.buffers.mkframe = state:frameOf()
	end
	local mf, mfChanged = numBuf(state, "mkframe", "##mkat", state:frameOf())
	if mfChanged then
		state.buffers.mkframeDirty = true
	end
	tip("marker frame")
	local mkName = state:text("mkname", "##markername", "")
	tip("marker name")
	if imgui.button("Add Marker") and mkName ~= "" then
		if state:act("Timeline", function()
			return doc:addEvent(mf, mkName)
		end) then
			state.buffers.mkname = ""
			state.buffers.mkframeDirty = nil
			state.buffers.mkframe = state:frameOf()
		end
	end
	tip("add marker at entered frame")

	imgui.separator()

	-- Selected cell controls
	local inSpec = state:cached("tlinspec", doc, function()
		local out = {}
		for _, n in ipairs(doc.rigSpec.nodes) do
			out[n.id] = true
		end
		return out
	end)
	local gsId, gsF = state.gridSelId, state.gridSelF
	local gk = if gsId ~= nil and inSpec[gsId] and def.keyframes[gsF] ~= nil then def.keyframes[gsF][gsId] else nil
	if gk ~= nil then
		imgui.text(("key %s @ %d"):format(gsId, gsF))
		if gsF > 0 and imgui.button("<##mvprev") then
			state:act("Timeline", function()
				local ok, err = doc:moveFrame(gsF, gsF - 1)
				if ok then
					state:setGridSel(gsId, gsF - 1)
					jumpTo(state, gsF - 1)
				end
				return ok, err
			end)
		end
		tip("move key one frame earlier")
		imgui.sameLine()
		if imgui.button(">##mvnext") then
			state:act("Timeline", function()
				local ok, err = doc:moveFrame(gsF, gsF + 1)
				if ok then
					state:setGridSel(gsId, gsF + 1)
					jumpTo(state, gsF + 1)
				end
				return ok, err
			end)
		end
		tip("move key one frame later")
		if state.buffers.mvSelId ~= gsId or state.buffers.mvSelF ~= gsF then
			state.buffers.mvSelId = gsId
			state.buffers.mvSelF = gsF
			state.buffers.mvto = gsF
		end
		local mv = numBuf(state, "mvto", "##mvto", gsF)
		imgui.sameLine()
		if imgui.button("Move To") then
			state:act("Timeline", function()
				local ok, err = doc:moveFrame(gsF, mv)
				if ok then
					state:setGridSel(gsId, mv)
					jumpTo(state, mv)
				end
				return ok, err
			end)
		end
		tip("move key to entered frame")

		if imgui.button("Delete Key") then
			state:act("Timeline", function()
				return doc:clearPose(gsF, gsId)
			end)
		end
		tip("delete keyframe")
		imgui.sameLine()
		if imgui.button("Delete Frame") then
			state:act("Timeline", function()
				return doc:removeFrame(gsF)
			end)
		end
		tip("delete all keys on frame")
	end

	imgui.separator()

	-- Grid
	if state.tlPage < 0 then
		state.tlPage = 0
	end
	if state.playing then
		local cur = state:frameOf()
		if cur < state.tlPage * PAGE or cur >= (state.tlPage + 1) * PAGE then
			state.tlPage = math.max(math.floor(cur / PAGE), 0)
		end
	end
	local start = state.tlPage * PAGE

	local rows = state:cached("tlrows", doc, function()
		local out = {}
		local seen = {}
		for _, kf in pairs(def.keyframes) do
			for id in pairs(kf) do
				if id ~= "events" and inSpec[id] and not seen[id] then
					seen[id] = true
					out[#out + 1] = id
				end
			end
		end
		table.sort(out)
		return out
	end)
	if #rows == 0 then
		imgui.text("No keys yet")
		return
	end

	if imgui.button("<##pgprev") then
		if state.tlPage > 0 then
			state.tlPage -= 1
		end
	end
	tip("previous frame page")
	imgui.sameLine()
	if imgui.button(">##pgnext") then
		state.tlPage += 1
	end
	tip("next frame page")
	imgui.sameLine()
	imgui.text(("f %d-%d"):format(start, start + PAGE - 1))

	imgui.child("tlgrid", function()
		imgui.table("tl", PAGE + 1, function()
			imgui.tableSetupColumn("##node", { width = 70 })
			for i = 0, PAGE - 1 do
				imgui.tableSetupColumn(colLabel(start + i), { width = 17 })
			end
			imgui.tableNextRow()
			imgui.tableSetColumnIndex(0)
			imgui.dummy({ x = 0, y = 6 })
			for i = 0, PAGE - 1 do
				if not imgui.tableSetColumnIndex(i + 1) then break end
				local f = start + i
				if isTick(f) then
					imgui.text(tostring(f))
				else
					imgui.dummy(HDR_DUMMY)
				end
			end
			for _, id in ipairs(rows) do
				imgui.tableNextRow()
				if imgui.tableSetColumnIndex(0) then
					if imgui.selectable(id, state.selNode == id) and state.selNode ~= id then
						state:select(id)
					end
					tip("select node")
				end
				local cellPrefix = "##" .. id .. ":"
				for i = 0, PAGE - 1 do
					if not imgui.tableSetColumnIndex(i + 1) then break end
					local f = start + i
					local occ = def.keyframes[f] ~= nil and def.keyframes[f][id] ~= nil
					local cellId = cellPrefix .. f
					local pressed
					if occ then
						imgui.style.with({ colors = { [imgui.Col.Button] = KEY_COLOR } }, function()
							pressed = imgui.button(cellId, CELL)
						end)
					else
						pressed = imgui.button(cellId, CELL)
					end
					if pressed then
						state:setGridSel(id, f)
						if occ then
							seek(state, f)
						else
							local pose = if doc.rig ~= nil then doc.rig:getPose(id) else nil
							if pose ~= nil and next(pose) ~= nil then
								state:act("Timeline", function()
									return doc:putPose(f, id, pose)
								end)
								seek(state, f)
							else
								state:toast("Timeline", "Nothing to key at frame " .. f)
							end
						end
					end
					tip(if occ then "select key" else "add key here")
				end
			end
		end, {
			flags = imgui.Flag.Table.ScrollX + imgui.Flag.Table.SizingFixedFit,
		})
	end)
end

return Timeline
