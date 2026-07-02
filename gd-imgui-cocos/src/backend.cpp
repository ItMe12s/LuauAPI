#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>
#include <imgui_internal.h>
#include <imgui.h>
#include "render3d/gpu/GlUtil.hpp"
#include "bindings/imgui/ImGuiHost.hpp"
#include <string>
#include <utility>

#ifdef GEODE_IS_WINDOWS
	// so msvc shuts up
	#define sscanf sscanf_s
#endif

using namespace geode::prelude;

// Cast ImTextureID (OpenGL texture name) to and from GLuint.

static GLuint toGLTexture(ImTextureID tex) {
	return static_cast<GLuint>(tex);
}
static ImTextureID fromGLTexture(GLuint tex) {
	return static_cast<ImTextureID>(tex);
}

#ifdef GEODE_IS_WINDOWS

#define MAT_SUPPORTS_CURSOR

// We can't use the GLFW API directly,
// so we set the cursor by changing a pointer at a fixed offset in the GLFWwindow struct.
// The kGlfwCursorOffset value is based on the current GLFW layout in GD (tested with 2.2081).
// If the bundled GLFW version changes, this offset needs to be checked again.
// Ask imes about this if you need to change it.
constexpr uintptr_t kGlfwCursorOffset = 0x50;

struct GLFWCursorData {
	void* next = nullptr;
	HCURSOR cursor;
};

static void setMouseCursor(ImGuiMouseCursor cursor) {
	auto* glfwWindow = CCEGLView::get()->getWindow();

	auto& cursorField = *reinterpret_cast<GLFWCursorData**>(
		reinterpret_cast<uintptr_t>(glfwWindow) + kGlfwCursorOffset
	);
	auto winCursor = IDC_ARROW;
	switch (cursor) {
		case ImGuiMouseCursor_Arrow: winCursor = IDC_ARROW; break;
		case ImGuiMouseCursor_TextInput: winCursor = IDC_IBEAM; break;
		case ImGuiMouseCursor_ResizeAll: winCursor = IDC_SIZEALL; break;
		case ImGuiMouseCursor_ResizeEW: winCursor = IDC_SIZEWE; break;
		case ImGuiMouseCursor_ResizeNS: winCursor = IDC_SIZENS; break;
		case ImGuiMouseCursor_ResizeNESW: winCursor = IDC_SIZENESW; break;
		case ImGuiMouseCursor_ResizeNWSE: winCursor = IDC_SIZENWSE; break;
		case ImGuiMouseCursor_Hand: winCursor = IDC_HAND; break;
		case ImGuiMouseCursor_NotAllowed: winCursor = IDC_NO; break;
	}
	if (cursorField) {
		cursorField->cursor = LoadCursor(NULL, winCursor);
	} else {
		// must be heap allocated
		cursorField = new GLFWCursorData {
			.next = nullptr,
			.cursor = LoadCursor(NULL, winCursor)
		};
	}
}
#endif

#ifdef GEODE_IS_MOBILE

class ImGuiIMEDelegate : public CCIMEDelegate {
protected:
	bool m_attached = false;
	std::string m_text;
public:
	bool attachWithIME() override {
		if (CCIMEDelegate::attachWithIME()) {
			m_attached = true;
			CCEGLView::get()->setIMEKeyboardState(true);
			return true;
		}
		return false;
	}

	bool detachWithIME() override {
		if (CCIMEDelegate::detachWithIME()) {
			m_attached = false;
			CCEGLView::get()->setIMEKeyboardState(false);
			ImGui::ClearActiveID();
			return true;
		}
		return false;
	}

	bool canAttachWithIME() override {
		return true;
	}

	bool canDetachWithIME() override {
		return true;
	}

	char const* getContentText() override {
		auto& buffer = ImGui::GetInputTextState(ImGui::GetFocusID())->TextA;
		m_text = std::string(buffer.begin(), buffer.end());
		return m_text.c_str();
	}

	bool isAttached() {
		return m_attached;
	}

	static ImGuiIMEDelegate* get() {
		static ImGuiIMEDelegate* instance = new ImGuiIMEDelegate();
		return instance;
	}
};

#endif

ImGuiCocos& ImGuiCocos::get() {
	static ImGuiCocos inst;
	return inst;
}

