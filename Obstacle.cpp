#include "Obstacle.h"
#include <algorithm> // for std::min, std::max
#include <cmath>

//------------------------------------------------------------------------------
// Determines if a line segment (start to end) intersects with an axis-aligned bounding box (AABB).
// The algorithm uses the Ray-Box intersection algorithm based on the slab method. This method checks 
// the intersection along each axis separately and determines if the segment overlaps with the box.
// Reference: www.scratchapixel.com. (n.d.). A Minimal Ray-Tracer: Rendering Simple Shapes (Sphere, Cube, Disk, Plane, etc.).
// Available at: https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection.html
bool Obstacle::IntersectsLineSegment(const Vector3& start, const Vector3& end)
{
    const AABB& box = mAABB;

    Vector3 dir = end - start;
    Vector3 invDir;
    invDir.x = 1.0f / dir.x;
    invDir.y = 1.0f / dir.y;
    invDir.z = 1.0f / dir.z;

    int sign[3];
    sign[0] = (invDir.x < 0);
    sign[1] = (invDir.y < 0);
    sign[2] = (invDir.z < 0);

    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    // X-axis
    tmin = (box.min.x - start.x) * invDir.x;
    tmax = (box.max.x - start.x) * invDir.x;
    if (sign[0])
        std::swap(tmin, tmax);

    // Y-axis
    tymin = (box.min.y - start.y) * invDir.y;
    tymax = (box.max.y - start.y) * invDir.y;
    if (sign[1])
        std::swap(tymin, tymax);

    if (tmin > tymax || tymin > tmax)
        return false;

    tmin = std::max(tmin, tymin);
    tmax = std::min(tmax, tymax);

    // Z-axis
    tzmin = (box.min.z - start.z) * invDir.z;
    tzmax = (box.max.z - start.z) * invDir.z;
    if (sign[2])
        std::swap(tzmin, tzmax);

    if (tmin > tzmax || tzmin > tmax)
        return false;

    tmin = std::max(tmin, tzmin);
    tmax = std::min(tmax, tzmax);

    // Check if the intersection is within the line segment [0, 1]
    return tmax >= 0.0f && tmin <= 1.0f;
}