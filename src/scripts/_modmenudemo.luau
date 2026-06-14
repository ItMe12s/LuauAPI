local keys = geode.cocos.enumKeyCodes
local mods = geode.KeyboardModifier
local action = geode.KeyboardInputData.Action

local MAIN_W = 520
local MAIN_H = 420
local TAB_NAMES = { "Theme", "Controls", "Data", "Lists", "Settings" }

local presetItems = { "Compact", "Standard", "Expanded" }
local listItems = { "Item A", "Item B", "Item C", "Item D" }
local tableRows = {
    { name = "Alpha", value = "12", status = "On" },
    { name = "Beta", value = "8", status = "Off" },
    { name = "Gamma", value = "24", status = "On" },
}

local FLAG_A = 1
local FLAG_B = 2
local FLAG_C = 4

local state = {
    menuOpen = false,
    menuAnim = 0,
    activeTab = 1,
    prevTab = 1,
    tabSlide = 0,
    tabAnim = 1,
    lastFrameTime = os.clock(),
    accent = { x = 0.45, y = 0.55, z = 0.95, w = 1.0 },
    -- Theme
    brightness = 0.6,
    contrast = 1.0,
    preset = 1,
    surfaceColor = { x = 0.9, y = 0.4, z = 0.7, w = 1.0 },
    -- Controls
    enabled = false,
    sensitivity = 1.0,
    stepSize = 0,
    controlMode = 1,
    progress = 0.72,
    -- Data
    showBorders = true,
    refreshRate = 1.0,
    treeOpen = true,
    -- Lists
    listIndex = 0,
    listFlags = FLAG_A,
    selectedRow = 1,
    -- Settings
    showFloaters = true,
    -- Floating panels
    primary = {
        optionA = false,
        optionB = false,
        optionC = false,
        optionD = true,
        amount = 0.5,
    },
    secondary = {
        optionA = false,
        optionB = false,
        optionC = false,
        optionD = true,
        rate = 1.2,
    },
    tertiary = {
        optionA = false,
        optionB = false,
        optionC = false,
        optionD = true,
        blend = 0.85,
    },
}

local function stepAnim(current: number, target: number, dt: number, speed: number): number
    local step = math.min(1, dt * speed)
    return current + (target - current) * step
end

local function applyTheme()
    imgui.theme.applyCustom({
        windowRounding = 10,
        frameRounding = 6,
        tabRounding = 6,
        windowPadding = { x = 14, y = 12 },
        framePadding = { x = 10, y = 6 },
        itemSpacing = { x = 10, y = 8 },
        colors = {
            [imgui.Col.WindowBg] = { x = 0.08, y = 0.09, z = 0.12, w = 0.92 },
            [imgui.Col.ChildBg] = { x = 0.10, y = 0.11, z = 0.14, w = 0.88 },
            [imgui.Col.Border] = { x = 0.22, y = 0.24, z = 0.30, w = 0.45 },
            [imgui.Col.FrameBg] = { x = 0.14, y = 0.15, z = 0.19, w = 0.90 },
            [imgui.Col.FrameBgHovered] = { x = 0.18, y = 0.20, z = 0.26, w = 0.95 },
            [imgui.Col.TitleBg] = { x = 0.10, y = 0.11, z = 0.15, w = 1.0 },
            [imgui.Col.TitleBgActive] = { x = 0.12, y = 0.14, z = 0.20, w = 1.0 },
            [imgui.Col.Tab] = { x = 0.12, y = 0.13, z = 0.17, w = 0.90 },
            [imgui.Col.TabHovered] = { x = 0.20, y = 0.24, z = 0.34, w = 0.95 },
            [imgui.Col.TabSelected] = state.accent,
            [imgui.Col.Button] = { x = 0.20, y = 0.24, z = 0.36, w = 0.90 },
            [imgui.Col.ButtonHovered] = { x = 0.28, y = 0.34, z = 0.50, w = 1.0 },
            [imgui.Col.ButtonActive] = state.accent,
            [imgui.Col.SliderGrab] = state.accent,
            [imgui.Col.SliderGrabActive] = { x = 0.55, y = 0.65, z = 1.0, w = 1.0 },
            [imgui.Col.CheckMark] = state.accent,
            [imgui.Col.Header] = { x = 0.18, y = 0.22, z = 0.32, w = 0.70 },
            [imgui.Col.HeaderHovered] = { x = 0.24, y = 0.30, z = 0.42, w = 0.85 },
            [imgui.Col.HeaderActive] = state.accent,
        },
    })
end

local function beginTab(index: number)
    if state.activeTab ~= index then
        state.prevTab = state.activeTab
        state.tabSlide = if index > state.activeTab then 1 else -1
        state.activeTab = index
        state.tabAnim = 0
    end
end