ImGuiCocos::ImGuiCocos() {
	m_setupCall = m_drawCall = [] {};
}

ImGuiCocos& ImGuiCocos::setup(std::function<void()> fun) {
	m_setupCall = std::move(fun);
	return this->setup();
}

ImGuiCocos& ImGuiCocos::draw(std::function<void()> fun) {
	m_drawCall = std::move(fun);
	return *this;
}

void ImGuiCocos::toggle() {
	this->setVisible(!m_visible);
}

void ImGuiCocos::setVisible(bool v) {
	m_visible = v;
	auto& io = ImGui::GetIO();
	if (!m_visible) {
		io.WantCaptureKeyboard = false;
		io.WantCaptureMouse = false;
		io.WantTextInput = false;

#ifdef MAT_SUPPORTS_CURSOR
		setMouseCursor(ImGuiMouseCursor_Arrow);
		m_lastCursor = ImGuiMouseCursor_COUNT;
#endif
	}
	io.SetAppAcceptingEvents(m_visible);
}

bool ImGuiCocos::isVisible() const {
	return m_visible;
}

bool ImGuiCocos::isInitialized() const {
	return m_initialized;
}

ImGuiCocos& ImGuiCocos::setup() {
	if (m_initialized) return *this;

	ImGui::CreateContext();

	auto& io = ImGui::GetIO();

	static const int glVersion = [] {
	#if defined(GEODE_IS_ANDROID)
		// android uses GLES v2
		return 200;
	#endif
		int major = 0;
		int minor = 0;
	#if defined(GEODE_IS_WINDOWS)
		// macos opengl is really outdated, and doesnt have these enums
		glGetIntegerv(GL_MAJOR_VERSION, &major);
		glGetIntegerv(GL_MINOR_VERSION, &minor);
	#endif
		if (major == 0 && minor == 0) {
			auto* verStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
			if (!verStr || sscanf(verStr, "%d.%d", &major, &minor) != 2) {
				// failed to parse version string, just assume opengl 2.1
				return 210;
			}
		}
		return major * 100 + minor * 10;
	}();

	io.BackendPlatformName = "gd-imgui-cocos + Geode";
	io.BackendPlatformUserData = this;
	if (glVersion >= 320) {
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	}
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

	// use static since imgui does not own the pointer!
	static const auto iniPath = (Mod::get()->getSaveDir() / "imgui.ini").string();
	io.IniFilename = iniPath.c_str();

	ImGui::GetPlatformIO().Platform_GetClipboardTextFn = [](ImGuiContext*) {
		static std::string text;
		text = geode::utils::clipboard::read();
		return text.c_str();
	};
	ImGui::GetPlatformIO().Platform_SetClipboardTextFn = [](ImGuiContext*, const char* text) {
		geode::utils::clipboard::write(text);
	};

	m_initialized = true;

	m_setupCall();

	return *this;
}

void ImGuiCocos::destroy(bool abandonTextures) {
	if (!m_initialized) return;

	ImGui::GetIO().BackendPlatformUserData = nullptr;
	for (auto* tex : ImGui::GetPlatformIO().Textures) {
		if (tex->BackendUserData != nullptr) {
			if (abandonTextures) {
				tex->SetTexID(ImTextureID_Invalid);
				tex->BackendUserData = nullptr;
				tex->SetStatus(ImTextureStatus_Destroyed);
				continue;
			}
			if (tex->RefCount == 1) {
				tex->SetStatus(ImTextureStatus_WantDestroy);
				this->updateTexture(tex);
			}
		}
	}
	ImGui::DestroyContext();
	m_initialized = false;
}

void ImGuiCocos::reload(bool abandonTextures) {
	m_reloading = true;
	m_abandonReloadTextures = m_abandonReloadTextures || abandonTextures;
}

void ImGuiCocos::setDisplayScale(float scale) {
	if (scale > 0.f)
		m_displayScale = scale;
}

float ImGuiCocos::getDisplayScale() const {
	return m_displayScale;
}

ImVec2 ImGuiCocos::displaySize() {
	const auto scale = get().m_displayScale;
	const auto winSize = CCDirector::sharedDirector()->getWinSize();
	return {winSize.width * scale, winSize.height * scale};
}

