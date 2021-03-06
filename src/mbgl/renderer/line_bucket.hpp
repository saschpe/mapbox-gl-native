#ifndef MBGL_RENDERER_LINEBUCKET
#define MBGL_RENDERER_LINEBUCKET

#include <mbgl/renderer/bucket.hpp>
#include <mbgl/tile/geometry_tile.hpp>
#include <mbgl/geometry/vao.hpp>
#include <mbgl/geometry/elements_buffer.hpp>
#include <mbgl/geometry/line_buffer.hpp>
#include <mbgl/util/vec.hpp>
#include <mbgl/layer/line_layer.hpp>

#include <vector>

namespace mbgl {

class Style;
class LineVertexBuffer;
class TriangleElementsBuffer;
class LineShader;
class LineSDFShader;
class LinepatternShader;

class LineBucket : public Bucket {
    using TriangleGroup = ElementGroup<3>;

public:
    LineBucket(float overscaling);
    ~LineBucket() override;

    void upload(gl::GLObjectStore&) override;
    void render(Painter&, const StyleLayer&, const TileID&, const mat4&) override;
    bool hasData() const override;

    void addGeometry(const GeometryCollection&);
    void addGeometry(const std::vector<Coordinate>& line);

    void drawLines(LineShader&, gl::GLObjectStore&);
    void drawLineSDF(LineSDFShader&, gl::GLObjectStore&);
    void drawLinePatterns(LinepatternShader&, gl::GLObjectStore&);

private:
    struct TriangleElement {
        TriangleElement(uint16_t a_, uint16_t b_, uint16_t c_) : a(a_), b(b_), c(c_) {}
        uint16_t a, b, c;
    };
    void addCurrentVertex(const Coordinate& currentVertex, float flip, double distance,
            const vec2<double>& normal, float endLeft, float endRight, bool round,
            GLint startVertex, std::vector<LineBucket::TriangleElement>& triangleStore);
    void addPieSliceVertex(const Coordinate& currentVertex, float flip, double distance,
            const vec2<double>& extrude, bool lineTurnsLeft, GLint startVertex,
            std::vector<TriangleElement>& triangleStore);

public:
    LineLayoutProperties layout;

private:
    LineVertexBuffer vertexBuffer;
    TriangleElementsBuffer triangleElementsBuffer;

    GLint e1;
    GLint e2;
    GLint e3;

    std::vector<std::unique_ptr<TriangleGroup>> triangleGroups;

    const float overscaling;
};

} // namespace mbgl

#endif
