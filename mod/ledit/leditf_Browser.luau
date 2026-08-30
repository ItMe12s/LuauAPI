local Project = require("./leditb_Project")
local State = require("./leditf_State")

local Browser = {}

local GREEN = { x = 0.4, y = 0.85, z = 0.4, w = 1 }
local RED = State.RED

local tip = State.tip

local function resetDocUi(state: any)
	state.selNode = nil
	state:resetAnimSel()
	state.buffers = {}
	state.combos = {}
	state.trees = {}
	state.tlPage = 0
end

local function refreshList(state: any)
	local names, err = state.proj:list()
	state.projects = names or {}
	state.corrupt = {}
	if err then
		state:toast("Browser", err)
		return
	end
	for _, name in ipairs(state.projects) do
		local ok, errors = state.proj:validate(name)
		if not ok then
			state.corrupt[name] = errors
		end
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

local function requestConfirm(state: any, kind: string, name: string)
	state.buffers.confirm = { kind = kind, name = name }
	imgui.openPopup("Confirm##ledit")
end

function Browser.draw(state: any)
	local doc = state.doc
	if doc == nil then return end
	if state.proj == nil then
		state.proj = Project.new()
		refreshList(state)
	end

	imgui.text("projects/" .. tostring(doc.name))

	local names = state.projects
	for _, name in ipairs(names) do
		local errors = state.corrupt[name]
		local label = if doc.name == name then name .. " *" else name
		local clicked
		if errors ~= nil then
			imgui.style.with({ colors = { Text = RED } }, function()
				clicked = imgui.selectable(label .. " (corrupt)", false)
			end)
			tip("project failed validation")
		else
			clicked = imgui.selectable(label, false)
			tip("load this project")
		end
		if clicked then
			if errors ~= nil then
				state.buffers.corrupt = { name = name, errors = errors }
				imgui.openPopup("Corrupt##ledit")
			else
				requestConfirm(state, "load", name)
			end
		end
	end
	if #names == 0 then
		imgui.text("(no projects)")
	end
	if imgui.button("Refresh") then
		refreshList(state)
	end
	tip("reload project list")

	imgui.separatorText("New Project")
	local nn = state:text("newproj", "##newproj", "")
	imgui.sameLine()
	if imgui.button("Create") and nn ~= "" then
		requestConfirm(state, "create", nn)
	end
	tip("create project with entered name")

	imgui.separatorText("Current Project")
	if imgui.button("Save") then
		if state:act("Browser", function()
			return state.proj:save(doc)
		end) then
			state.buffers.savedAt = os.clock()
			refreshList(state)
		end
	end
	tip("save rig and animations to disk")

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
	tip("save copy under entered name")

	if state.buffers.confirmDel == nil then
		if imgui.button("Delete Project") then
			state.buffers.confirmDel = true
		end
		tip("delete project files on disk")
	else
		if imgui.button("Confirm Delete") then
			if state:act("Browser", function()
				return state.proj:delete(doc.name)
			end) then
				refreshList(state)
			end
			state.buffers.confirmDel = nil
		end
		tip("confirm deletion")
	end
	if state.buffers.savedAt ~= nil and os.clock() - state.buffers.savedAt < 2 then
		imgui.style.with({ colors = { Text = GREEN } }, function()
			imgui.text("Saved")
		end)
	end

	local pending = state.buffers.confirm
	if pending ~= nil then
		imgui.popupModal("Confirm##ledit", function()
			if pending.kind == "load" then
				imgui.text(("Load '%s'?"):format(tostring(pending.name)))
			else
				imgui.text(("Create '%s'?"):format(tostring(pending.name)))
			end
			imgui.textWrapped("Unsaved changes will be lost.")
			if imgui.button("Yes") then
				state.buffers.confirm = nil
				imgui.closeCurrentPopup()
				if pending.kind == "load" then
					openProject(state, pending.name)
				elseif state:act("Browser", function()
					return state.proj:create(pending.name)
				end) then
					state.buffers.newproj = ""
					refreshList(state)
					openProject(state, pending.name)
				end
			end
			tip("confirm")
			imgui.sameLine()
			if imgui.button("No") then
				state.buffers.confirm = nil
				imgui.closeCurrentPopup()
			end
			tip("cancel")
		end, { size = { x = 340, y = 220 }, sizeCond = imgui.Cond.Always })
	end

	local corrupt = state.buffers.corrupt
	if corrupt ~= nil then
		imgui.popupModal("Corrupt##ledit", function()
			imgui.text(("Project '%s' is corrupt:"):format(tostring(corrupt.name)))
			imgui.spacing()
			for _, e in ipairs(corrupt.errors or {}) do
				imgui.style.with({ colors = { Text = RED } }, function()
					imgui.textWrapped("- " .. e)
				end)
			end
			imgui.spacing()
			imgui.textWrapped("Inspect and fix the listed files with a text editor, then click Refresh.")
			imgui.spacing()
			if imgui.button("OK") then
				state.buffers.corrupt = nil
				imgui.closeCurrentPopup()
			end
			tip("close")
		end, { size = { x = 340, y = 220 }, sizeCond = imgui.Cond.Always })
	end
end

return Browser