ImVec2 ImGuiCocos::cocosToFrame(const CCPoint& pos) {
	auto* director = CCDirector::sharedDirector();
	const auto frameSize = displaySize();
	const auto winSize = director->getWinSize();

	return {
		pos.x / winSize.width * frameSize.x,
		(1.f - pos.y / winSize.height) * frameSize.y,
	};
}

CCPoint ImGuiCocos::frameToCocos(const ImVec2& pos) {
	auto* director = CCDirector::sharedDirector();
	const auto frameSize = displaySize();
	const auto winSize = director->getWinSize();

	return {
		pos.x / frameSize.x * winSize.width,
		(1.f - pos.y / frameSize.y) * winSize.height,
	};
}

void ImGuiCocos::drawFrame() {
#if !defined(LUAUAPI_HOST_TESTS)
	if (luax::render3d::gpuSessionReady()) {
		luax::initImGuiHost();
	}
#endif
	auto reloadIfNeeded = [this] {
		if (!m_reloading) return;
		bool const abandonTextures = m_abandonReloadTextures;
		this->destroy(abandonTextures);
		this->setup();
		m_reloading = false;
		m_abandonReloadTextures = false;
	};

	reloadIfNeeded();
	if (!m_initialized || !m_visible) return;
	if (!luax::render3d::glContextAvailable() || !luax::render3d::gpuSessionReady()) {
		return;
	}

	luax::render3d::DrawStateSnapshot prevState{};
	prevState.capture();

	ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	this->newFrame();
	ImGui::NewFrame();
	m_drawCall();
	ImGui::Render();
	this->renderFrame();

	prevState.restore();
	reloadIfNeeded();
}

void ImGuiCocos::newFrame() {
	auto& io = ImGui::GetIO();

	// opengl2 new frame
	auto* director = CCDirector::sharedDirector();
	io.DisplaySize = displaySize();
	if (director->getDeltaTime() > 0.f) {
		io.DeltaTime = director->getDeltaTime();
	} else {
		io.DeltaTime = 1.f / 60.f;
	}

	if (auto pos = geode::cocos::getMousePos(); !pos.isZero()) {
		const auto mouse = cocosToFrame(pos);
		io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
		io.AddMousePosEvent(mouse.x, mouse.y);
	}

	auto* kb = director->getKeyboardDispatcher();
	io.KeyAlt = kb->getAltKeyPressed() || kb->getCommandKeyPressed(); // look
	io.KeyCtrl = kb->getControlKeyPressed();
	io.KeyShift = kb->getShiftKeyPressed();

#ifdef GEODE_IS_MOBILE
	auto ime = ImGuiIMEDelegate::get();
	if (io.WantTextInput && !ime->isAttached()) {
		ime->attachWithIME();
	} else if (!io.WantTextInput && ime->isAttached()) {
		ime->detachWithIME();
	}
#endif

#ifdef MAT_SUPPORTS_CURSOR
	auto cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
	if (cursor != m_lastCursor) {
		m_lastCursor = cursor;
		setMouseCursor(cursor);
	}
#endif
}

static bool hasExtension(const std::string_view ext) {
	static auto exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
	if (exts == nullptr)
		return false;

	return std::string_view(exts).find(ext) != std::string::npos;
}

static void drawTriangle(const std::array<CCPoint, 3>& poly, const std::array<ccColor4F, 3>& colors, const std::array<CCPoint, 3>& uvs) {
	auto* shader = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
	shader->use();
	shader->setUniformsForBuiltins();

	ccGLEnableVertexAttribs(kCCVertexAttribFlag_PosColorTex);

	static_assert(sizeof(CCPoint) == sizeof(ccVertex2F), "so the cocos devs were right then");

	glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, poly.data());
	glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_FLOAT, GL_FALSE, 0, colors.data());
	glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, uvs.data());

	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
}

