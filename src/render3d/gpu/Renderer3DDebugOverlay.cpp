#include "render3d/gpu/Renderer3DDebugOverlay.hpp"

#include "render3d/assets/MeshAsset.hpp"
#include "render3d/gpu/GlUtil.hpp"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace luax::render3d {
    void drawDebugOverlay(
        Renderer3DPrograms& programs, glm::mat4 const& projection, glm::mat4 const& view,
        std::map<int, DebugLine> const& debugLines, bool debugBounds,
        std::map<int, ViewportInstance> const& instances
    ) {
        if (!programs.ensureDebugLineProgram() || !programs.ensureDebugLineVbo()) {
            return;
        }

        struct LineSegment {
            glm::vec3 from{};
            glm::vec3 to{};
            glm::vec3 color{1.0f, 1.0f, 1.0f};
        };

        std::vector<LineSegment> segments;
        segments.reserve(debugLines.size() + instances.size() * 12);

        for (auto const& [lineId, line] : debugLines) {
            (void)lineId;
            segments.push_back(LineSegment{line.from, line.to, line.color});
        }

        if (debugBounds) {
            glm::vec3 const boundsColor{0.0f, 1.0f, 0.0f};
            for (auto const& [instanceId, instance] : instances) {
                (void)instanceId;
                if (instance.mesh == nullptr) {
                    continue;
                }
                BoundingBox const& bounds = instance.mesh->boundingBox();
                if (bounds.empty) {
                    continue;
                }

                glm::mat4 const model = instance.transform.toMat4();
                glm::vec3 const corners[8] = {
                    glm::vec3(model * glm::vec4(bounds.min.x, bounds.min.y, bounds.min.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.max.x, bounds.min.y, bounds.min.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.max.x, bounds.max.y, bounds.min.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.min.x, bounds.max.y, bounds.min.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.min.x, bounds.min.y, bounds.max.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.max.x, bounds.min.y, bounds.max.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.max.x, bounds.max.y, bounds.max.z, 1.0f)),
                    glm::vec3(model * glm::vec4(bounds.min.x, bounds.max.y, bounds.max.z, 1.0f)),
                };

                auto addEdge = [&](int a, int b) {
                    segments.push_back(LineSegment{corners[a], corners[b], boundsColor});
                };

                addEdge(0, 1);
                addEdge(1, 2);
                addEdge(2, 3);
                addEdge(3, 0);
                addEdge(4, 5);
                addEdge(5, 6);
                addEdge(6, 7);
                addEdge(7, 4);
                addEdge(0, 4);
                addEdge(1, 5);
                addEdge(2, 6);
                addEdge(3, 7);
            }
        }

        if (segments.empty()) {
            return;
        }

        std::sort(segments.begin(), segments.end(), [](LineSegment const& a, LineSegment const& b) {
            if (a.color.x != b.color.x) {
                return a.color.x < b.color.x;
            }
            if (a.color.y != b.color.y) {
                return a.color.y < b.color.y;
            }
            return a.color.z < b.color.z;
        });

        bindVao(0);

        glm::mat4 const viewProj = projection * view;
        glUseProgram(programs.debugLineProgram);
        glUniformMatrix4fv(programs.debugLineLocMvp, 1, GL_FALSE, glm::value_ptr(viewProj));
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);

        glBindBuffer(GL_ARRAY_BUFFER, programs.debugLineVbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(glm::vec3)), nullptr
        );

        std::size_t runStart = 0;
        while (runStart < segments.size()) {
            glm::vec3 const runColor = segments[runStart].color;
            std::size_t runEnd = runStart + 1;
            while (runEnd < segments.size() && segments[runEnd].color == runColor) {
                ++runEnd;
            }

            std::vector<glm::vec3> vertices;
            vertices.reserve((runEnd - runStart) * 2);
            for (std::size_t i = runStart; i < runEnd; ++i) {
                vertices.push_back(segments[i].from);
                vertices.push_back(segments[i].to);
            }

            glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)),
                vertices.data(),
                GL_DYNAMIC_DRAW
            );
            glUniform3fv(programs.debugLineLocColor, 1, glm::value_ptr(runColor));
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

            runStart = runEnd;
        }

        glDisableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDepthMask(GL_TRUE);
    }

} // namespace luax::render3d
