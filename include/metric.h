#pragma once

#include <string>
#include <vector>
#include <ostream>

struct CameraParameters
{
    double azimuth;
    double elevation;
    double distance;
    double inplane_rotation;
    int principal_point[2];

    double camera_position[3];
    double camera_focal_point[3];
    double model_mass_center_position[3];
};

// calculate principal point (u, v) on image pixel coordinate
CameraParameters GetCameraParameters(void* scene_renderer_, double scene_movement[3], int img_extent[6]);

void GetCameraPosition(double azimuth, double elevation, double distance, double cam_pos[3]);

struct Metric
{
    std::string model_name;
    std::string model_category;
    bool truncated;
    bool occluded;
    CameraParameters camera_parameters;
    std::string model_path;
};
struct Metrics
{
    std::string image_name;
    std::string image_path;
    std::vector<Metric> metrics;
};

void WriteMetricsToFile(std::string const& filepath, Metrics const& metrics);

Metrics ReadMetricsFromFile(std::string const& filepath);

std::ostream& operator<<(std::ostream& os, CameraParameters camera_parameters);

std::ostream& operator<<(std::ostream& os, Metric metric);

std::ostream& operator<<(std::ostream& os, Metrics metrics);
