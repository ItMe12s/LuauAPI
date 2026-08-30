local Ser = require("./leditb_Ser")
local Core = require("./leditb_DocCore")
local DocModule = require("./leditb_Doc")

local Project = {} :: any
Project.__index = Project

local DIR = "projects"
local ANIM_SUFFIX = ".anim.luau"

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
		if Core.validName(name)
			and self.fs.exists("save", dirFor(name) .. "/" .. name .. ".rig.luau")
		then
			out[#out + 1] = name
		end
	end
	table.sort(out)
	return out, nil
end

function Project:create(name: any): (boolean, string?)
	if not Core.validName(name) then
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
		self.fs.remove("save", dir)
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

local function readAnimFiles(fs: any, lunar: any, dir: string): ({string:any}?, {string}?)
	local animations = {}
	local errors = {}
	local entries, lerr = fs.list("save", dir)
	if entries == nil then
		return nil, { lerr }
	end
	table.sort(entries)
	for _, file in ipairs(entries) do
		if file:sub(-#ANIM_SUFFIX) == ANIM_SUFFIX then
			local animName = file:sub(1, -(#ANIM_SUFFIX + 1))
			if not Core.validName(animName) then
				errors[#errors + 1] = "invalid anim file name: " .. file
			else
				local asrc, aerr = fs.read("save", dir .. "/" .. file)
				if asrc == nil then
					errors[#errors + 1] = tostring(aerr)
				else
					local def, derr = evalSource(asrc, "=project/" .. dir .. "/" .. animName)
					if type(def) ~= "table" then
						errors[#errors + 1] = "invalid anim file " .. animName .. ": " .. tostring(derr)
					elseif type(def.keyframes) ~= "table" and def.keyframes ~= nil then
						errors[#errors + 1] = "invalid anim file " .. animName .. ": keyframes must be a table"
					else
						if def.keyframes == nil then
							def.keyframes = {}
						end
						local loaded, verr = lunar.animation.load(def)
						if loaded == nil then
							errors[#errors + 1] = animName .. ": " .. tostring(verr)
						elseif not pcall(Ser.anim, def) then
							errors[#errors + 1] = "invalid anim file " .. animName .. ": serialization failed"
						else
							animations[animName] = def
						end
					end
				end
			end
		end
	end
	if #errors > 0 then
		return animations, errors
	end
	return animations, nil
end

function Project:load(name: any): (any, string?)
	if not Core.validName(name) then
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
	local rigOk, rigErr = pcall(Ser.spec, spec)
	if not rigOk then
		return nil, "invalid rig file: " .. tostring(rigErr)
	end
	local animations, errors = readAnimFiles(self.fs, self.lunar, dir)
	if errors ~= nil then
		return nil, errors[1]
	end
	return DocModule.restore({ lunar = self.lunar }, name, spec, animations)
end

function Project:validate(name: any): (boolean, {string}?)
	local errors = {}
	if not Core.validName(name) then
		errors[1] = "invalid project name: " .. tostring(name)
		return false, errors
	end
	local dir = dirFor(name)
	local src = self.fs.read("save", dir .. "/" .. name .. ".rig.luau")
	if src == nil then
		errors[#errors + 1] = name .. ".rig.luau: missing or unreadable"
	else
		local spec, evalErr = evalSource(src, "=check/" .. name .. "/rig")
		if type(spec) ~= "table" or type(spec.nodes) ~= "table" then
			errors[#errors + 1] = name .. ".rig.luau: " .. tostring(evalErr)
		elseif not pcall(Ser.spec, spec) then
			errors[#errors + 1] = name .. ".rig.luau: serialization failed"
		end
	end
	local entries = self.fs.list("save", dir)
	if entries == nil then
		errors[#errors + 1] = "project folder unreadable"
		return false, errors
	end
	local _, animErrs = readAnimFiles(self.fs, self.lunar, dir)
	if animErrs ~= nil then
		for _, e in ipairs(animErrs) do
			errors[#errors + 1] = e
		end
	end
	if #errors > 0 then
		return false, errors
	end
	return true, nil
end

function Project:save(doc: any): (boolean, string?)
	local name = doc.name
	if not Core.validName(name) then
		return false, "invalid project name"
	end
	local rigOk, rigSrc = pcall(Ser.spec, doc.rigSpec)
	if not rigOk then
		return false, tostring(rigSrc)
	end
	local animSrcs = {}
	for animName, def in pairs(doc.animations) do
		if not Core.validName(animName) then
			return false, "invalid animation name: " .. tostring(animName)
		end
		local animOk, src = pcall(Ser.anim, def)
		if not animOk then
			return false, tostring(src)
		end
		animSrcs[animName] = src
	end
	local dir = dirFor(name)
	self.fs.mkdir("save", dir)

	local files = { [name .. ".rig.luau"] = rigSrc }
	for animName, src in pairs(animSrcs) do
		files[animName .. ANIM_SUFFIX] = src
	end

	local prev = {}
	for file in pairs(files) do
		prev[file] = self.fs.read("save", dir .. "/" .. file)
	end
	for file, src in pairs(files) do
		local ok, err = self.fs.write("save", dir .. "/" .. file, src)
		if not ok then
			for f, old in pairs(prev) do
				if old ~= nil then
					self.fs.write("save", dir .. "/" .. f, old)
				else
					self.fs.remove("save", dir .. "/" .. f)
				end
			end
			return false, err
		end
	end

	local entries, lerr = self.fs.list("save", dir)
	if entries == nil then
		return false, lerr
	end
	for _, file in ipairs(entries) do
		if files[file] == nil and file:sub(-#ANIM_SUFFIX) == ANIM_SUFFIX then
			self.fs.remove("save", dir .. "/" .. file)
		end
	end
	return true, nil
end

function Project:saveAs(doc: any, newName: any): (boolean, string?)
	if not Core.validName(newName) then
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
	if not Core.validName(name) then
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