void ImGuiCocos::legacyRenderFrame() const {
	glEnable(GL_SCISSOR_TEST);

	auto* drawData = ImGui::GetDrawData();

	if (drawData->Textures != nullptr) {
		for (auto* tex : *drawData->Textures) {
			if (tex->Status != ImTextureStatus_OK) {
				this->updateTexture(tex);
			}
		}
	}

	for (int i = 0; i < drawData->CmdListsCount; ++i) {
		auto* list = drawData->CmdLists[i];
		auto* idxBuffer = list->IdxBuffer.Data;
		auto* vtxBuffer = list->VtxBuffer.Data;
		for (auto& cmd : list->CmdBuffer) {
			GLuint const cmdTexture = toGLTexture(cmd.GetTexID());
			ccGLBindTexture2D(cmdTexture);

			const auto rect = cmd.ClipRect;
			const auto orig = frameToCocos(ImVec2(rect.x, rect.y));
			const auto end = frameToCocos(ImVec2(rect.z, rect.w));
			if (end.x <= orig.x || end.y >= orig.y)
				continue;
			CCDirector::sharedDirector()->getOpenGLView()->setScissorInPoints(orig.x, end.y, end.x - orig.x, orig.y - end.y);

			for (unsigned int j = 0; j < cmd.ElemCount; j += 3) {
				const auto a = vtxBuffer[idxBuffer[cmd.IdxOffset + j + 0]];
				const auto b = vtxBuffer[idxBuffer[cmd.IdxOffset + j + 1]];
				const auto c = vtxBuffer[idxBuffer[cmd.IdxOffset + j + 2]];
				std::array<CCPoint, 3> points = {
					frameToCocos(a.pos),
					frameToCocos(b.pos),
					frameToCocos(c.pos),
				};
				static constexpr auto ccc4FromImColor = [](const ImColor color) {
					// beautiful
					return ccc4f(color.Value.x, color.Value.y, color.Value.z, color.Value.w);
				};
				std::array<ccColor4F, 3> colors = {
					ccc4FromImColor(a.col),
					ccc4FromImColor(b.col),
					ccc4FromImColor(c.col),
				};

				std::array<CCPoint, 3> uvs = {
					ccp(a.uv.x, a.uv.y),
					ccp(b.uv.x, b.uv.y),
					ccp(c.uv.x, c.uv.y),
				};

				drawTriangle(points, colors, uvs);
			}
		}
	}
}

void ImGuiCocos::renderFrame() const {
	if (!luax::render3d::glContextAvailable()) return;

#if defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
	static bool hasVAO = hasExtension("GL_APPLE_vertex_array_object");
#else
	static bool hasVAO = hasExtension("GL_ARB_vertex_array_object");
#endif
	if (!hasVAO)
		return legacyRenderFrame();

	auto* drawData = ImGui::GetDrawData();

	const bool hasVtxOffset = (ImGui::GetIO().BackendFlags & ImGuiBackendFlags_RendererHasVtxOffset) != 0;

	if (drawData->Textures != nullptr) {
		for (auto* tex : *drawData->Textures) {
			if (tex->Status != ImTextureStatus_OK) {
				this->updateTexture(tex);
			}
		}
	}

	glEnable(GL_SCISSOR_TEST);

	unsigned int const vao = luax::render3d::genVao();
	if (vao == 0) return legacyRenderFrame();

	GLuint vbos[2] = {0, 0};

	luax::render3d::bindVao(vao);

	glGenBuffers(2, &vbos[0]);

	glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[1]);

	glEnableVertexAttribArray(kCCVertexAttrib_Position);
	glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), reinterpret_cast<void*>(offsetof(ImDrawVert, pos)));

	glEnableVertexAttribArray(kCCVertexAttrib_TexCoords);
	glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), reinterpret_cast<void*>(offsetof(ImDrawVert, uv)));

	glEnableVertexAttribArray(kCCVertexAttrib_Color);
	glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), reinterpret_cast<void*>(offsetof(ImDrawVert, col)));

	auto* shader = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
	shader->use();
	shader->setUniformsForBuiltins();

	for (int i = 0; i < drawData->CmdListsCount; ++i) {
		auto* list = drawData->CmdLists[i];

		// convert vertex coords to cocos space
		for (auto& j : list->VtxBuffer) {
			const auto point = frameToCocos(j.pos);
			j.pos = ImVec2(point.x, point.y);
		}

		glBufferData(GL_ARRAY_BUFFER, list->VtxBuffer.Size * sizeof(ImDrawVert), list->VtxBuffer.Data, GL_STREAM_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, list->IdxBuffer.Size * sizeof(ImDrawIdx), list->IdxBuffer.Data, GL_STREAM_DRAW);

		for (auto& cmd : list->CmdBuffer) {
			if (cmd.UserCallback != nullptr) {
				cmd.UserCallback(list, &cmd);
				continue;
			}

			GLuint const cmdTexture = toGLTexture(cmd.GetTexID());
			ccGLBindTexture2D(cmdTexture);

			const auto rect = cmd.ClipRect;
			const auto orig = frameToCocos(ImVec2(rect.x, rect.y));
			const auto end = frameToCocos(ImVec2(rect.z, rect.w));

			if (end.x <= orig.x || end.y >= orig.y)
				continue;

			CCDirector::sharedDirector()->getOpenGLView()->setScissorInPoints(orig.x, end.y, end.x - orig.x, orig.y - end.y);

			if (hasVtxOffset) {
			#if !defined(GEODE_IS_MOBILE)
				glDrawElementsBaseVertex(GL_TRIANGLES, cmd.ElemCount, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(cmd.IdxOffset * sizeof(ImDrawIdx)), cmd.VtxOffset);
			#endif
			} else {
				glDrawElements(GL_TRIANGLES, cmd.ElemCount, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(cmd.IdxOffset * sizeof(ImDrawIdx)));
			}
		}
	}

	luax::render3d::bindVao(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDeleteBuffers(2, &vbos[0]);
	luax::render3d::deleteVao(vao);
}

