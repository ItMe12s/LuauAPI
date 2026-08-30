local State = {} :: any

State.RED = { x = 0.9, y = 0.25, z = 0.25, w = 1 }

-- Channel keys are canonical in leditb_DocCore, this table adds UI metadata.
local CHANNEL_META: { any } = {
	{ speed = 1, tip = "position x" },
	{ speed = 1, tip = "position y" },
	{ speed = 0.5, tip = "rotation in degrees" },
	{ speed = 0.02, tip = "scale x" },
	{ speed = 0.02, tip = "scale y" },
	{ speed = 0.02, tip = "skew x" },
	{ speed = 0.02, tip = "skew y" },
	{ fmt = "%.0f", tip = "render order" },
	{ speed = 2, min = 0, max = 255, fmt = "%.0f", tip = "opacity 0 to 255" },
	{ speed = 0.01, min = 0, max = 1, tip = "anchor x" },
	{ speed = 0.01, min = 0, max = 1, tip = "anchor y" },
}

State.CHANNELS = {}
for i, key in ipairs(require("./leditb_DocCore").CHANNEL_KEYS) do
	local meta = CHANNEL_META[i]
	meta.k = key
	State.CHANNELS[i] = meta
end

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
		caches = {},
	}, { __index = State })
end

function State.tip(t: string)
	if imgui.isItemHovered() then
		imgui.setTooltip(t)
	end
end

function State.cached(self: any, key: string, doc: any, build: any): any
	local c = self.caches[key]
	if c ~= nil and c.doc == doc and c.rev == doc.rev then
		return c.value
	end
	local value = build()
	self.caches[key] = { doc = doc, rev = doc.rev, value = value }
	return value
end

function State.channelDrags(self: any, winId: string, source: any, onChannel: any)
	for _, ch in ipairs(self.CHANNELS) do
		local cur = source[ch.k]
		if cur == nil then
			cur = self.FALLBACK[ch.k]
		end
		local v, changed = self:drag(ch.k, cur, {
			speed = ch.speed,
			min = ch.min,
			max = ch.max,
			fmt = ch.fmt,
		})
		if changed then
			self:act(winId, function()
				return onChannel(ch, v)
			end)
		end
		State.tip(ch.tip)
	end
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
