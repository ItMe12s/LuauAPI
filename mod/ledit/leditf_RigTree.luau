local M = {}

local function tip(t: string)
	if imgui.isItemHovered() then
		imgui.setTooltip(t)
	end
end

function M.draw(state: any)
	local doc = state.doc
	if doc == nil then return end

	local CHANNELS = state.CHANNELS
	local FALLBACK = state.FALLBACK

	local index, kids, roots = {}, {}, {}
	for i, n in ipairs(doc.rigSpec.nodes) do
		index[n.id] = i
	end
	local function parentItems()
		local items = { "(root)" }
		for _, n in ipairs(doc.rigSpec.nodes) do
			table.insert(items, n.id)
		end
		return items
	end
	for _, n in ipairs(doc.rigSpec.nodes) do
		if n.parent ~= nil and index[n.parent] then
			kids[n.parent] = kids[n.parent] or {}
			table.insert(kids[n.parent], n.id)
		else
			table.insert(roots, n.id)
		end
	end

	local function drawNode(id, depth)
		if depth > 64 then return end
		local selected = state.selNode == id
		local childIds = kids[id]
		if childIds ~= nil then
			local open = if state.trees[id] == nil then true else state.trees[id]
			local flags = if selected then imgui.Flag.TreeNode.Selected else imgui.Flag.TreeNode.None
			local tracked = imgui.treeNode(id .. "##tree:" .. id, function()
				if imgui.isItemClicked() and not selected then
					state:select(id)
				end
				for _, cid in ipairs(childIds) do
					drawNode(cid, depth + 1)
				end
			end, { open = open, flags = flags })
			tip("select node")
			if tracked == false and imgui.isItemClicked() and not selected then
				state:select(id)
			end
			if tracked ~= nil then
				state.trees[id] = tracked
			end
		else
			if imgui.selectable(id, selected) and not selected then
				state:select(id)
			end
			tip("select node")
		end
	end

	local rootOpen = if state.trees["(root)"] == nil then true else state.trees["(root)"]
	local rootTracked = imgui.treeNode("(root)", function()
		if #roots == 0 then
			imgui.text("(no nodes)")
		end
		for _, id in ipairs(roots) do
			drawNode(id, 0)
		end
	end, { open = rootOpen })
	if rootTracked ~= nil then
		state.trees["(root)"] = rootTracked
	end

	imgui.separator()

	local sel = state.selNode
	if sel == nil or not index[sel] then
		imgui.text("No node selected")
	else
		imgui.text("Selected: " .. sel)
		local node = doc.rigSpec.nodes[index[sel]]

		imgui.separatorText("Rename")
		local newName = state:text("rename", "##rename")
		if imgui.button("Rename") and newName ~= "" then
			if state:act("RigTree", function()
				return doc:renameNode(sel, newName)
			end) then
				state:select(newName)
			end
		end
		tip("rename to entered name")

		imgui.separatorText("Sprite")
		local sprite = state:text("sprite", "##sprite", node.sprite or "")
		imgui.textWrapped("Geometry Dash: file.png | Mod: <mod-id>/file.png")
		if imgui.button("Set Sprite") then
			state:act("RigTree", function()
				return doc:setSprite(sel, sprite ~= "" and sprite or nil)
			end)
		end
		tip("set node sprite")

		imgui.separatorText("Reparent")
		local pItems = parentItems()
		local curParent = node.parent or "(root)"
		state:combo("reparent", "##reparent", pItems, curParent, function(_, item)
			local parent = if item == "(root)" then nil else item
			state:act("RigTree", function()
				return doc:reparent(sel, parent)
			end)
		end)
		tip("change parent node")
		if imgui.button("Delete Node") then
			if state:act("RigTree", function()
				return doc:deleteNode(sel)
			end) then
				state:select(nil)
				state:setGridSel(nil, nil)
			end
		end
		tip("delete node from rig")

		imgui.separatorText("Base Transform")
		for _, ch in ipairs(CHANNELS) do
			local cur = node[ch.k]
			if cur == nil then cur = FALLBACK[ch.k] end
			local v, changed = state:drag(ch.k, cur, {
				speed = ch.speed,
				min = ch.min,
				max = ch.max,
				fmt = ch.fmt,
			})
			if changed then
				state:act("RigTree", function()
					return doc:setBase(sel, { [ch.k] = v }, "base:" .. sel .. ":" .. ch.k)
				end)
			end
			tip(ch.tip)
		end
	end

	imgui.separator()

	imgui.separatorText("Add Node")
	local newId = state:text("addid", "id", "")
	local addSprite = state:text("addsprite", "sprite (optional)", "")
	local addParent = select(2, state:combo("addparent", "parent", parentItems()))
	tip("parent for new node")
	if imgui.button("Add Node") and newId ~= "" then
		if state:act("RigTree", function()
			return doc:addNode({
				id = newId,
				sprite = if addSprite ~= "" then addSprite else nil,
				parent = if addParent == "(root)" then nil else addParent,
			})
		end) then
			state.buffers.addid = ""
			state.buffers.addsprite = ""
			state.combos.addparent = nil
			state:select(newId)
		end
	end
	tip("add node with entered details")
end

return M