local function drawAnimatedTabBody(fn: () -> ())
    imgui.style.with({
        alpha = state.tabAnim * state.menuAnim,
    }, function()
        imgui.dummy({ x = state.tabSlide * 48, y = 0 })
        fn()
    end)
end

local function drawThemeTab()
    beginTab(1)
    drawAnimatedTabBody(function()
        imgui.separatorText("Colors")
        state.brightness = imgui.sliderFloat("Brightness", state.brightness, 0, 1)
        state.contrast = imgui.sliderFloat("Contrast", state.contrast, 0, 2)
        state.preset = imgui.combo("Layout preset", state.preset, presetItems)
        state.surfaceColor = imgui.colorEdit4("Surface color", state.surfaceColor)
        imgui.tooltip(function()
            imgui.text("Adjust the surface tint used by scoped styles.")
        end)
    end)
end

local function drawControlsTab()
    beginTab(2)
    drawAnimatedTabBody(function()
        imgui.separatorText("Widgets")
        state.enabled = imgui.checkbox("Enabled", state.enabled)
        state.sensitivity = imgui.dragFloat("Sensitivity", state.sensitivity, { speed = 0.05, min = 0.5, max = 5 })
        state.stepSize = imgui.dragInt("Step size", state.stepSize, { speed = 1, min = 0, max = 20 })
        imgui.progressBar(state.progress, { overlay = string.format("%.0f%%", state.progress * 100) })
        imgui.text("Mode")
        if imgui.radioButton("Basic", state.controlMode == 1) then
            state.controlMode = 1
        end
        imgui.sameLine()
        if imgui.radioButton("Advanced", state.controlMode == 2) then
            state.controlMode = 2
        end
        imgui.sameLine()
        if imgui.radioButton("Custom", state.controlMode == 3) then
            state.controlMode = 3
        end
    end)
end

local function drawDataTab()
    beginTab(3)
    drawAnimatedTabBody(function()
        imgui.separatorText("Table")
        state.showBorders = imgui.checkbox("Show borders", state.showBorders)
        state.refreshRate = imgui.sliderFloat("Refresh rate", state.refreshRate, 0.25, 3)

        local propsOpen = imgui.collapsingHeader("Properties", { visible = true })
        if propsOpen then
            imgui.indent()
            imgui.bulletText("Border width: 1px")
            imgui.bulletText("Cell padding: 8px")
            imgui.unindent()
        end

        local treeResult = imgui.treeNode("Records", function()
            imgui.text("Total: 128")
            imgui.text("Selected: 42")
        end, { open = state.treeOpen })
        if treeResult ~= nil then
            state.treeOpen = treeResult
        end

        imgui.separator()
        imgui.table("data-table", 3, function()
            imgui.tableSetupColumn("Name")
            imgui.tableSetupColumn("Value")
            imgui.tableSetupColumn("Status")
            imgui.tableHeadersRow()
            for _, row in tableRows do
                imgui.tableNextRow()
                imgui.tableNextColumn()
                imgui.text(row.name)
                imgui.tableNextColumn()
                imgui.text(row.value)
                imgui.tableNextColumn()
                imgui.text(row.status)
            end
        end, { flags = imgui.Flag.Table.Borders })
    end)
end

local function drawListsTab()
    beginTab(4)
    drawAnimatedTabBody(function()
        imgui.separatorText("Selection")
        state.listIndex = imgui.listBox("Items", state.listIndex, listItems, { height = 96 })

        imgui.text("Flags")
        state.listFlags = imgui.checkboxFlags("Flag A", state.listFlags, FLAG_A)
        state.listFlags = imgui.checkboxFlags("Flag B", state.listFlags, FLAG_B)
        state.listFlags = imgui.checkboxFlags("Flag C", state.listFlags, FLAG_C)

        imgui.separator()
        for i, label in listItems do
            local selected = state.selectedRow == i
            if imgui.selectable(label, selected) then
                state.selectedRow = i
            end
        end

        if imgui.isItemHovered() then
            imgui.setTooltip("Click to select a row.")
        end
    end)
end

local function resetDefaults()
    state.brightness = 0.6
    state.contrast = 1.0
    state.preset = 1
    state.enabled = false
    state.sensitivity = 1.0
    state.showBorders = true
    state.refreshRate = 1.0
    state.showFloaters = true
end

