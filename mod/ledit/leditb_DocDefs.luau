local Core = require("./leditb_DocCore")

local activeDef = Core.activeDef
local clone = Core.clone
local finite = Core.finite
local finishOp = Core.finishOp
local recompile = Core.recompile
local snapshot = Core.snapshot
local validFrame = Core.validFrame
local validName = Core.validName

local M = {}

function M.addAnim(d: any, name: any, def: any?, ukey: any?): (boolean, string?)
	if not validName(name) then
		return false, "invalid animation name"
	end
	if d.animations[name] ~= nil then
		return false, "animation exists: " .. name
	end
	local before = snapshot(d)
	def = def or { fps = 30, looped = false, keyframes = {} }
	d.animations[name] = clone(def)
	return finishOp(d, ukey, before, true, nil)
end

function M.deleteAnim(d: any, name: any, ukey: any?): (boolean, string?)
	if d.animations[name] == nil then
		return false, "no such animation: " .. tostring(name)
	end
	local before = snapshot(d)
	d.animations[name] = nil
	if d.activeAnim == name then
		if d.track ~= nil then
			d.track:stop()
		end
		d.activeAnim = nil
		d.playhead = 0
		d.track = nil
	end
	return finishOp(d, ukey, before, true, nil)
end

function M.renameAnim(d: any, old: any, new: any, ukey: any?): (boolean, string?)
	if d.animations[old] == nil then
		return false, "no such animation: " .. tostring(old)
	end
	if not validName(new) then
		return false, "invalid animation name"
	end
	if old ~= new and d.animations[new] ~= nil then
		return false, "animation exists: " .. new
	end
	local before = snapshot(d)
	d.animations[new] = d.animations[old]
	d.animations[old] = nil
	if d.activeAnim == old then
		d.activeAnim = new
	end
	return finishOp(d, ukey, before, true, nil)
end

function M.duplicateAnim(d: any, src: any, dst: any, ukey: any?): (boolean, string?)
	local origin = d.animations[src]
	if origin == nil then
		return false, "no such animation: " .. tostring(src)
	end
	if not validName(dst) then
		return false, "invalid animation name"
	end
	if d.animations[dst] ~= nil then
		return false, "animation exists: " .. dst
	end
	local before = snapshot(d)
	d.animations[dst] = clone(origin)
	return finishOp(d, ukey, before, true, nil)
end

function M.setActive(d: any, name: any, ukey: any?): (boolean, string?)
	if d.animations[name] == nil then
		return false, "no such animation: " .. tostring(name)
	end
	local before = snapshot(d)
	d.activeAnim = name
	d.playhead = 0
	return finishOp(d, ukey, before, recompile(d))
end

function M.setFps(d: any, n: any, ukey: any?): (boolean, string?)
	local anim, err = activeDef(d)
	if not anim then
		return false, err
	end
	if not finite(n) or n <= 0 then
		return false, "fps must be > 0"
	end
	local before = snapshot(d)
	anim.fps = n
	return finishOp(d, ukey, before, recompile(d))
end

function M.setLooped(d: any, b: any, ukey: any?): (boolean, string?)
	local anim, err = activeDef(d)
	if not anim then
		return false, err
	end
	local before = snapshot(d)
	anim.looped = b == true
	return finishOp(d, ukey, before, recompile(d))
end

local function validatePose(pose: any): string?
	if type(pose) ~= "table" then
		return "pose table required"
	end
	for k, v in pairs(pose) do
		if k == "easing" then
			if type(v) ~= "string" then
				return "easing must be a string"
			end
		elseif not Core.BASE_CHANNELS[k] then
			return "unknown pose channel: " .. tostring(k)
		elseif not finite(v) then
			return "channel " .. k .. " must be a finite number"
		end
	end
	return nil
end

local function eventArray(events: any): {any}
	if events == nil then
		return {}
	end
	if type(events) == "string" then
		return { events }
	end
	return events
end

