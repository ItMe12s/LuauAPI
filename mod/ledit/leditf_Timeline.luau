local KEY_COLOR = { x = 0.95, y = 0.75, z = 0.2, w = 1 }
local CELL = { x = 15, y = 15 }
local HDR_DUMMY = { x = 8, y = 8 }
local MAX_COLS = 1024
local COL_LABELS = {}
do
	for f = 0, MAX_COLS do
		COL_LABELS[f] = ("c%d"):format(f)
	end
end

local M = {}

local function numBuf(state: any, key: string, label: string, initial: number, delta: number?): (number, boolean)
	local cur = state.buffers[key]
	if type(cur) ~= "number" then
		cur = initial
	end
	local v = imgui.inputInt(label, cur, { step = delta })
	state.buffers[key] = v
	return v, v ~= cur
end

local function seek(state: any, f: number)
	state:act("Timeline", function()
		return state.doc:scrubToTime(f / state:fps())
	end)
end

function M.draw(state: any)
	local doc = state.doc
	if doc == nil then return end

	imgui.text(("%s  f:%d  %.2fs / %.2fs"):format(
		tostring(doc.activeAnim or "-"), state:frameOf(), doc.playhead, doc:length()))
	imgui.spacing()
	if imgui.button("Play") then
		state:act("Timeline", function()
			return doc:play()
		end)
	end
	imgui.sameLine()
	if imgui.button("Pause") then
		state:act("Timeline", function()
			return doc:pause()
		end)
	end
	imgui.sameLine()
	if imgui.button("Stop") then
		state:act("Timeline", function()
			return doc:stop()
		end)
	end
	imgui.sameLine()
	local sp, spChanged = state:drag("Speed", doc.speed, { speed = 0.01, min = 0.05, max = 5, fmt = "%.2f" })
	if spChanged then
		state:act("Timeline", function()
			return doc:setSpeed(sp)
		end)
	end

	if imgui.button("Undo") then
		state:act("Timeline", function()
			return doc:undo()
		end)
	end
	imgui.sameLine()
	if imgui.button("Redo") then
		state:act("Timeline", function()
			return doc:redo()
		end)
	end

	local def = if doc.activeAnim ~= nil then doc.animations[doc.activeAnim] else nil
	if def == nil then return end

	-- Markers
	imgui.separatorText("Markers")
	local first = true
	local evs = {}
	for f, kf in pairs(def.keyframes) do
		if kf.events ~= nil then
			evs[#evs + 1] = { f = f, events = kf.events }
		end
	end
	table.sort(evs, function(a, b) return a.f < b.f end)
	for _, ev in ipairs(evs) do
		local names = if type(ev.events) == "table" then ev.events else { ev.events }
		for _, name in ipairs(names) do
			first = false
			if imgui.selectable(name .. " @ " .. ev.f, state.selFrame == ev.f) then
				state.selFrame = ev.f
				seek(state, ev.f)
			end
			imgui.sameLine()
			if imgui.button("-##mk:" .. name .. ":" .. ev.f) then
				state:act("Timeline", function()
					return doc:removeEvent(ev.f, name)
				end)
			end
			if imgui.isItemHovered() then
				imgui.setTooltip("remove marker")
			end
		end
	end
	if first then
		imgui.text("(none)")
	end
	local mf = numBuf(state, "mkframe", "##mkat", state:frameOf(), 1)
	local mkName = state:text("mkname", "##markername", "")
	if imgui.button("Add Marker") and mkName ~= "" then
		state:act("Timeline", function()
			return doc:addEvent(mf, mkName)
		end)
	end

	imgui.separator()

	-- Selected cell controls
	local gsId, gsF = state.gridSelId, state.gridSelF
	local gk = if gsId ~= nil and def.keyframes[gsF] ~= nil then def.keyframes[gsF][gsId] else nil
	if gk ~= nil then
		imgui.text(("key %s @ %d"):format(gsId, gsF))
		if gsF > 0 and imgui.button("<") then
			state:act("Timeline", function()
				local ok, err = doc:moveFrame(gsF, gsF - 1)
				if ok then state:setGridSel(gsId, gsF - 1) end
				return ok, err
			end)
		end
		imgui.sameLine()
		if imgui.button(">") then
			state:act("Timeline", function()
				local ok, err = doc:moveFrame(gsF, gsF + 1)
				if ok then state:setGridSel(gsId, gsF + 1) end
				return ok, err
			end)
		end
		local mv = numBuf(state, "mvframe", "##mvto", gsF, 1)
		imgui.sameLine()
		if imgui.button("Move To") then
			state:act("Timeline", function()
				local ok, err = doc:moveFrame(gsF, mv)
				if ok then state:setGridSel(gsId, mv) end
				return ok, err
			end)
		end

		if imgui.button("Del Key") then
			state:act("Timeline", function()
				return doc:clearPose(gsF, gsId)
			end)
		end
		imgui.sameLine()
		if imgui.button("Del Frame") then
			state:act("Timeline", function()
				return doc:removeFrame(gsF)
			end)
		end
	end

	-- Grid
	local fps = state:fps()
	local maxF = math.max(31, math.floor(doc:length() * fps + 0.5))
	if maxF > MAX_COLS then maxF = MAX_COLS end

	local inSpec = {}
	for _, n in ipairs(doc.rigSpec.nodes) do
		inSpec[n.id] = true
	end
	local rows = {}
	local seenRows = {}
	for _, kf in pairs(def.keyframes) do
		for id in pairs(kf) do
			if id ~= "events" and inSpec[id] and not seenRows[id] then
				seenRows[id] = true
				rows[#rows + 1] = id
			end
		end
	end
	table.sort(rows)
	if #rows == 0 then
		imgui.text("No keys yet")
		return
	end

	imgui.child("tlgrid", function()
		imgui.table("tl", maxF + 2, function()
			imgui.tableSetupColumn("##node", { width = 70 })
			for f = 0, maxF do
				imgui.tableSetupColumn(COL_LABELS[f], { width = 17 })
			end
			imgui.tableNextRow()
			imgui.tableSetColumnIndex(0)
			imgui.dummy({ x = 0, y = 6 })
			for f = 0, maxF do
				if not imgui.tableSetColumnIndex(f + 1) then break end
				if f % 5 == 0 then
					imgui.text(tostring(f))
				else
					imgui.dummy(HDR_DUMMY)
				end
			end
			for _, id in ipairs(rows) do
				imgui.tableNextRow()
				if imgui.tableSetColumnIndex(0) then
					if imgui.selectable(id, state.selNode == id) then
						state:select(id)
					end
				end
				for f = 0, maxF do
					if not imgui.tableSetColumnIndex(f + 1) then break end
					local occ = def.keyframes[f] ~= nil and def.keyframes[f][id] ~= nil
					local cellId = "##" .. id .. ":" .. f
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
							state:act("Timeline", function()
								return doc:putPose(f, id, {})
							end)
							seek(state, f)
						end
					end
				end
			end
		end, {
			flags = imgui.Flag.Table.ScrollX + imgui.Flag.Table.SizingFixedFit,
		})
	end)
end

return M
