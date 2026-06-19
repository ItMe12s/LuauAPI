local Executor = {}

Executor.spacing = 10
Executor.footer = 72
Executor.windowWidth = 460
Executor.windowHeight = 360
Executor.buttonColumns = 3
Executor.ranCount = 0
Executor.code = "print('hello from executor :3')"
Executor.status = ""
Executor.initialized = false
Executor.drawHandle = nil :: ImGuiDrawHandle?
Executor.font = nil :: ImGuiFontHandle?
Executor.enabled = geode.Mod.getSettingValue("enable-executor")

local function getThemeOpts(): ImGuiStyleOpts
    return {
        windowRounding = 8,
        childRounding = 6,
        frameRounding = 6,
        popupRounding = 6,
        scrollbarRounding = 6,
        grabRounding = 6,
        windowBorderSize = 0,
        frameBorderSize = 0,
        windowPadding = { x = 16, y = 14 },
        framePadding = { x = 12, y = 8 },
        itemSpacing = { x = 10, y = 10 },
        colors = {
            [imgui.Col.Text] = { x = 1.0, y = 1.0, z = 1.0, w = 1.0 },
            [imgui.Col.TextDisabled] = { x = 0.45, y = 0.45, z = 0.45, w = 1.0 },
            [imgui.Col.WindowBg] = { x = 0.10, y = 0.10, z = 0.10, w = 1.0 },
            [imgui.Col.ChildBg] = { x = 0.12, y = 0.12, z = 0.12, w = 1.04 },
            [imgui.Col.Border] = { x = 0.25, y = 0.25, z = 0.25, w = 0.40 },
            [imgui.Col.FrameBg] = { x = 0.14, y = 0.14, z = 0.14, w = 1.0 },
            [imgui.Col.FrameBgHovered] = { x = 0.18, y = 0.18, z = 0.18, w = 1.0 },
            [imgui.Col.FrameBgActive] = { x = 0.22, y = 0.22, z = 0.22, w = 1.0 },
            [imgui.Col.TitleBg] = { x = 0.08, y = 0.08, z = 0.08, w = 1.0 },
            [imgui.Col.TitleBgActive] = { x = 0.12, y = 0.12, z = 0.12, w = 1.0 },
            [imgui.Col.Button] = { x = 0.20, y = 0.20, z = 0.20, w = 0.95 },
            [imgui.Col.ButtonHovered] = { x = 0.30, y = 0.30, z = 0.30, w = 1.0 },
            [imgui.Col.ButtonActive] = { x = 0.38, y = 0.38, z = 0.38, w = 1.0 },
            [imgui.Col.Header] = { x = 0.20, y = 0.20, z = 0.20, w = 0.75 },
            [imgui.Col.HeaderHovered] = { x = 0.28, y = 0.28, z = 0.28, w = 0.90 },
            [imgui.Col.HeaderActive] = { x = 0.35, y = 0.35, z = 0.35, w = 1.0 },
            [imgui.Col.ScrollbarBg] = { x = 0.08, y = 0.08, z = 0.08, w = 0.60 },
            [imgui.Col.ScrollbarGrab] = { x = 0.28, y = 0.28, z = 0.28, w = 0.80 },
            [imgui.Col.ScrollbarGrabHovered] = { x = 0.38, y = 0.38, z = 0.38, w = 0.90 },
            [imgui.Col.ScrollbarGrabActive] = { x = 0.48, y = 0.48, z = 0.48, w = 1.0 },
        },
    } :: ImGuiStyleOpts
end

function Executor.setStatus(successful: boolean, output: string)
    local result
    if successful then
        result = "Success: " .. output
    else
        result = "Error: " .. output
    end
    Executor.status = "Status (#" .. Executor.ranCount .. ") " .. result
end

function Executor.execute(code: string)
    if code == "" then
        Executor.setStatus(false, "Code is empty")
        return
    end

    local fn, err = loadstring(code, "=executor")
    if not fn then
        Executor.setStatus(false, tostring(err))
        return
    end

    Executor.ranCount += 1

    task.defer(function()
        local ok, res = pcall(fn)
        if ok then
            local output = if res == nil then "(no return value)" else tostring(res)
            Executor.setStatus(true, output)
        else
            Executor.setStatus(false, tostring(res))
        end
    end)
end

function Executor.draw()
    local function drawContent()
        imgui.style.with(getThemeOpts(), function()
            imgui.window("Executor", function()
                local avail = imgui.getContentRegionAvail()
                local buttonWidth = math.max(avail.x / Executor.buttonColumns, 1) - Executor.spacing / 1.5

                Executor.code = imgui.inputTextMultiline("##code", Executor.code, { x = -1, y = -Executor.footer })

                if imgui.button("Execute", { size = { x = buttonWidth, y = 0 } }) then
                    Executor.execute(Executor.code)
                end

                imgui.sameLine()
                if imgui.button("Paste", { size = { x = buttonWidth, y = 0 } }) then
                    local paste = geode.utils.clipboard.read()
                    if paste then
                        Executor.code = paste
                    end
                end

                imgui.sameLine()
                if imgui.button("Clear", { size = { x = buttonWidth, y = 0 } }) then
                    Executor.code = ""
                    Executor.setStatus(true, "Code cleared")
                end

                imgui.text(Executor.status)
            end, { size = { x = Executor.windowWidth, y = Executor.windowHeight } })
        end)
    end

    if Executor.font then
        imgui.font.with(Executor.font, drawContent)
    else
        drawContent()
    end
end

function Executor.init()
    if Executor.initialized then return end
    Executor.initialized = true

    Executor.font = imgui.font.add("resources", "JetBrainsMono-Regular.ttf", 16)
    Executor.drawHandle = imgui.onDraw(function()
        if not Executor.enabled then return end
        Executor.draw()
    end)
    Executor.setStatus(true, "Executor is ready")
end

geode.Mod.listenForSettingChanges("enable-executor", function(value)
    Executor.enabled = value
    if value then
        Executor.init()
    end
end)

return Executor
