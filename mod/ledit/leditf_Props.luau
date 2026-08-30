local M = {}

local function tip(t: string)
	if imgui.isItemHovered() then
		imgui.setTooltip(t)
	end
end

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

	if doc:findNode(sel) == nil then
		state:select(nil)
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
					state:toast("Props", "Paused for keying")
					local ok, err = doc:pause()
					if not ok then return false, err end
				end
				return doc:putPose(frame, sel, { [ch.k] = v },
					("pose:%d:%s:%s"):format(frame, sel, ch.k))
			end)
		end
		tip(ch.tip)
	end

	imgui.separatorText("Easing")

	local kf = def.keyframes[frame]
	local target = kf ~= nil and kf[sel] or nil
	if target == nil then
		imgui.text("(no key at this frame)")
		return
	end

	local ease = target.easing

	state:combo("easing", "##ease", state.EASINGS, ease or "linear", function(_, picked)
		state:act("Props", function()
			return doc:putPose(frame, sel, { easing = picked },
				("ease:%d:%s"):format(frame, sel))
		end)
	end)
	tip("interpolation curve to next key")
end

return M
