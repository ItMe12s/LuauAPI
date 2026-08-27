local Project = require("./leditb_Project")

local M = {}

local GREEN = { x = 0.4, y = 0.85, z = 0.4, w = 1 }

local function resetDocUi(state: any)
	state.selNode = nil
	state:resetAnimSel()
	state.buffers = {}
	state.combos = {}
end

local function refreshList(state: any)
	local names, err = state.proj:list()
	state.projects = names or {}
	if err then
		state:toast("Browser", err)
	end
end

local function openProject(state: any, name: any)
	state:act("Browser", function()
		local doc, err = state.proj:load(name)
		if doc == nil then
			return false, err
		end
		state.doc = doc
		resetDocUi(state)
		return true, nil
	end)
end

function M.draw(state: any)
	local doc = state.doc
	if doc == nil then return end
	if state.proj == nil then
		state.proj = Project.new()
		refreshList(state)
	end

	imgui.text("projects/" .. tostring(doc.name))

	local names = state.projects
	if names ~= nil then
		for _, name in ipairs(names) do
			local label = if doc.name == name then name .. " *" else name
			if imgui.selectable(label, false) then
				openProject(state, name)
			end
		end
		if #names == 0 then
			imgui.text("(no projects)")
		end
	end
	if imgui.button("Refresh") then
		refreshList(state)
	end

	imgui.separatorText("New Project")
	local nn = state:text("newproj", "##newproj", "")
	imgui.sameLine()
	if imgui.button("Create") and nn ~= "" then
		if state:act("Browser", function()
			return state.proj:create(nn)
		end) then
			state.buffers.newproj = ""
			refreshList(state)
			openProject(state, nn)
		end
	end

	imgui.separatorText("Current Project")
	if imgui.button("Save") then
		if state:act("Browser", function()
			return state.proj:save(doc)
		end) then
			state.buffers.savedAt = os.clock()
			refreshList(state)
		end
	end
	if imgui.isItemHovered() then
		imgui.setTooltip("writes rig + all anims")
	end

	local sa = state:text("saveas", "##saveas", "")
	imgui.sameLine()
	if imgui.button("Save As") and sa ~= "" then
		if state:act("Browser", function()
			return state.proj:saveAs(doc, sa)
		end) then
			state.buffers.saveas = ""
			resetDocUi(state)
			refreshList(state)
		end
	end

	if state.buffers.confirmDel == nil then
		if imgui.button("Delete Project") then
			state.buffers.confirmDel = true
		end
		if imgui.isItemHovered() then
			imgui.setTooltip("removes files on disk")
		end
	else
		if imgui.button("Confirm Delete") then
			if state:act("Browser", function()
				return state.proj:delete(doc.name)
			end) then
				refreshList(state)
			end
			state.buffers.confirmDel = nil
		end
		if imgui.isItemHovered() then
			imgui.setTooltip("click again to confirm")
		end
	end
	if state.buffers.savedAt ~= nil and os.clock() - state.buffers.savedAt < 2 then
		imgui.style.with({ colors = { Text = GREEN } }, function()
			imgui.text("Saved")
		end)
	end
end

return M
