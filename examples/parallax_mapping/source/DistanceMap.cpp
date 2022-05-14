#include "DistanceMap.hpp"

#include <cmath>
#include <limits>
#include <fmt/format.h>
// A type to hold the distance map while it's being constructed
struct DistanceMap {
    using Distance = short;

    DistanceMap(int width, int height, int depth)
            : m_data(new Distance[width*height*depth*3]),
              m_width(width), m_height(height), m_depth(depth)
    {
    }

    Distance& operator()(int x, int y, int z, int i)
    {
        return m_data[ 3*(m_depth*(m_height*x + y) + z) + i];
    }

    Distance operator()(int x, int y, int z, int i) const
    {
        return m_data[ 3*(m_depth*(m_height*x + y) + z) + i];
    }

    bool inside(int x, int y, int z) const
    {
        return (0 <= x && x < m_width &&
                0 <= y && y < m_height &&
                0 <= z && z < m_depth);
    }

    // Do a single pass over the data.
    // Start at (x,y,z) and walk in the direction (cx,cy,cz)
    // Combine each pixel (x,y,z) with the value at (x+dx,y+dy,z+dz)
    void combine(int dx, int dy, int dz,
                 int cx, int cy, int cz,
                 int x, int y, int z)
    {
        while (inside(x, y, z) && inside(x + dx, y + dy, z + dz)) {

            Distance d[3]; d[0] = std::abs(dx); d[1] = std::abs(dy); d[2] = std::abs(dz);

            unsigned long v1[3], v2[3];
            for (int i = 0; i < 3; i++) v1[i] = (*this)(x, y, z, i);
            for (int i = 0; i < 3; i++) v2[i] = (*this)(x+dx, y+dy, z+dz, i) + d[i];

            if (v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2] >
                v2[0] * v2[0] + v2[1] * v2[1] + v2[2] * v2[2]) {
                for (int i = 0; i < 3; i++) (*this)(x,y,z, i) = v2[i];
            }

            x += cx; y += cy; z += cz;
        }
    }

    Distance* m_data;
    int m_width, m_height, m_depth;
};

std::vector<float> init_distance_map(const unsigned char* heightMap, int width, int height, int depth, bool invert){
    // Initialize the distance map to zero below the surface,
    // and +infinity above
    DistanceMap dmap(width, height, depth);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * 4;    // rgba
            for (int z = 0; z < depth; z++) {
                for (int i = 0; i < 3; i++) {
                    auto r = static_cast<float>(static_cast<int>(heightMap[index]));
                    auto h = r/255.0f;
                    h = invert ? 1 - h : h;
                    if (z < h * depth || z <= 1) {
                        dmap(x, y, z, i) = 0;
                    } else {
                        dmap(x, y, z, i) = std::numeric_limits<DistanceMap::Distance>::max();
                    }
                }
            }
        }
    }

// Compute the rest of dmap by sequential sweeps over the data
    // using a 3d variant of Danielsson's algorithm
    fmt::print("Computing Distance Map\n");

    for (int z = 1; z < depth; z++) {


        // combine with everything with dz = -1
        for (int y = 0; y < height; y++) {
            dmap.combine( 0, 0, -1,
                          1, 0, 0,
                          0, y, z);
        }

        for (int y = 1; y < height; y++) {
            dmap.combine( 0, -1, 0,
                          1, 0, 0,
                          0, y, z);
            dmap.combine(-1, 0, 0,
                         1, 0, 0,
                         1, y, z);
            dmap.combine(+1, 0, 0,
                         -1, 0, 0,
                         width - 2, y, z);
        }

        for (int y = height - 2; y >= 0; y--) {
            dmap.combine( 0, +1, 0,
                          1, 0, 0,
                          0, y, z);
            dmap.combine(-1, 0, 0,
                         1, 0, 0,
                         1, y, z);
            dmap.combine(+1, 0, 0,
                         -1, 0, 0,
                         width - 2, y, z);
        }
    }
    fmt::print(" done first pass\n");

    for (int z = depth - 2; z >= 0; z--) {

        // combine with everything with dz = +1
        for (int y = 0; y < height; y++) {
            dmap.combine( 0, 0, +1,
                          1, 0, 0,
                          0, y, z);
        }

        for (int y = 1; y < height; y++) {
            dmap.combine( 0, -1, 0,
                          1, 0, 0,
                          0, y, z);
            dmap.combine(-1, 0, 0,
                         1, 0, 0,
                         1, y, z);
            dmap.combine(+1, 0, 0,
                         -1, 0, 0,
                         width - 2, y, z);
        }
        for (int y = height - 2; y >= 0; y--) {
            dmap.combine( 0, +1, 0,
                          1, 0, 0,
                          0, y, z);
            dmap.combine(-1, 0, 0,
                         1, 0, 0,
                         1, y, z);
            dmap.combine(+1, 0, 0,
                         -1, 0, 0,
                         width - 2, y, z);
        }
    }

    fmt::print( " done second pass\n");

    // Construct a 3d texture img and fill it with the magnitudes of the
    // displacements in dmap, scaled appropriately
    std::vector<float> img(width * height * depth, 1);
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float value = 0;
                for (int i = 0; i < 3; i++) {
                    value += dmap(x, y, z, i)*dmap(x, y, z, i);
                }
                value = sqrt(value)/depth;
                if (value > 1.0) value = 1.0;
                int index = (z * height + y) * width + x;
                img[index] = value;
            }
        }
    }

    return img;
}