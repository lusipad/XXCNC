#pragma once

#include <array>
#include <cmath>
#include <memory>

namespace xxcnc {

struct Point3D {
    double x;
    double y;
    double z;

    Point3D(double x = 0.0, double y = 0.0, double z = 0.0)
        : x(x), y(y), z(z) {}

    Point3D operator+(const Point3D& other) const {
        return Point3D(x + other.x, y + other.y, z + other.z);
    }

    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }
};

class CoordinateSystem {
public:
    // 工件坐标系统编号（G54-G59）
    enum class WorkCoordinate {
        G54 = 0,
        G55,
        G56,
        G57,
        G58,
        G59
    };

    CoordinateSystem();
    ~CoordinateSystem() = default;

    // 设置当前工件坐标系
    void setActiveWorkCoordinate(WorkCoordinate coord);

    // 设置工件坐标系原点偏移
    void setWorkOffset(WorkCoordinate coord, const Point3D& offset);

    // 获取工件坐标系原点偏移
    Point3D getWorkOffset(WorkCoordinate coord) const;

    // 机械坐标转换为工件坐标
    Point3D machineToWork(const Point3D& machine) const;

    // 工件坐标转换为机械坐标
    Point3D workToMachine(const Point3D& work) const;

    // 设置相对坐标系原点
    void setRelativeOrigin(const Point3D& origin);

    // 绝对坐标转换为相对坐标
    Point3D absoluteToRelative(const Point3D& absolute) const;

    // 相对坐标转换为绝对坐标
    Point3D relativeToAbsolute(const Point3D& relative) const;

private:
    std::array<Point3D, 6> work_offsets_;  // G54-G59工件坐标系偏移量
    WorkCoordinate active_work_coord_;      // 当前激活的工件坐标系
    Point3D relative_origin_;               // 相对坐标系原点
};

} // namespace xxcnc