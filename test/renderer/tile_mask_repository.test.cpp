#include <mbgl/test/util.hpp>

#include <mbgl/renderer/tile_mask_repository.hpp>

namespace std {

template <class T, std::size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& array) {
    os << "{{ ";
    bool first = true;
    for (const auto& t : array) {
        if (!first) {
            os << ", ";
        } else {
            first = false;
        }
        os << t;
    }
    return os << " }}";
}

} // namespace std

namespace mbgl {
namespace gl {
namespace detail {

template <class A1, class A2>
std::ostream& operator<<(std::ostream& os, const Vertex<A1, A2>& v) {
    return os << "{ " << v.a1 << ", " << v.a2 << " }";
}

} // namespace detail
} // namespace gl
} // namespace mbgl

using namespace mbgl;

TEST(TileMaskRepository, Empty) {
    auto primitives = TileMaskRepository::getPrimitives({});
    EXPECT_EQ((std::vector<RasterLayoutVertex>{}), primitives.getVertices().vector());
    EXPECT_EQ((std::vector<uint16_t>{}), primitives.getIndices().vector());
    EXPECT_EQ((gl::SegmentInfoVector{}), primitives.getSegmentInfo());
}

TEST(TileMaskRepository, NoChildren) {
    auto primitives = TileMaskRepository::getPrimitives({ CanonicalTileID{ 0, 0, 0 } });

    EXPECT_EQ(
        (std::vector<RasterLayoutVertex>{
            // 0/0/0
            RasterProgram::layoutVertex({ 0, 0 }, { 0, 0 }),
            RasterProgram::layoutVertex({ 8192, 0 }, { 32768, 0 }),
            RasterProgram::layoutVertex({ 0, 8192 }, { 0, 32768 }),
            RasterProgram::layoutVertex({ 8192, 8192 }, { 32768, 32768 }),
        }),
        primitives.getVertices().vector());

    EXPECT_EQ(
        (std::vector<uint16_t>{
            // 0/0/0
            0, 1, 2,
            1, 2, 3,
        }),
        primitives.getIndices().vector());


    EXPECT_EQ(
        (gl::SegmentInfoVector{
            { 0, 0, 4, 6 }
        }),
        primitives.getSegmentInfo());
}

TEST(TileMaskRepository, TwoChildren) {
    auto primitives = TileMaskRepository::getPrimitives(
        { CanonicalTileID{ 1, 0, 0 }, CanonicalTileID{ 1, 1, 1 } });

    EXPECT_EQ(
        (std::vector<RasterLayoutVertex>{
            // 1/0/1
            RasterProgram::layoutVertex({ 0, 0 }, { 0, 0 }),
            RasterProgram::layoutVertex({ 4096, 0 }, { 16384, 0 }),
            RasterProgram::layoutVertex({ 0, 4096 }, { 0, 16384 }),
            RasterProgram::layoutVertex({ 4096, 4096 }, { 16384, 16384 }),

            // 1/1/1
            RasterProgram::layoutVertex({ 4096, 4096 }, { 16384, 16384 }),
            RasterProgram::layoutVertex({ 8192, 4096 }, { 32768, 16384 }),
            RasterProgram::layoutVertex({ 4096, 8192 }, { 16384, 32768 }),
            RasterProgram::layoutVertex({ 8192, 8192 }, { 32768, 32768 }),
        }),
        primitives.getVertices().vector());

    EXPECT_EQ(
        (std::vector<uint16_t>{
            // 1/0/1
            0, 1, 2,
            1, 2, 3,

            // 1/1/1
            4, 5, 6,
            5, 6, 7,
        }),
        primitives.getIndices().vector());


    EXPECT_EQ(
        (gl::SegmentInfoVector{
            { 0, 0, 8, 12 }
        }),
        primitives.getSegmentInfo());
}

TEST(TileMaskRepository, Complex) {
    auto primitives = TileMaskRepository::getPrimitives(
        { CanonicalTileID{ 1, 0, 1 }, CanonicalTileID{ 1, 1, 0 }, CanonicalTileID{ 2, 2, 3 },
          CanonicalTileID{ 2, 3, 2 }, CanonicalTileID{ 3, 6, 7 }, CanonicalTileID{ 3, 7, 6 } });

    EXPECT_EQ(
        (std::vector<RasterLayoutVertex>{
            // 1/0/1
            RasterProgram::layoutVertex({ 0, 4096 }, { 0, 16384 }),
            RasterProgram::layoutVertex({ 4096, 4096 }, { 16384, 16384 }),
            RasterProgram::layoutVertex({ 0, 8192 }, { 0, 32768 }),
            RasterProgram::layoutVertex({ 4096, 8192 }, { 16384, 32768 }),

            // 1/1/0
            RasterProgram::layoutVertex({ 4096, 0 }, { 16384, 0 }),
            RasterProgram::layoutVertex({ 8192, 0 }, { 32768, 0 }),
            RasterProgram::layoutVertex({ 4096, 4096 }, { 16384, 16384 }),
            RasterProgram::layoutVertex({ 8192, 4096 }, { 32768, 16384 }),

            // 2/2/3
            RasterProgram::layoutVertex({ 4096, 6144 }, { 16384, 24576 }),
            RasterProgram::layoutVertex({ 6144, 6144 }, { 24576, 24576 }),
            RasterProgram::layoutVertex({ 4096, 8192 }, { 16384, 32768 }),
            RasterProgram::layoutVertex({ 6144, 8192 }, { 24576, 32768 }),

            // 2/3/2
            RasterProgram::layoutVertex({ 6144, 4096 }, { 24576, 16384 }),
            RasterProgram::layoutVertex({ 8192, 4096 }, { 32768, 16384 }),
            RasterProgram::layoutVertex({ 6144, 6144 }, { 24576, 24576 }),
            RasterProgram::layoutVertex({ 8192, 6144 }, { 32768, 24576 }),

            // 3/6/7
            RasterProgram::layoutVertex({ 6144, 7168 }, { 24576, 28672 }),
            RasterProgram::layoutVertex({ 7168, 7168 }, { 28672, 28672 }),
            RasterProgram::layoutVertex({ 6144, 8192 }, { 24576, 32768 }),
            RasterProgram::layoutVertex({ 7168, 8192 }, { 28672, 32768 }),

            // 3/7/6
            RasterProgram::layoutVertex({ 7168, 6144 }, { 28672, 24576 }),
            RasterProgram::layoutVertex({ 8192, 6144 }, { 32768, 24576 }),
            RasterProgram::layoutVertex({ 7168, 7168 }, { 28672, 28672 }),
            RasterProgram::layoutVertex({ 8192, 7168 }, { 32768, 28672 }),
        }),
        primitives.getVertices().vector());

    EXPECT_EQ(
        (std::vector<uint16_t>{
            // 1/0/1
            0, 1, 2,
            1, 2, 3,

            // 1/1/0
            4, 5, 6,
            5, 6, 7,

            // 2/2/3
            8, 9, 10,
            9, 10, 11,

            // 2/3/2
            12, 13, 14,
            13, 14, 15,

            // 3/6/7
            16, 17, 18,
            17, 18, 19,

            // 3/7/6
            20, 21, 22,
            21, 22, 23,
        }),
        primitives.getIndices().vector());


    EXPECT_EQ(
        (gl::SegmentInfoVector{
            { 0, 0, 24, 36 }
        }),
        primitives.getSegmentInfo());
}
