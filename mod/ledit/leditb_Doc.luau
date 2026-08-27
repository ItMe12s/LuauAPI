local Core = require("./leditb_DocCore")
local RigOps = require("./leditb_DocRig")
local DefOps = require("./leditb_DocDefs")
local Undo = require("./leditb_Undo")

local Doc = {} :: any
Doc.__index = Doc

function Doc.new(deps: any)
	deps = deps or {}
	local d = setmetatable({}, Doc)
	d.lunar = deps.lunar or lunar
	d.name = ""
	d.rigSpec = { nodes = {} }
	d.animations = {}
	d.activeAnim = nil
	d.playhead = 0
	d.speed = 1
	d.undoStack = deps.undoStack or Undo.new()
	d.rig = nil
	d.track = nil
	return d
end

function Doc.restore(deps, name, rigSpec, animations)
	local d = Doc.new(deps)
	d.name = name
	d.rigSpec = rigSpec
	d.animations = animations
	local ok, err = Core.rebuildRig(d)
	if not ok then
		return nil, err
	end
	return d, nil
end

for k, fn in pairs(RigOps) do
	Doc[k] = fn
end

for k, fn in pairs(DefOps) do
	Doc[k] = fn
end

function Doc:play()
	if self.track == nil then
		local ok, err = Core.recompile(self)
		if not ok then
			return false, err
		end
	end
	if self.track == nil then
		return false, "no active animation"
	end
	self.track:play()
	return true, nil
end

function Doc:pause()
	if self.track == nil then
		return false, "no track"
	end
	self.track:pause()
	return true, nil
end

function Doc:unpause()
	if self.track == nil then
		return false, "no track"
	end
	self.track:unpause()
	return true, nil
end

function Doc:stop()
	if self.track == nil then
		return false, "no track"
	end
	self.track:stop()
	self.playhead = self.track:currentTime()
	return true, nil
end

function Doc:setSpeed(speed)
	if not Core.finite(speed) or speed <= 0 then
		return false, "speed must be > 0"
	end
	self.speed = speed
	if self.track ~= nil then
		self.track:setSpeed(speed)
	end
	return true, nil
end

function Doc:scrubToTime(t)
	if self.track == nil then
		return false, "no track"
	end
	local dur = self.track:duration()
	self.track:seek(math.clamp(t, 0, dur))
	self.playhead = if dur > 0 then self.track:currentTime() else math.max(t, 0)
	return true, nil
end

function Doc:undo()
	if not self.undoStack:canUndo() then
		return false, "nothing to undo"
	end
	self.undoStack:undo()
	return true, nil
end

function Doc:redo()
	if not self.undoStack:canRedo() then
		return false, "nothing to redo"
	end
	self.undoStack:redo()
	return true, nil
end

return Doc
