#include "render3d/gpu/ShaderSources.hpp"

namespace luax::render3d::shader_sources {

    char const kLambertVert[] = R"(attribute vec3 aPos;
attribute vec3 aNormal;
attribute vec2 aTexCoord;
uniform mat4 uMVP;
uniform mat4 uNormalMat;
uniform vec3 uTint;
varying vec3 vNormal;
varying vec2 vTexCoord;
varying vec3 vTint;

void main() {
    vNormal = mat3(uNormalMat) * aNormal;
    vTexCoord = aTexCoord;
    vTint = uTint;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

    char const kLambertInstVert[] = R"(attribute vec3 aPos;
attribute vec3 aNormal;
attribute vec2 aTexCoord;
attribute vec4 aModel0;
attribute vec4 aModel1;
attribute vec4 aModel2;
attribute vec4 aModel3;
attribute vec3 aTint;
uniform mat4 uViewProj;
varying vec3 vNormal;
varying vec2 vTexCoord;
varying vec3 vTint;

void main() {
    mat4 model = mat4(aModel0, aModel1, aModel2, aModel3);
    vNormal = mat3(model) * aNormal;
    vTexCoord = aTexCoord;
    vTint = aTint;
    gl_Position = uViewProj * model * vec4(aPos, 1.0);
}
)";

    char const kLambertFrag[] = R"(#ifdef GL_ES
precision mediump float;
#endif
varying vec3 vNormal;
varying vec2 vTexCoord;
varying vec3 vTint;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform float uAmbient;
uniform vec4 uBaseColor;
uniform sampler2D uTexture;
uniform float uUseTexture;
uniform float uAlphaCutoff;

void main() {
    vec4 texel = uUseTexture > 0.5 ? texture2D(uTexture, vTexCoord) : vec4(1.0);
    vec3 albedo = uUseTexture > 0.5 ? texel.rgb : uBaseColor.rgb;
    albedo *= vTint;
    float alpha = uBaseColor.a * (uUseTexture > 0.5 ? texel.a : 1.0);
    if (alpha < uAlphaCutoff) {
        discard;
    }
    vec3 n = normalize(vNormal);
    float diff = max(dot(n, normalize(uLightDir)), 0.0);
    vec3 lighting = vec3(uAmbient) + uLightColor * diff;
    gl_FragColor = vec4(albedo * lighting, alpha);
}
)";

    char const kBlitVert[] = R"(attribute vec2 aPos;
attribute vec2 aTexCoord;
uniform mat4 uMVP;
varying vec2 vTexCoord;

void main() {
    gl_Position = uMVP * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

    char const kBlitFrag[] = R"(#ifdef GL_ES
precision mediump float;
#endif
uniform sampler2D uTexture;
varying vec2 vTexCoord;

void main() {
    gl_FragColor = texture2D(uTexture, vTexCoord);
}
)";

    char const kDebugLineVert[] = R"(attribute vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

    char const kDebugLineFrag[] = R"(#ifdef GL_ES
precision mediump float;
#endif
uniform vec3 uColor;
void main() {
    gl_FragColor = vec4(uColor, 1.0);
}
)";

} // namespace luax::render3d::shader_sources
