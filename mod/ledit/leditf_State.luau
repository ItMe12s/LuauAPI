local State = {} :: any

State.CHANNELS = {
	{ k = "x", speed = 1 },
	{ k = "y", speed = 1 },
	{ k = "rot", speed = 0.5 },
	{ k = "sx", speed = 0.02 },
	{ k = "sy", speed = 0.02 },
	{ k = "skx", speed = 0.02 },
	{ k = "sky", speed = 0.02 },
	{ k = "z", speed = 1, fmt = "%.0f" },
	{ k = "opacity", speed = 2, min = 0, max = 255, fmt = "%.0f" },
	{ k = "ax", speed = 0.01, min = 0, max = 1 },
	{ k = "ay", speed = 0.01, min = 0, max = 1 },
}

State.FALLBACK = { x = 0, y = 0, rot = 0, sx = 1, sy = 1, skx = 0, sky = 0, z = 0, opacity = 255, ax = 0.5, ay = 0.5 }

State.EASINGS = {
	"linear",
	"quad_in", "quad_out", "quad_in_out",
	"cubic_in", "cubic_out", "cubic_in_out",
	"quart_in", "quart_out", "quart_in_out",
	"quint_in", "quint_out", "quint_in_out",
	"sine_in", "sine_out", "sine_in_out",
	"expo_in", "expo_out", "expo_in_out",
	"back_in", "back_out", "back_in_out",
	"elastic_in", "elastic_out", "elastic_in_out",
	"bounce_in", "bounce_out", "bounce_in_out",
}

function State.new(): any
	return setmetatable({
		active = false,
		doc = nil,
		toasts = {},
		selNode = nil,
		selFrame = nil,
		gridSelId = nil,
		gridSelF = nil,
		playing = false,
		tickError = nil,
		lastActiveAnim = nil,
		tlPage = 0,
		trees = {},
		buffers = {},
		combos = {},
	}, { __index = State })
end

function State.fps(self: any): number
	local doc = self.doc
	local def = if doc ~= nil and doc.activeAnim ~= nil then doc.animations[doc.activeAnim] else nil
	return (def and def.fps) or 30
end

function State.frameOf(self: any): number
	local doc = self.doc
	if doc == nil then return 0 end
	return math.floor(doc.playhead * self:fps())
end

function State.toast(self: any, id: any, msg: any)
	self.toasts[id] = { msg = msg, at = os.clock() }
end

function State.act(self: any, winId: any, op: any): (boolean, string?)
	local pok, r1, r2 = pcall(op)
	local ok = pok and r1 == true
	local err = if not pok then tostring(r1) else r2
	if not ok then
		self:toast(winId, err or "unknown error")
	end
	return ok, err
end

function State.setGridSel(self: any, id: any, f: any)
	self.gridSelId = id
	self.gridSelF = f
end

function State.resetAnimSel(self: any)
	self.selFrame = nil
	self:setGridSel(nil, nil)
	self.tlPage = 0
	self.buffers.animrename = nil
	self.buffers.animdup = nil
end

function State.select(self: any, id: any)
	self.selNode = id
	self.buffers.rename = nil
	self.buffers.sprite = nil
	self.combos.reparent = nil
end

function State.text(self: any, key: any, label: string, initial: string?): string
	local b = self.buffers[key]
	if b == nil then
		b = initial or ""
	end
	local v = imgui.inputText(label, b)
	self.buffers[key] = v
	return v
end

function State.drag(self: any, label: string, value: number, opts: any): (number, boolean)
	local v = imgui.dragFloat(label, value, opts)
	return v, v ~= value
end

function State.combo(self: any, key: any, label: string, items: { string }, current: string?, onChanged: any?): (number, string?)
	if current ~= nil then
		local found = nil
		for i, item in ipairs(items) do
			if item == current then
				found = i - 1
				break
			end
		end
		if found ~= nil and self.combos[key] ~= found then
			self.combos[key] = found
		end
	end
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