local function drawSettingsTab()
    beginTab(5)
    drawAnimatedTabBody(function()
        imgui.separatorText("Preferences")
        state.accent = imgui.colorEdit4("Accent color", state.accent)
        state.showFloaters = imgui.checkbox("Show floating panels", state.showFloaters)

        imgui.style.with({
            colors = {
                [imgui.Col.Button] = state.accent,
                [imgui.Col.ButtonHovered] = { x = 0.55, y = 0.65, z = 1.0, w = 1.0 },
            },
        }, function()
            if imgui.button("Reset defaults") then
                imgui.openPopup("reset-confirm")
            end
            imgui.sameLine()
            if imgui.button("About") then
                imgui.openPopup("about-popup")
            end
        end)

        imgui.popupModal("reset-confirm", function()
            imgui.text("Reset all demo values?")
            if imgui.button("Yes") then
                resetDefaults()
                imgui.closeCurrentPopup()
            end
            imgui.sameLine()
            if imgui.button("No") then
                imgui.closeCurrentPopup()
            end
        end)

        imgui.popup("about-popup", function()
            imgui.text("ImGui Demo")
            imgui.text("LuauAPI widget showcase")
            imgui.separator()
            imgui.text("Press TAB to toggle.")
        end)

        local tabKey = geode.Keybind.toString({ key = keys.KEY_Tab, modifiers = mods.None })
        imgui.separator()
        imgui.text("Toggle key: " .. tabKey)
    end)
end

local TAB_DRAWERS = {
    drawThemeTab,
    drawControlsTab,
    drawDataTab,
    drawListsTab,
    drawSettingsTab,
}

local function drawMainWindow()
    local slideX = 24 + (1 - state.menuAnim) * -(MAIN_W + 40)

    imgui.style.with({
        alpha = state.menuAnim,
    }, function()
        local stillOpen = imgui.window("ImGui Demo", function()
            imgui.menuBar(function()
                imgui.menu("File", function()
                    if imgui.menuItem("Close") then
                        state.menuOpen = false
                    end
                end)
                imgui.menu("View", function()
                    state.showFloaters = imgui.menuItem("Floating panels", state.showFloaters)
                end)
            end)

            imgui.text("ImGui Demo")
            imgui.sameLine()
            imgui.text("v1.0")

            imgui.tabBar("main-tabs", function()
                for i, name in TAB_NAMES do
                    imgui.tabItem(name, TAB_DRAWERS[i])
                end
            end)
        end, {
            size = { x = MAIN_W, y = MAIN_H },
            sizeCond = imgui.Cond.Always,
            pos = { x = slideX, y = 48 },
            posCond = imgui.Cond.Always,
            flags = imgui.Flag.Window.NoCollapse,
        })

        if not stillOpen then
            state.menuOpen = false
        end
    end)
end

local FLOATING_PANELS = {
    { title = "Panel A", pos = { x = 560, y = 60 }, key = "primary" },
    { title = "Panel B", pos = { x = 40, y = 480 }, key = "secondary" },
    { title = "Panel C", pos = { x = 560, y = 360 }, key = "tertiary" },
}

local function drawFloater(panel: { title: string, pos: { x: number, y: number }, key: string })
    local panelState = state[panel.key]
    imgui.style.with({
        alpha = state.menuAnim * 0.95,
    }, function()
        imgui.window(panel.title, function()
            panelState.optionA = imgui.checkbox("Option A", panelState.optionA)
            panelState.optionB = imgui.checkbox("Option B", panelState.optionB)
            panelState.optionC = imgui.checkbox("Option C", panelState.optionC)
            panelState.optionD = imgui.checkbox("Option D", panelState.optionD)
            if panel.key == "primary" then
                panelState.amount = imgui.sliderFloat("Amount", panelState.amount, 0, 1)
            elseif panel.key == "secondary" then
                panelState.rate = imgui.sliderFloat("Rate", panelState.rate, 0.5, 3)
            else
                panelState.blend = imgui.sliderFloat("Blend", panelState.blend, 0, 1)
            end
        end, {
            pos = panel.pos,
            posCond = imgui.Cond.FirstUseEver,
            flags = bit32.bor(
                imgui.Flag.Window.NoCollapse,
                imgui.Flag.Window.AlwaysAutoResize
            ),
        })
    end)
end

local function drawFloatingPanels()
    if not state.showFloaters then
        return
    end
    for _, panel in FLOATING_PANELS do
        drawFloater(panel)
    end
end

local function updateAnimation(dt: number)
    local menuTarget = if state.menuOpen then 1 else 0
    state.menuAnim = stepAnim(state.menuAnim, menuTarget, dt, 10)
    state.tabSlide = stepAnim(state.tabSlide, 0, dt, 14)
    state.tabAnim = math.min(1, state.tabAnim + dt * 10)
end

applyTheme()

geode.KeyboardInputEvent.listenFor(keys.KEY_Tab, function(data)
    if data.action ~= action.Press then
        return false
    end
    state.menuOpen = not state.menuOpen
    return true
end, 100)

_G.modmenudemo_imguiHandle = imgui.onDraw(function()
    local now = os.clock()
    local dt = now - state.lastFrameTime
    state.lastFrameTime = now
    updateAnimation(dt)

    if state.menuAnim < 0.02 and not state.menuOpen then
        return
    end

    applyTheme()
    drawMainWindow()
    drawFloatingPanels()
end)

print("ImGui demo loaded. Press TAB to toggle.")
