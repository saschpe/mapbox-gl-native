#ifndef MBGL_MAP_TILE_ID
#define MBGL_MAP_TILE_ID

#include <cstdint>
#include <cmath>
#include <string>
#include <functional>
#include <forward_list>
#include <limits>

namespace mbgl {

// Has integer z/x/y coordinates
// All tiles must be derived from 0/0/0 (no tiles outside of the main tile pyramid).
// Used for requesting data.
// z is never larger than the source's maxzoom.
class TileID {
public:
    const uint8_t z;
    const int32_t x;
    const int32_t y;
    const uint8_t sourceZ;

    inline explicit TileID(uint8_t z_, int32_t x_, int32_t y_, uint8_t sourceZ_)
        : z(z_), x(x_), y(y_), sourceZ(sourceZ_)
    {
    }

    inline uint64_t to_uint64() const {
        return ((std::pow(2, z) * y + x) * 32) + z;
    }

    inline bool operator==(const TileID& rhs) const {
        return z == rhs.z && x == rhs.x && y == rhs.y;
    }

    inline bool operator!=(const TileID& rhs) const {
        return !operator==(rhs);
    }

    inline bool operator<(const TileID& rhs) const {
        if (z != rhs.z) return z < rhs.z;
        if (x != rhs.x) return x < rhs.x;
        return y < rhs.y;
    }

    TileID parent(uint8_t z, uint8_t sourceMaxZoom) const;
    TileID normalized() const;
    std::forward_list<TileID>
    children(uint8_t sourceMaxZoom = std::numeric_limits<uint8_t>::max()) const;
    bool isChildOf(const TileID&) const;
    operator std::string() const;
};

// Has integer z/x/y coordinates
// w describes tiles that are left/right of the main tile pyramid, e.g. when wrapping the world.
// Used for describing what position tiles are getting rendered at (= calc the matrix).
// z is never larger than the source's maxzoom.
class UnwrappedTileID : public TileID {
public:
    const int16_t w;
    inline explicit UnwrappedTileID(uint8_t z_, int32_t x_, int32_t y_, uint8_t sourceZ_)
        : TileID(z_, x_, y_, sourceZ_), w(x_ < 0 ? x_ - (1 << z_) + 1 : x_ / (1 << z_)) {}
};

} // namespace mbgl

namespace std {
template <>
struct hash<mbgl::TileID> {
    typedef mbgl::TileID argument_type;
    typedef std::size_t result_type;

    result_type operator()(const mbgl::TileID& id) const {
            return std::hash<uint64_t>()(id.to_uint64());
    }
};
} // namespace std

#endif
