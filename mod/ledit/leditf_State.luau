local State = {} :: any

function State.new(): any
	return {
		active = false,
		doc = nil,
		toasts = {},
		selNode = nil,
		buffers = {},
		combos = {},
	}
end

function State.toast(self: any, id: any, msg: any)
	self.toasts[id] = { msg = msg, at = os.clock() }
end

function State.act(self: any, winId: any, op: any): (boolean, string?)
	local ok, err = op()
	if not ok then
		self:toast(winId, err or "unknown error")
	end
	return ok, err
end

function State.select(self: any, id: any)
	self.selNode = id
	self.buffers.rename = nil
	self.buffers.sprite = nil
	self.combos.reparent = nil
end

function State.text(self: any, key: any, label: string, initial: string?): (string, boolean)
	local b = self.buffers[key]
	if b == nil then
		b = initial or ""
	end
	local v = imgui.inputText(label, b)
	local changed = v ~= b
	self.buffers[key] = v
	return v, changed
end

function State.drag(self: any, label: string, value: number, opts: any): (number, boolean)
	local v = imgui.dragFloat(label, value, opts)
	return v, v ~= value
end

function State.combo(self: any, key: any, label: string, items: { string }, onChanged: any?): (number, string?)
	local idx = self.combos[key]
	if idx == nil or idx >= #items then
		idx = 0
	end
	local prev = idx
	idx = imgui.combo(label, idx, items)
	self.combos[key] = idx
	if idx ~= prev and onChanged then
		onChanged(idx, items[idx + 1])
	end
	return idx, items[idx + 1]
end

return State
