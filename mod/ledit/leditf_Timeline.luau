local M = {}

function M.draw(state: any)
	local doc = state.doc
	if doc == nil then return end

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

	imgui.separator()

	local frame = state:frameOf()
	local maxF = doc:length() * state:fps()
	if doc.track ~= nil and maxF > 0 then
		local nf = imgui.sliderFloat("##scrub", frame, 0, maxF, "%.0f")
		if nf ~= frame then
			state:act("Timeline", function()
				return doc:scrubToTime(nf / state:fps())
			end)
		end
		imgui.sameLine()
	end
	imgui.text(string.format("%s  f:%d  %.2fs / %.2fs",
		tostring(doc.activeAnim or "-"), frame, doc.playhead, doc:length()))
	imgui.sameLine()
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
	imgui.sameLine()
	if imgui.button("Key Pose") then
		state:act("Timeline", function()
			if doc.rig == nil then
				return false, "no rig"
			end
			if state.selNode == nil then
				return false, "no node selected"
			end
			local pose = doc.rig:getPose(state.selNode)
			if pose == nil then
				return false, "no pose readback for node"
			end
			return doc:putPose(frame, state.selNode, pose, "keypose:" .. state.selNode)
		end)
	end
end

return M
