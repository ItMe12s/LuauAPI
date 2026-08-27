local Core = {}

local NAME_PATTERN = "^[%w%-_ ]+$"

Core.BASE_CHANNELS = {
	x = true, y = true, rot = true, sx = true, sy = true,
	skx = true, sky = true, z = true, opacity = true, ax = true, ay = true,
}

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
	return type(v) == "number" and v == v and v ~= math.huge and v ~= -math.huge
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
		if type(node) == "table" and type(node.id) == "string" and not seen[node.id] then
			visit(node, 0)
		else
			out[#out + 1] = Core.clone(node)
		end
	end
	local ordered = Core.clone(spec)
	ordered.nodes = out
	return ordered
end

function Core.rebuildRig(d: any): (any, string?)
	local rig, err = d.lunar.rig.new():load(Core.orderedSpec(d.rigSpec))
	d.track = nil
	if rig == nil then
		d.rig = nil
		return false, err
	end
	d.rig = rig
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
	d.playhead = track:currentTime()
	if wasPlaying then
		track:play()
	end
	return true, nil
end

function Core.swapRig(d: any): (boolean, string?)
	local ok, err = Core.rebuildRig(d)
	if not ok then
		return false, err
	end
	ok, err = Core.recompile(d)
	if not ok then
		return false, err
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
	return false, "no active animation"
end

function Core.snapshot(d: any): any
	return {
		spec = Core.clone(d.rigSpec),
		anims = Core.clone(d.animations),
		active = d.activeAnim,
		head = d.playhead,
	}
end

function Core.restoreSnap(d: any, s: any)
	d.rigSpec = s.spec
	d.animations = s.anims
	d.activeAnim = s.active
	d.playhead = s.head
	Core.swapRig(d)
end

function Core.replay(d: any, s: any)
	d.replaying = true
	Core.restoreSnap(d, s)
	d.replaying = nil
end

function Core.pushUndo(d: any, ukey: any, before: any)
	if d.replaying or d.undoStack == nil then
		return
	end
	local after = Core.snapshot(d)
	d.undoStack:push(ukey, function()
		Core.replay(d, before)
	end, function()
		Core.replay(d, after)
	end)
end

function Core.finishOp(d: any, ukey: any, before: any, ok: any, err: any): (boolean, string?)
	if not ok then
		return false, err
	end
	Core.pushUndo(d, ukey, before)
	return true, nil
end

return Core
