#pragma once

#include <string>
#include <fstream>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkVector.h>
#include <vtkCoordinate.h>
#include <vtkMatrix4x4.h>

struct CameraParameters
{
    double azimuth;
    double elevation;
    double distance;
    double inplane_rotation;
    int principal_point[2];
};

void world2view(vtkRenderer* renderer, double& x, double& y, double& z)
{
    double coordinates[3] { x, y, z };
    renderer->SetWorldPoint(coordinates);
    renderer->WorldToView();
    renderer->GetViewPoint(coordinates);
    x = coordinates[0];
    y = coordinates[1];
    //std::cout << "x: " << x << ", y: " << y << '\n';  // x [-0.5, 0.5], y [-1, 1]
    //renderer->ViewportToNormalizedViewport(x, y);  // normalize it

    // auto vp = renderer->GetViewport();
    // std::cout << "viewport: " << vp[0] << ", " << vp[1] << ", " << vp[2] << ", " << vp[3] << '\n';
    // auto sz = renderer->GetRenderWindow()->GetSize();
    // std::cout << "viewport size: " << sz[0] << ", " << sz[1] / 2 << '\n';
    // std::cout << "movement from viewport center: " << sz[0] / 2 * x << ", " << sz[1] / 4 * y << '\n';
}

// FIXME: correct?
// calculate principal point (u, v) on image pixel coordinate
CameraParameters GetCameraParameters(vtkRenderer* scene_renderer, double scene_movement[3], vtkSmartPointer<vtkImageData> imgData)
{
    auto camera = scene_renderer->GetActiveCamera();

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

    double elevation = atan2(vz, sqrt(vx * vx + vy * vy)) * 180.0 / PI;  // [-180, 180]

    double azimuth = atan2(vy, vx) * 180.0 / PI;  // [-180, 180]

    // std::cout << '[' << vx << ", " << vy << ", " << vz << "]; "
    //           << '[' << azimuth << ", " << elevation << ", " << distance << "]\n";

    // update mesh mass center position
    double new_mass_center[3];
    new_mass_center[0] = camera_focal_point[0] + scene_movement[0];
    new_mass_center[1] = camera_focal_point[1] + scene_movement[1];
    new_mass_center[2] = camera_focal_point[2] + scene_movement[2];

    //std::cout << "camera pos: " << camera_position[0] << ", " << camera_position[1] << ", " << camera_position[2] << '\n';
    //std::cout << "mass center pos: " << new_mass_center[0] << ", " << new_mass_center[1] << ", " << new_mass_center[2] << '\n';

    // // project mass center onto image plane (screen space, so need transfer to viewport coordinates...)
    // // focal point as image plane origin
    // auto proj_mat = camera->GetProjectionTransformMatrix(scene_renderer);
    // auto mv_mat = camera->GetModelViewTransformMatrix();
    // vtkNew<vtkMatrix4x4> mat;
    // vtkMatrix4x4::Multiply4x4(proj_mat, mv_mat, mat);
    // double in[4]{ new_mass_center[0], new_mass_center[1], new_mass_center[2], 1.0 };
    // double out[4]{ 0 };
    // mat->MultiplyPoint(in, out);
    // std::cout << "out: " << out[0] << ", " << out[1] << ", " << out[2] << ", " << out[3] << '\n';
    // int principal_point[2]{ int( out[0] ), -int( out[1] ) };  // left-up -- / left-down -+ / right-up +- / right-down ++

    // mass center in scene renderer and transform it to viewport coordinate
    double vpp[3]{ new_mass_center[0], new_mass_center[1], new_mass_center[2] };
    world2view(scene_renderer, vpp[0], vpp[1], vpp[2]);
    //std::cout << "world2view: " << vpp[0] << ", " << vpp[1] << '\n';
    // image center in background renderer and its viewport coordinate is (0, 0)

    // they are different renderers! image is on backgroundRenderer!
    int extent[6];
    imgData->GetExtent(extent);
    //std::cout << "extent: " << extent[0] << ", " << extent[1] << ", " << extent[2] << ", " << extent[3] << ", " << extent[4] << ", " << extent[5] << "\n";
    double img_width = extent[1] - extent[0] + 1;
    double img_height = extent[3] - extent[2] + 1;
    int principal_point[2]{ 0 };
    auto sz = scene_renderer->GetRenderWindow()->GetSize();
    if (img_width >= img_height)
    {
        auto y = img_height / 2.0;
        auto x_y_factor = double(sz[0]) / (sz[1] / 2.0);
        principal_point[1] = int(vpp[1] * y);
        principal_point[0] = int( vpp[0] * x_y_factor * y );
    }
    else
    {
        auto x = img_width / 2.0;
        auto y_x_factor = (sz[1] / 2.0) / double(sz[0]);
        principal_point[0] = int(vpp[0] * x);
        principal_point[1] = int( vpp[1] * y_x_factor * x );
    }

    //std::cout << "principal_point: " << principal_point[0] << ", " << principal_point[1] << '\n';

    return CameraParameters{
        azimuth,
        elevation,
        distance,
        inplane_rotation,
        { principal_point[0], principal_point[1] }
    };
}

void GetCameraPosition(double azimuth, double elevation, double distance, double cam_pos[3])
{
    cam_pos[0] = distance * cos(elevation) * cos(azimuth);
    cam_pos[1] = distance * cos(elevation) * sin(azimuth);
    cam_pos[2] = distance * sin(elevation);
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
             << '[' << m.camera_parameters.principal_point[0] << ' ' << m.camera_parameters.principal_point[1] << "],"
             << m.model_path << '\n';
}