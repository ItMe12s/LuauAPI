local M = {}

local function tip(t: string)
	if imgui.isItemHovered() then
		imgui.setTooltip(t)
	end
end

local function sortedNames(doc: any): { string }
	local names = {}
	for name in pairs(doc.animations) do
		table.insert(names, name)
	end
	table.sort(names)
	return names
end

function M.draw(state: any)
	local doc = state.doc
	if doc == nil then return end

	if state.lastActiveAnim ~= doc.activeAnim then
		state.lastActiveAnim = doc.activeAnim
		state:resetAnimSel()
	end

	for _, name in ipairs(sortedNames(doc)) do
		if imgui.selectable(name, doc.activeAnim == name) and doc.activeAnim ~= name then
			state:act("Anims", function()
				return doc:setActive(name)
			end)
		end
		tip("select this animation")
	end

	imgui.separator()

	local newName = state:text("animnew", "##animnew", "")
	imgui.sameLine()
	if imgui.button("Add Anim") then
		if state:act("Anims", function()
			return doc:addAnim(newName)
		end) then
			state.buffers.animnew = ""
			state:act("Anims", function()
				return doc:setActive(newName)
			end)
		end
	end
	tip("create animation with entered name")

	local act = doc.activeAnim
	local def = if act ~= nil then doc.animations[act] else nil
	if def == nil then return end

	imgui.separatorText("Active: " .. act)

	local v = imgui.inputInt("FPS", def.fps or 30)
	if v ~= (def.fps or 30) and v > 0 then
		state:act("Anims", function()
			return doc:setFps(v, "animfps")
		end)
	end
	tip("frames per second")

	local lb = imgui.checkbox("Looped", def.looped == true)
	if lb ~= (def.looped == true) then
		state:act("Anims", function()
			return doc:setLooped(lb)
		end)
	end
	tip("loop playback")

	local rn = state:text("animrename", "new name")
	imgui.sameLine()
	if imgui.button("Rename") and rn ~= "" and rn ~= act then
		if state:act("Anims", function()
			return doc:renameAnim(act, rn)
		end) then
			state.buffers.animrename = nil
		end
	end
	tip("rename to entered name")

	local dup = state:text("animdup", "##dup", "")
	imgui.sameLine()
	if imgui.button("Duplicate") then
		if dup ~= "" and state:act("Anims", function()
			return doc:duplicateAnim(act, dup)
		end) then
			state.buffers.animdup = ""
			state:act("Anims", function()
				return doc:setActive(dup)
			end)
		end
	end
	tip("copy under entered name")

	if imgui.button("Delete") then
		state:act("Anims", function()
			return doc:deleteAnim(act)
		end)
	end
	tip("delete this animation")
end

return M
