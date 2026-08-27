local Ser = require("./leditb_Ser")
local DocModule = require("./leditb_Doc")

local Project = {} :: any
Project.__index = Project

local DIR = "projects"
local ANIM_SUFFIX = ".anim.luau"

local function sanitize(name: any): boolean
	return type(name) == "string" and #name > 0 and name:match("^[%w%-_ ]+$") ~= nil
end

local function dirFor(name: any): string
	return DIR .. "/" .. name
end

function Project.new(deps: any)
	deps = deps or {}
	local p = setmetatable({}, Project)
	p.fs = deps.fs or geode.fs
	p.lunar = deps.lunar or lunar
	return p
end

function Project:list(): ({string}?, string?)
	local entries, err = self.fs.list("save", DIR)
	if entries == nil then
		return nil, err
	end
	local out = {}
	for _, name in ipairs(entries) do
		if sanitize(name)
			and self.fs.exists("save", dirFor(name) .. "/" .. name .. ".rig.luau")
		then
			out[#out + 1] = name
		end
	end
	table.sort(out)
	return out, nil
end

function Project:create(name: any): (boolean, string?)
	if not sanitize(name) then
		return false, "invalid project name"
	end
	local dir = dirFor(name)
	if self.fs.exists("save", dir) then
		return false, "project exists: " .. name
	end
	local ok, err = self.fs.mkdir("save", dir)
	if not ok then
		return false, err
	end
	ok, err = self.fs.write("save", dir .. "/" .. name .. ".rig.luau", Ser.spec({ nodes = {} }))
	if not ok then
		return false, err
	end
	return true, nil
end

local function evalSource(src: any, chunk: any): (any, string?)
	local fn, err = loadstring(src, chunk)
	if fn == nil then
		return nil, err
	end
	local ok, result = pcall(fn)
	if not ok then
		return nil, result
	end
	return result, nil
end

function Project:load(name: any): (any, string?)
	if not sanitize(name) then
		return nil, "invalid project name"
	end
	local dir = dirFor(name)
	local src, err = self.fs.read("save", dir .. "/" .. name .. ".rig.luau")
	if src == nil then
		return nil, err
	end
	local spec, evalErr = evalSource(src, "=project/" .. name .. "/rig")
	if type(spec) ~= "table" or type(spec.nodes) ~= "table" then
		return nil, "invalid rig file: " .. tostring(evalErr)
	end
	local entries, lerr = self.fs.list("save", dir)
	if entries == nil then
		return nil, lerr
	end
	table.sort(entries)
	local animations = {}
	for _, file in ipairs(entries) do
		if file:sub(-#ANIM_SUFFIX) == ANIM_SUFFIX then
			local animName = file:sub(1, -(#ANIM_SUFFIX + 1))
			local asrc, aerr = self.fs.read("save", dir .. "/" .. file)
			if asrc == nil then
				return nil, aerr
			end
			local def, derr = evalSource(asrc, "=project/" .. name .. "/" .. animName)
			if type(def) ~= "table" then
				return nil, "invalid anim file " .. animName .. ": " .. tostring(derr)
			end
			local ok, verr = self.lunar.animation.load(def)
			if ok == nil then
				return nil, animName .. ": " .. tostring(verr)
			end
			animations[animName] = def
		end
	end
	return DocModule.restore({ lunar = self.lunar }, name, spec, animations)
end

function Project:save(doc: any): (boolean, string?)
	local name = doc.name
	if not sanitize(name) then
		return false, "invalid project name"
	end
	local dir = dirFor(name)
	self.fs.mkdir("save", dir)
	local ok, err = self.fs.write("save", dir .. "/" .. name .. ".rig.luau", Ser.spec(doc.rigSpec))
	if not ok then
		return false, err
	end
	local written = {}
	for animName, def in pairs(doc.animations) do
		if not sanitize(animName) then
			return false, "invalid animation name: " .. tostring(animName)
		end
		local file = animName .. ANIM_SUFFIX
		ok, err = self.fs.write("save", dir .. "/" .. file, Ser.anim(def))
		if not ok then
			return false, err
		end
		written[file] = true
	end

	local entries, lerr = self.fs.list("save", dir)
	if entries == nil then
		return false, lerr
	end
	for _, file in ipairs(entries) do
		if written[file] ~= true and file:sub(-#ANIM_SUFFIX) == ANIM_SUFFIX then
			self.fs.remove("save", dir .. "/" .. file)
		end
	end
	return true, nil
end

function Project:saveAs(doc: any, newName: any): (boolean, string?)
	if not sanitize(newName) then
		return false, "invalid project name"
	end
	if newName ~= doc.name and self.fs.exists("save", dirFor(newName)) then
		return false, "project exists: " .. newName
	end
	local old = doc.name
	doc.name = newName
	local ok, err = self:save(doc)
	if not ok then
		doc.name = old
	end
	return ok, err
end

function Project:delete(name: any): (boolean, string?)
	if not sanitize(name) then
		return false, "invalid project name"
	end
	local dir = dirFor(name)
	local entries, lerr = self.fs.list("save", dir)
	if entries == nil then
		return false, lerr
	end
	for _, file in ipairs(entries) do
		local ok, err = self.fs.remove("save", dir .. "/" .. file)
		if not ok then
			return false, err
		end
	end
	local ok, derr = self.fs.remove("save", dir)
	if not ok then
		return false, derr
	end
	return true, nil
end

return Project