void ImGuiCocos::updateTexture(ImTextureData* tex) const {
	if (tex->Status == ImTextureStatus_WantCreate) {
		IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);
		const void* pixels = tex->GetPixels();
		auto* ccTexture = new CCTexture2D;
		ccTexture->initWithData(pixels, kCCTexture2DPixelFormat_RGBA8888, tex->Width, tex->Height, CCSize(static_cast<float>(tex->Width), static_cast<float>(tex->Height)));

		tex->SetTexID(fromGLTexture(ccTexture->getName()));
		tex->BackendUserData = ccTexture;
		tex->SetStatus(ImTextureStatus_OK);
	} else if (tex->Status == ImTextureStatus_WantUpdates) {
		// cocos has no function for this, do it manually with opengl
		// copied from `imgui_impl_opengl3.cpp`
	#ifdef GL_UNPACK_ALIGNMENT
		GLint lastUnpackAlignment = 0;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &lastUnpackAlignment);
		// dont mess up odd widths
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	#endif

		GLint lastTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);
		GLuint texID = toGLTexture(tex->TexID);
		glBindTexture(GL_TEXTURE_2D, texID);

	#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH, tex->Width);
		for (ImTextureRect& r : tex->Updates) {
			glTexSubImage2D(GL_TEXTURE_2D, 0, r.x, r.y, r.w, r.h, GL_RGBA, GL_UNSIGNED_BYTE, tex->GetPixelsAt(r.x, r.y));
		}
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	#else
		// GL ES doesn't have GL_UNPACK_ROW_LENGTH, so copy to a contiguous buffer first
		static std::vector<unsigned char> tempBuffer;
		for (ImTextureRect& r : tex->Updates) {
			tempBuffer.resize(r.w * r.h * tex->BytesPerPixel);
			auto* ptr = tempBuffer.data();
			int dataWidth = r.w * tex->BytesPerPixel;
			for (int y = 0; y < r.h; y++) {
				std::memcpy(ptr, tex->GetPixelsAt(r.x, r.y + y), dataWidth);
				ptr += dataWidth;
			}
			glTexSubImage2D(GL_TEXTURE_2D, 0, r.x, r.y, r.w, r.h, GL_RGBA, GL_UNSIGNED_BYTE, tempBuffer.data());
		}
	#endif

		glBindTexture(GL_TEXTURE_2D, lastTexture); // Restore state
	#ifdef GL_UNPACK_ALIGNMENT
		glPixelStorei(GL_UNPACK_ALIGNMENT, lastUnpackAlignment);
	#endif

		tex->SetStatus(ImTextureStatus_OK);
	} else if (tex->Status == ImTextureStatus_WantDestroy) {
		auto* ccTexture = static_cast<CCTexture2D*>(tex->BackendUserData);
		if (ccTexture != nullptr) {
			delete ccTexture;
		}

		tex->SetTexID(ImTextureID_Invalid);
		tex->BackendUserData = nullptr;
		tex->SetStatus(ImTextureStatus_Destroyed);
	}
}
