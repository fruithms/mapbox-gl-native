#pragma once

#include <mbgl/renderer/tile_mask.hpp>
#include <mbgl/renderer/indexed_primitives.hpp>
#include <mbgl/renderer/drawable.hpp>
#include <mbgl/gl/draw_mode.hpp>
#include <mbgl/programs/raster_program.hpp>

namespace mbgl {

namespace gl {
class Context;
} // namespace gl

class TileMaskRepository {
public:
    // Call this function at the beginning of a frame to mark all drawables as "stale".
    void mark();

    // Delete all drawables that are still marked stale. Call this function at the end of a frame
    // during the cleanup phase.
    void sweep();

    // Obtains a drawable with the specified mask. Creates a new drawable if it doesn't exist, or
    // marks the existing as not "stale".
    const Drawable<gl::Triangles, RasterLayoutVertex, RasterAttributes>&
    getDrawable(gl::Context&, const TileMask&);

    static IndexedPrimitives<gl::Triangles, RasterLayoutVertex, RasterAttributes>
    getPrimitives(const TileMask&);

private:
    struct Entry {
        template <class... Args>
        Entry(Args&&... args) : drawable(std::forward<Args>(args)...) {}
        const Drawable<gl::Triangles, RasterLayoutVertex, RasterAttributes> drawable;
        bool stale = false;
    };
    std::map<TileMask, Entry> drawables;
};

} // namespace mbgl
