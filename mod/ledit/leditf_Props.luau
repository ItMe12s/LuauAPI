local M = {}

function M.draw(state: any)
	local doc = state.doc
	if doc == nil then return end

	local sel = state.selNode
	if sel == nil then
		imgui.text("No node selected")
		return
	end

	local def = if doc.activeAnim ~= nil then doc.animations[doc.activeAnim] else nil
	if def == nil then
		imgui.text("No active animation")
		return
	end

	local inSpec = false
	for _, n in ipairs(doc.rigSpec.nodes) do
		if n.id == sel then
			inSpec = true
			break
		end
	end
	if not inSpec then
		imgui.text("Deleted node")
		return
	end

	local frame = state:frameOf()
	imgui.text(sel .. " @ " .. tostring(frame))
	local pose = if doc.rig ~= nil then doc.rig:getPose(sel) else nil

	for _, ch in ipairs(state.CHANNELS) do
		local cur = pose ~= nil and pose[ch.k] or nil
		if cur == nil then
			cur = state.FALLBACK[ch.k]
		end
		local v, changed = state:drag(ch.k, cur, {
			speed = ch.speed,
			min = ch.min,
			max = ch.max,
			fmt = ch.fmt,
		})
		if changed then
			state:act("Props", function()
				if state.playing then
					local ok, err = doc:pause()
					if not ok then return false, err end
				end
				return doc:putPose(frame, sel, { [ch.k] = v },
					("pose:%d:%s:%s"):format(frame, sel, ch.k))
			end)
		end
	end

	imgui.separatorText("Easing")

	local kf = def.keyframes[frame]
	local target = kf ~= nil and kf[sel] or nil
	local ease = target ~= nil and target.easing or nil
	local easedIdx = nil
	for i, name in ipairs(state.EASINGS) do
		if name == ease then
			easedIdx = i - 1
			break
		end
	end
	if easedIdx == nil then
		easedIdx = 0
	end
	local comboKey = "e:" .. sel .. ":" .. frame
	if state.combos.lastEaseKey ~= comboKey then
		state.combos.lastEaseKey = comboKey
		state.combos.easing = easedIdx
	end

	state:combo("easing", "##ease", state.EASINGS, function(_, picked)
		state:act("Props", function()
			return doc:putPose(frame, sel, { easing = picked },
				("ease:%d:%s"):format(frame, sel))
		end)
	end)
end

return M
