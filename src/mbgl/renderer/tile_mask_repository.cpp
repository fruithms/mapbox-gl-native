#include <mbgl/renderer/tile_mask_repository.hpp>
#include <mbgl/util/constants.hpp>

namespace mbgl {

void TileMaskRepository::mark() {
    // Mark all drawables as stale.
    for (auto& pair : drawables) {
        pair.second.stale = true;
    }
}

void TileMaskRepository::sweep() {
    // Delete all drawables that are still marked stale.
    for (auto it = drawables.begin(); it != drawables.end();) {
        if (it->second.stale) {
            drawables.erase(it++);
        } else {
            ++it;
        }
    }
}

const Drawable<gl::Triangles, RasterLayoutVertex, RasterAttributes>&
TileMaskRepository::getDrawable(gl::Context& context, const TileMask& mask) {
    auto it = drawables.find(mask);
    if (it == drawables.end()) {
        it = drawables
                 .emplace(std::piecewise_construct, std::forward_as_tuple(mask),
                          std::forward_as_tuple(context, getPrimitives(mask)))
                 .first;
    } else {
        it->second.stale = false;
    }
    return it->second.drawable;
}

IndexedPrimitives<gl::Triangles, RasterLayoutVertex, RasterAttributes>
TileMaskRepository::getPrimitives(const TileMask& mask) {
    IndexedPrimitives<gl::Triangles, RasterLayoutVertex, RasterAttributes> primitives;

    for (const auto& id : mask) {
        // Create a quad for every masked tile.
        const int32_t vertexExtent = util::EXTENT >> id.z;
        const int32_t textureExtent = 32768 >> id.z;

        const Point<int16_t> tlVertex = { static_cast<int16_t>(id.x * vertexExtent),
                                          static_cast<int16_t>(id.y * vertexExtent) };
        const Point<int16_t> brVertex = { static_cast<int16_t>(tlVertex.x + vertexExtent),
                                          static_cast<int16_t>(tlVertex.y + vertexExtent) };
        const Point<uint16_t> tlTexture = { static_cast<uint16_t>(id.x * textureExtent),
                                            static_cast<uint16_t>(id.y * textureExtent) };
        const Point<uint16_t> brTexture = { static_cast<uint16_t>(tlTexture.x + textureExtent),
                                            static_cast<uint16_t>(tlTexture.y + textureExtent) };

        primitives.add(
            {
                RasterProgram::layoutVertex({ tlVertex.x, tlVertex.y },
                                            { tlTexture.x, tlTexture.y }),
                RasterProgram::layoutVertex({ brVertex.x, tlVertex.y },
                                            { brTexture.x, tlTexture.y }),
                RasterProgram::layoutVertex({ tlVertex.x, brVertex.y },
                                            { tlTexture.x, brTexture.y }),
                RasterProgram::layoutVertex({ brVertex.x, brVertex.y },
                                            { brTexture.x, brTexture.y }),
            },
            {
                {{ 0, 1, 2 }},
                {{ 1, 2, 3 }},
            });
    }

    return primitives;
}

} // namespace mbgl
