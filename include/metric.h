#pragma once

#include <string>
#include <fstream>

struct Metric
{
    std::string model_name;
    std::string model_category;
    bool truncated;
    bool occluded;
    double camera_position[3];
    double camera_focal_point[3];
    double camera_view_up[3];
    double camera_projection_direction[3];
    double inplane_rotation;
    double model_center_of_mass[3];
    std::string model_path;
};
struct Metrics
{
    std::string image_name;
    std::string image_path;
    std::vector<Metric> metrics;
};

void WriteMetricsToFile(std::string const& filepath, Metrics const& metrics)
{
    std::ofstream file(filepath);
    // TODO: handle error
    file << metrics.image_name << ',' << metrics.image_path << "\n\n";
    for (auto const& m : metrics.metrics)
        file << m.model_name << ',' << m.model_category << ','
             << m.truncated << ',' << m.occluded << ','
             << '[' << m.camera_position[0] << ' ' << m.camera_position[1] << ' ' << m.camera_position[2] << "],"
             << '[' << m.camera_focal_point[0] << ' ' << m.camera_focal_point[1] << ' ' << m.camera_focal_point[2] << "],"
             << '[' << m.camera_view_up[0] << ' ' << m.camera_view_up[1] << ' ' << m.camera_view_up[2] << "],"
             << '[' << m.camera_projection_direction[0] << ' ' << m.camera_projection_direction[1] << ' ' << m.camera_projection_direction[2] << "],"
             << m.inplane_rotation << ','
             << '[' << m.model_center_of_mass[0] << ' ' << m.model_center_of_mass[1] << ' ' << m.model_center_of_mass[2] << "],"
             << m.model_path << '\n';
}