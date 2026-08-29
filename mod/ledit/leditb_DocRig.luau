local Core = require("./leditb_DocCore")

local clone = Core.clone
local findNode = Core.findNode
local finite = Core.finite
local swapRig = Core.swapRig
local validName = Core.validName
local BASE_CHANNELS = Core.BASE_CHANNELS

local M = {}

local snapshot = Core.snapshot
local finishOp = Core.finishOp

function M.addNode(d: any, entry: any, ukey: any?): (boolean, string?)
	if type(entry) ~= "table" then
		return false, "node table required"
	end
	if not validName(entry.id) then
		return false, "invalid node id"
	end
	if findNode(d, entry.id) ~= nil then
		return false, "duplicate node id: " .. entry.id
	end
	for k, v in pairs(entry) do
		if BASE_CHANNELS[k] and not finite(v) then
			return false, "channel " .. k .. " must be a finite number"
		end
	end
	local before = snapshot(d)
	table.insert(d.rigSpec.nodes, clone(entry))
	return finishOp(d, ukey, before, swapRig(d))
end

function M.deleteNode(d: any, id: any, ukey: any?): (boolean, string?)
	local node, index = findNode(d, id)
	if index == nil then
		return false, "no such node: " .. tostring(id)
	end
	local before = snapshot(d)
	for _, n in ipairs(d.rigSpec.nodes) do
		if n.parent == id then
			n.parent = node.parent
		end
	end
	table.remove(d.rigSpec.nodes, index)
	return finishOp(d, ukey, before, swapRig(d))
end

function M.renameNode(d: any, old: any, new: any, ukey: any?): (boolean, string?)
	if not validName(new) then
		return false, "invalid node id"
	end
	if old == new then
		return true, nil
	end
	local node = findNode(d, old)
	if node == nil then
		return false, "no such node: " .. tostring(old)
	end
	if findNode(d, new) ~= nil then
		return false, "duplicate node id: " .. new
	end
	local before = snapshot(d)
	node.id = new
	for _, n in ipairs(d.rigSpec.nodes) do
		if n.parent == old then
			n.parent = new
		end
	end
	for _, anim in pairs(d.animations) do
		for _, kf in pairs(anim.keyframes or {}) do
			local pose = kf[old]
			if pose ~= nil then
				kf[new] = pose
				kf[old] = nil
			end
		end
	end
	return finishOp(d, ukey, before, swapRig(d))
end

function M.reparent(d: any, id: any, parent: any?, ukey: any?): (boolean, string?)
	local node = findNode(d, id)
	if node == nil then
		return false, "no such node: " .. tostring(id)
	end
	if node.parent == parent then
		return true, nil
	end
	if parent ~= nil then
		if findNode(d, parent) == nil then
			return false, "no such parent: " .. tostring(parent)
		end
		local walked = {}
		local cur = parent
		while cur ~= nil do
			if walked[cur] then
				break
			end
			walked[cur] = true
			if cur == id then
				return false, "reparent cycle: " .. id
			end
			local pn = findNode(d, cur)
			cur = pn ~= nil and pn.parent or nil
		end
	end
	local before = snapshot(d)
	node.parent = parent
	return finishOp(d, ukey, before, swapRig(d))
end

function M.setSprite(d: any, id: any, sprite: any?, ukey: any?): (boolean, string?)
	local node = findNode(d, id)
	if node == nil then
		return false, "no such node: " .. tostring(id)
	end
	if sprite ~= nil and type(sprite) ~= "string" then
		return false, "sprite must be a string or nil"
	end
	local before = snapshot(d)
	node.sprite = sprite
	return finishOp(d, ukey, before, swapRig(d))
end

function M.setBase(d: any, id: any, channels: any, ukey: any?): (boolean, string?)
	local node = findNode(d, id)
	if node == nil then
		return false, "no such node: " .. tostring(id)
	end
	if type(channels) ~= "table" then
		return false, "channels table required"
	end
	for k, v in pairs(channels) do
		if not BASE_CHANNELS[k] then
			return false, "unknown channel: " .. tostring(k)
		end
		if not finite(v) then
			return false, "channel " .. k .. " must be a finite number"
		end
	end
	local before = snapshot(d)
	for k, v in pairs(channels) do
		node[k] = v
	end
	return finishOp(d, ukey, before, swapRig(d))
end

return M
