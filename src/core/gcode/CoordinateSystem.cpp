#include "CoordinateSystem.h"

namespace xxcnc {

CoordinateSystem::CoordinateSystem()
    : active_work_coord_(WorkCoordinate::G54)
    , relative_origin_(0.0, 0.0, 0.0) {
    // 初始化所有工件坐标系偏移量为零
    for (auto& offset : work_offsets_) {
        offset = Point3D(0.0, 0.0, 0.0);
    }
}

void CoordinateSystem::setActiveWorkCoordinate(WorkCoordinate coord) {
    active_work_coord_ = coord;
}

void CoordinateSystem::setWorkOffset(WorkCoordinate coord, const Point3D& offset) {
    work_offsets_[static_cast<size_t>(coord)] = offset;
}

Point3D CoordinateSystem::getWorkOffset(WorkCoordinate coord) const {
    return work_offsets_[static_cast<size_t>(coord)];
}

Point3D CoordinateSystem::machineToWork(const Point3D& machine) const {
    // 机械坐标减去工件坐标系偏移量得到工件坐标
    const Point3D& offset = work_offsets_[static_cast<size_t>(active_work_coord_)];
    return machine - offset;
}

Point3D CoordinateSystem::workToMachine(const Point3D& work) const {
    // 工件坐标加上工件坐标系偏移量得到机械坐标
    const Point3D& offset = work_offsets_[static_cast<size_t>(active_work_coord_)];
    return work + offset;
}

void CoordinateSystem::setRelativeOrigin(const Point3D& origin) {
    relative_origin_ = origin;
}

Point3D CoordinateSystem::absoluteToRelative(const Point3D& absolute) const {
    // 绝对坐标减去相对坐标系原点得到相对坐标
    return absolute - relative_origin_;
}

Point3D CoordinateSystem::relativeToAbsolute(const Point3D& relative) const {
    // 相对坐标加上相对坐标系原点得到绝对坐标
    return relative + relative_origin_;
}

} // namespace xxcnc