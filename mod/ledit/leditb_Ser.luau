local Ser = {}

local Core = require("./leditb_DocCore")

local NODE_KEYS = { "id", "sprite", "parent" }
for _, k in ipairs(Core.CHANNEL_KEYS) do
	NODE_KEYS[#NODE_KEYS + 1] = k
end
local POSE_KEYS = {}
for _, k in ipairs(Core.CHANNEL_KEYS) do
	POSE_KEYS[#POSE_KEYS + 1] = k
end
POSE_KEYS[#POSE_KEYS + 1] = "easing"

local function num(v: any): string
	if type(v) ~= "number" then
		error("expected number", 0)
	end
	if not math.isfinite(v) then
		error("non-finite number", 0)
	end
	return string.format("%.14g", v)
end

local function str(v: any): string
	local s = tostring(v)
	local out = { '"' }
	for i = 1, #s do
		local c = s:sub(i, i)
		local b = c:byte()
		if c == "\\" then
			out[#out + 1] = "\\\\"
		elseif c == '"' then
			out[#out + 1] = '\\"'
		elseif c == "\n" then
			out[#out + 1] = "\\n"
		elseif c == "\r" then
			out[#out + 1] = "\\r"
		elseif c == "\t" then
			out[#out + 1] = "\\t"
		elseif b < 32 or b == 127 then
			out[#out + 1] = string.format("\\%03d", b)
		else
			out[#out + 1] = c
		end
	end
	out[#out + 1] = '"'
	return table.concat(out)
end

local function inlinePose(pose: any): string
	local parts = {}
	for _, key in ipairs(POSE_KEYS) do
		local v = pose[key]
		if v ~= nil then
			parts[#parts + 1] = key .. " = " .. (key == "easing" and str(v) or num(v))
		end
	end
	if #parts == 0 then
		return "{}"
	end
	return "{ " .. table.concat(parts, ", ") .. " }"
end

function Ser.spec(spec: any): string
	local known = {}
	for _, key in ipairs(NODE_KEYS) do
		known[key] = true
	end
	local out = { "return {", "\tnodes = {" }
	for i, node in ipairs(spec.nodes or {}) do
		if type(node) ~= "table" or type(node.id) ~= "string" or node.id == "" then
			error("node " .. i .. ": id required", 0)
		end
		local parts = {}
		for _, key in ipairs(NODE_KEYS) do
			local v = node[key]
			if v ~= nil then
				local rendered
				if key == "sprite" or key == "parent" or key == "id" then
					rendered = str(v)
				else
					rendered = num(v)
				end
				parts[#parts + 1] = key .. " = " .. rendered
			end
		end
		-- Pass through unknown scalar fields so they survive save/load.
		local extras = {}
		for k, v in pairs(node) do
			local t = type(k)
			local tv = type(v)
			if not known[k] and t == "string"
				and (tv == "number" or tv == "string" or tv == "boolean") then
				extras[#extras + 1] = k
			end
		end
		table.sort(extras)
		for _, key in ipairs(extras) do
			local v = node[key]
			local rendered
			if type(v) == "number" then
				rendered = num(v)
			elseif type(v) == "boolean" then
				rendered = tostring(v)
			else
				rendered = str(v)
			end
			parts[#parts + 1] = key .. " = " .. rendered
		end
		out[#out + 1] = "\t\t{ " .. table.concat(parts, ", ") .. " },"
	end
	out[#out + 1] = "\t},"
	out[#out + 1] = "}"
	return table.concat(out, "\n")
end

function Ser.eventArray(events: any): {any}
	if events == nil then
		return {}
	end
	if type(events) == "string" then
		return { events }
	end
	return events
end

local function eventLine(kf: any, indent: string): string?
	if kf.events == nil then
		return nil
	end
	local events = Ser.eventArray(kf.events)
	local items = {}
	for _, name in ipairs(events) do
		if type(name) ~= "string" then
			error("event names must be strings", 0)
		end
		items[#items + 1] = str(name)
	end
	return indent .. "events = { " .. table.concat(items, ", ") .. " },"
end

local function sortedFrames(keyframes: any): {number}
	local frames = {}
	for f in pairs(keyframes) do
		if type(f) ~= "number" or f % 1 ~= 0 or f < 0 then
			error("keyframe frames must be integers >= 0", 0)
		end
		frames[#frames + 1] = f
	end
	table.sort(frames)
	return frames
end

function Ser.anim(def: any): string
	local out = { "return {" }
	if def.fps ~= nil then
		out[#out + 1] = "\tfps = " .. num(def.fps) .. ","
	end
	if def.looped ~= nil then
		if type(def.looped) ~= "boolean" then
			error("looped must be a boolean", 0)
		end
		out[#out + 1] = "\tlooped = " .. tostring(def.looped) .. ","
	end
	local keyframes = def.keyframes or {}
	local frames = sortedFrames(keyframes)
	if #frames > 0 then
		out[#out + 1] = "\tkeyframes = {"
		for _, f in ipairs(frames) do
			local kf = keyframes[f]
			local ids = {}
			for id in pairs(kf) do
				if id ~= "events" then
					if type(id) ~= "string" or id == "" then
						error("pose keys must be non-empty strings", 0)
					end
					ids[#ids + 1] = id
				end
			end
			table.sort(ids)
			out[#out + 1] = "\t\t[" .. num(f) .. "] = {"
			for _, id in ipairs(ids) do
				out[#out + 1] = "\t\t\t[" .. str(id) .. "] = " .. inlinePose(kf[id]) .. ","
			end
			local events = eventLine(kf, "\t\t\t")
			if events ~= nil then
				out[#out + 1] = events
			end
			out[#out + 1] = "\t\t},"
		end
		out[#out + 1] = "\t},"
	end
	out[#out + 1] = "}"
	return table.concat(out, "\n")
end

return Ser