local function mergeEvents(dst: any, incoming: {any})
	local events = dst.events
	if type(events) == "string" then
		events = { events }
	elseif events == nil then
		events = {}
	end
	local seen = {}
	for _, name in ipairs(events) do
		seen[name] = true
	end
	for _, name in ipairs(incoming) do
		if not seen[name] then
			events[#events + 1] = name
			seen[name] = true
		end
	end
	dst.events = events
end

function M.putPose(d: any, frame: any, id: any, pose: any, ukey: any?): (boolean, string?)
	local anim, err = activeDef(d)
	if not anim then
		return false, err
	end
	if not validFrame(frame) then
		return false, "frame must be an integer >= 0"
	end
	err = validatePose(pose)
	if err ~= nil then
		return false, err
	end
	local before = snapshot(d)
	local kf = anim.keyframes[frame]
	if kf == nil then
		kf = {}
		anim.keyframes[frame] = kf
	end
	local target = kf[id]
	if target == nil then
		target = {}
		kf[id] = target
	end
	for k, v in pairs(pose) do
		target[k] = v
	end
	return finishOp(d, ukey, before, recompile(d))
end

function M.clearPose(d: any, frame: any, id: any, ukey: any?): (boolean, string?)
	local anim, err = activeDef(d)
	if not anim then
		return false, err
	end
	if not validFrame(frame) then
		return false, "frame must be an integer >= 0"
	end
	local kf = anim.keyframes[frame]
	if kf ~= nil and kf[id] ~= nil then
		local before = snapshot(d)
		kf[id] = nil
		if next(kf) == nil then
			anim.keyframes[frame] = nil
		end
		return finishOp(d, ukey, before, recompile(d))
	end
	return true, nil
end

function M.removeFrame(d: any, frame: any, ukey: any?): (boolean, string?)
	local anim, err = activeDef(d)
	if not anim then
		return false, err
	end
	if not validFrame(frame) then
		return false, "frame must be an integer >= 0"
	end
	local before = snapshot(d)
	anim.keyframes[frame] = nil
	return finishOp(d, ukey, before, recompile(d))
end

function M.moveFrame(d: any, from: any, to: any, ukey: any?): (boolean, string?)
	local anim, err = activeDef(d)
	if not anim then
		return false, err
	end
	if not validFrame(from) or not validFrame(to) then
		return false, "frames must be integers >= 0"
	end
	local src = anim.keyframes[from]
	if src == nil then
		return false, "no keyframe at frame " .. tostring(from)
	end
	if from == to then
		return true, nil
	end
	local before = snapshot(d)
	local dst = anim.keyframes[to]
	if dst == nil then
		dst = {}
	end
	for key, value in pairs(src) do
		if key == "events" then
			mergeEvents(dst, eventArray(value))
		else
			dst[key] = value
		end
	end
	anim.keyframes[to] = dst
	anim.keyframes[from] = nil
	return finishOp(d, ukey, before, recompile(d))
end

function M.addEvent(d: any, frame: any, name: any, ukey: any?): (boolean, string?)
	local anim, err = activeDef(d)
	if not anim then
		return false, err
	end
	if not validFrame(frame) then
		return false, "frame must be an integer >= 0"
	end
	if type(name) ~= "string" or name == "" then
		return false, "event name required"
	end
	local before = snapshot(d)
	local kf = anim.keyframes[frame]
	if kf == nil then
		kf = {}
		anim.keyframes[frame] = kf
	end
	mergeEvents(kf, { name })
	return finishOp(d, ukey, before, recompile(d))
end

function M.removeEvent(d: any, frame: any, name: any, ukey: any?): (boolean, string?)
	local anim, err = activeDef(d)
	if not anim then
		return false, err
	end
	local kf = anim.keyframes[frame]
	if kf == nil or kf.events == nil then
		return true, nil
	end
	local kept = {}
	local removed = false
	for _, existing in ipairs(eventArray(kf.events)) do
		if existing == name then
			removed = true
		else
			kept[#kept + 1] = existing
		end
	end
	if removed then
		local before = snapshot(d)
		if #kept == 0 then
			kf.events = nil
		else
			kf.events = kept
		end
		if next(kf) == nil then
			anim.keyframes[frame] = nil
		end
		return finishOp(d, ukey, before, recompile(d))
	end
	return true, nil
end

function M.renameEvent(d: any, old: any, new: any, ukey: any?): (boolean, string?)
	local anim, err = activeDef(d)
	if not anim then
		return false, err
	end
	if type(new) ~= "string" or new == "" then
		return false, "event name required"
	end
	local before = snapshot(d)
	local changed = false
	for f, kf in pairs(anim.keyframes) do
		if kf.events ~= nil then
			local events = eventArray(kf.events)
			local seenNew = false
			for _, e in ipairs(events) do
				if e == new then
					seenNew = true
					break
				end
			end
			local kept = {}
			for _, e in ipairs(events) do
				if e == old then
					changed = true
					if not seenNew then
						kept[#kept + 1] = new
						seenNew = true
					end
				else
					kept[#kept + 1] = e
				end
			end
			if #kept == 0 then
				kf.events = nil
			else
				kf.events = kept
			end
			if next(kf) == nil then
				anim.keyframes[f] = nil
			end
		end
	end
	if changed then
		return finishOp(d, ukey, before, recompile(d))
	end
	return true, nil
end

function M.length(d: any): number
	local anim = d.animations[d.activeAnim]
	if anim == nil or anim.keyframes == nil then
		return 0
	end
	local maxFrame = 0
	for f in pairs(anim.keyframes) do
		if type(f) == "number" and f > maxFrame then
			maxFrame = f
		end
	end
	local fps = anim.fps
	if not finite(fps) or fps <= 0 then
		fps = 30
	end
	return maxFrame / fps
end

return M
