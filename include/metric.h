#pragma once

#include <string>
#include <fstream>
#include <vtkCamera.h>

struct CameraParameters
{
    double azimuth;
    double elevation;
    double distance;
    double inplane_rotation;
    double principal_point[3];
};

// FIXME: correct?
CameraParameters GetCameraParameters(vtkCamera* camera)
{
    double camera_position[3];
    double camera_focal_point[3];

    camera->GetPosition(camera_position);
    camera->GetFocalPoint(camera_focal_point);

    double inplane_rotation = camera->GetRoll();

    double distance = camera->GetDistance();
    double vx = camera_focal_point[0] - camera_position[0];
    double vy = camera_focal_point[1] - camera_position[1];
    double vz = camera_focal_point[2] - camera_position[2];

    const double PI = 3.141592653589793238463;

    double elevation = asin(vy / distance) * 180.0 / PI;  // asin(v.y / distance)

    double azimuth = 0;
    
    if (abs(vz) < 0.00001)
    {
        // Special case
        if (vx > 0)
        {
            azimuth = PI/2.0;
        }
        else if (vx < 0)
        {
            azimuth = -PI/2.0;
        }
        else
        {
            azimuth = 0.0;
        }
    }
    else
    {
        azimuth = atan2(vx, vz);
    }

    azimuth *= 180.0 / PI;

    return CameraParameters{
        azimuth,
        elevation,
        distance,
        inplane_rotation,
        { camera_focal_point[0], camera_focal_point[1], camera_focal_point[2] }
    };
}

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

void WriteMetricsToFile(std::string const& filepath, Metrics const& metrics)
{
    std::ofstream file(filepath);
    // TODO: handle error
    file << metrics.image_name << ',' << metrics.image_path << "\n\n";
    for (auto const& m : metrics.metrics)
        file << m.model_name << ',' << m.model_category << ','
             << m.truncated << ',' << m.occluded << ','
             << m.camera_parameters.azimuth << ',' << m.camera_parameters.elevation << ',' << m.camera_parameters.distance << ','
             << m.camera_parameters.inplane_rotation << ','
             << '[' << m.camera_parameters.principal_point[0] << ' ' << m.camera_parameters.principal_point[1] << ' ' << m.camera_parameters.principal_point[2] << "],"
             << m.model_path << '\n';
}