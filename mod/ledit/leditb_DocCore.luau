local Core = {}

local NAME_PATTERN = "^[%w%-_ ]+$"

Core.CHANNEL_KEYS = {
	"x", "y", "rot", "sx", "sy", "skx", "sky", "z", "opacity", "ax", "ay",
}

Core.BASE_CHANNELS = {}
for _, k in ipairs(Core.CHANNEL_KEYS) do
	Core.BASE_CHANNELS[k] = true
end

function Core.clone(value: any): any
	if type(value) ~= "table" then
		return value
	end
	local out = {}
	for k, v in pairs(value) do
		out[k] = Core.clone(v)
	end
	return out
end

function Core.finite(v: any): boolean
	return type(v) == "number" and math.isfinite(v)
end

function Core.validName(name: any): boolean
	return type(name) == "string" and #name > 0 and name:match(NAME_PATTERN) ~= nil
end

function Core.validFrame(f: any): boolean
	return type(f) == "number" and f >= 0 and f % 1 == 0
end

function Core.findNode(d: any, id: any): (any, any)
	for i, node in ipairs(d.rigSpec.nodes) do
		if node.id == id then
			return node, i
		end
	end
	return nil, nil
end

function Core.orderedSpec(spec: any): any
	local nodes = spec.nodes or {}
	local out = {}
	local seen = {}
	local function visit(node: any, depth: number)
		if depth > 64 or seen[node.id] then
			return
		end
		seen[node.id] = true
		if node.parent ~= nil then
			for _, n in ipairs(nodes) do
				if n.id == node.parent then
					visit(n, depth + 1)
					break
				end
			end
		end
		out[#out + 1] = Core.clone(node)
	end
	for _, node in ipairs(nodes) do
		if type(node) == "table" and type(node.id) == "string" then
			visit(node, 0)
		end
	end
	local ordered = {}
	for k, v in pairs(spec) do
		ordered[k] = v
	end
	ordered.nodes = out
	return ordered
end

function Core.rebuildRig(d: any): (any, string?)
	for i, n in ipairs(d.rigSpec.nodes) do
		if type(n) ~= "table" or type(n.id) ~= "string" or n.id == "" then
			return nil, "node " .. i .. ": id required"
		end
	end
	local rig, err = d.lunar.rig.new():load(Core.orderedSpec(d.rigSpec))
	d.track = nil
	if rig == nil then
		d.rig = nil
		d.rigWarn = nil
		return nil, err
	end
	d.rig = rig
	local live = {}
	for _, id in ipairs(rig:listNodes()) do
		live[id] = true
	end
	local missing = {}
	for _, n in ipairs(d.rigSpec.nodes) do
		if not live[n.id] then
			table.insert(missing, n.id)
		end
	end
	d.rigWarn = if #missing > 0
		then "nodes skipped (sprite failed to load?): " .. table.concat(missing, ", ")
		else nil
	return rig, nil
end

function Core.recompile(d: any): (boolean, string?)
	if d.activeAnim == nil or d.animations[d.activeAnim] == nil then
		d.track = nil
		return true, nil
	end
	if d.rig == nil then
		local ok, err = Core.rebuildRig(d)
		if not ok then
			return false, err
		end
	end
	local def, err = d.lunar.animation.load(d.animations[d.activeAnim])
	if not def then
		return false, err
	end
	local wasPlaying = d.track ~= nil and d.track:isPlaying() or false
	local track, terr = d.rig:loadAnimation(def)
	if not track then
		return false, terr
	end
	d.track = track
	track:setSpeed(d.speed)
	track:seek(math.clamp(d.playhead, 0, track:duration()))
	if track:duration() > 0 then
		d.playhead = track:currentTime()
	end
	if wasPlaying then
		track:play()
	end
	return true, nil
end

function Core.swapRig(d: any): (boolean, string?)
	local wasPlaying = d.track ~= nil and d.track:isPlaying() or false
	local ok, err = Core.rebuildRig(d)
	if not ok then
		return false, err
	end
	ok, err = Core.recompile(d)
	if not ok then
		return false, err
	end
	if wasPlaying and d.track ~= nil then
		d.track:play()
	end
	return true, nil
end

function Core.activeDef(d: any): (any, string?)
	if d.activeAnim ~= nil then
		local anim = d.animations[d.activeAnim]
		if anim ~= nil then
			return anim, nil
		end
	end
	return nil, "no active animation"
end

function Core.snapshot(d: any): any
	return {
		full = true,
		spec = Core.clone(d.rigSpec),
		anims = Core.clone(d.animations),
		active = d.activeAnim,
		head = d.playhead,
	}
end

function Core.snapshotAnim(d: any, name: any): any
	return {
		anims = { [name] = Core.clone(d.animations[name]) or false },
		active = d.activeAnim,
		head = d.playhead,
	}
end

local function restoreState(d: any, s: any)
	if s.spec ~= nil then
		d.rigSpec = Core.clone(s.spec)
	end
	if s.anims ~= nil then
		if s.full then
			d.animations = Core.clone(s.anims)
		else
			for name, anim in pairs(s.anims) do
				d.animations[name] = if anim == false then nil else Core.clone(anim)
			end
		end
	end
	d.activeAnim = s.active
	d.playhead = s.head
	d.rev += 1
end

function Core.replay(d: any, s: any)
	d.replaying = true
	restoreState(d, s)
	local ok, err
	if s.spec ~= nil then
		ok, err = Core.swapRig(d)
	else
		ok, err = Core.recompile(d)
	end
	d.replaying = nil
	if not ok then
		error(err or "undo replay failed", 0)
	end
end

function Core.pushUndo(d: any, ukey: any, before: any)
	if d.replaying then
		return
	end
	local after
	if not before.full then
		after = Core.snapshotAnim(d, (next(before.anims)))
	else
		after = Core.snapshot(d)
	end
	d.undoStack:push(ukey, function()
		Core.replay(d, before)
	end, function()
		Core.replay(d, after)
	end)
	d.rev += 1
end

function Core.finishOp(d: any, ukey: any, before: any, ok: any, err: any): (boolean, string?)
	if not ok then
		restoreState(d, before)
		return false, err
	end
	Core.pushUndo(d, ukey, before)
	return true, nil
end

return Core
