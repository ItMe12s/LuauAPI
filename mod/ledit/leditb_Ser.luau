local Ser = {}

local NODE_KEYS = {
	"id", "sprite", "parent",
	"x", "y", "rot", "sx", "sy", "skx", "sky", "z", "opacity", "ax", "ay",
}
local POSE_KEYS = {
	"x", "y", "rot", "sx", "sy", "skx", "sky", "z", "opacity", "ax", "ay", "easing",
}

local function num(v: any): string
	if type(v) ~= "number" or v ~= v or v == math.huge or v == -math.huge then
		error("non-finite number", 0)
	end
	return string.format("%.14g", v)
end

local function str(v: any): string
	return string.format("%q", tostring(v))
end

function Ser.value(v: any): string
	local t = type(v)
	if v == nil then
		return "nil"
	elseif t == "boolean" then
		return tostring(v)
	elseif t == "number" then
		return num(v)
	elseif t == "string" then
		return str(v)
	end
	error("unsupported type: " .. t, 0)
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
				if key == "sprite" or key == "parent" then
					rendered = str(v)
				else
					rendered = num(v)
				end
				parts[#parts + 1] = key .. " = " .. rendered
			end
		end
		out[#out + 1] = "\t\t{ " .. table.concat(parts, ", ") .. " },"
	end
	out[#out + 1] = "\t},"
	out[#out + 1] = "}"
	return table.concat(out, "\n")
end

local function eventLine(kf: any, indent: string): string?
	local events = kf.events
	if events == nil then
		return nil
	end
	if type(events) == "string" then
		events = { events }
	end
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
		out[#out + 1] = "\tlooped = " .. tostring(def.looped == true) .. ","
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
