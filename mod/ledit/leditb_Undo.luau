local Undo = {}
Undo.__index = Undo

local CAP = 256
local COALESCE_SECONDS = 0.5

function Undo.new(): any
	return setmetatable({ undoStack = {}, redoStack = {} }, Undo)
end

function Undo:push(key: any, undoFn: any, redoFn: any)
	self.redoStack = {}
	local top = self.undoStack[#self.undoStack]
	local now = os.clock()
	if key ~= nil and top ~= nil and top.key == key and now - top.time <= COALESCE_SECONDS then
		top.redo = redoFn
		top.time = now
		return
	end
	if #self.undoStack >= CAP then
		table.remove(self.undoStack, 1)
	end
	self.undoStack[#self.undoStack + 1] = {
		key = key,
		undo = undoFn,
		redo = redoFn,
		time = now,
	}
end

function Undo:canUndo(): boolean
	return #self.undoStack > 0
end

function Undo:canRedo(): boolean
	return #self.redoStack > 0
end

function Undo:undo(): (boolean, string?)
	local cmd = table.remove(self.undoStack)
	if cmd == nil then
		return false, "nothing to undo"
	end
	local ok, err = pcall(cmd.undo)
	if not ok then
		-- Restore the command so a throw does not lose it from both stacks.
		self.undoStack[#self.undoStack + 1] = cmd
		return false, tostring(err)
	end
	self.redoStack[#self.redoStack + 1] = cmd
	return true, nil
end

function Undo:redo(): (boolean, string?)
	local cmd = table.remove(self.redoStack)
	if cmd == nil then
		return false, "nothing to redo"
	end
	local ok, err = pcall(cmd.redo)
	if not ok then
		self.redoStack[#self.redoStack + 1] = cmd
		return false, tostring(err)
	end
	self.undoStack[#self.undoStack + 1] = cmd
	return true, nil
end

function Undo:clear()
	self.undoStack = {}
	self.redoStack = {}
end

return Undo
