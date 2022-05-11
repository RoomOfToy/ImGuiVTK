#include "metric.h"
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <json.hpp>
#include <fstream>

using json = nlohmann::json;

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
CameraParameters GetCameraParameters(void* scene_renderer_, double scene_movement[3], int img_extent[6])
{
    auto scene_renderer = static_cast<vtkRenderer*>(scene_renderer_);
    auto camera = scene_renderer->GetActiveCamera();

    double camera_position[3];
    double camera_focal_point[3];

    camera->GetPosition(camera_position);
    camera->GetFocalPoint(camera_focal_point);

    double inplane_rotation = camera->GetRoll();

    double distance = camera->GetDistance();
    double vx = camera_position[0] - camera_focal_point[0];
    double vy = camera_position[1] - camera_focal_point[1];
    double vz = camera_position[2] - camera_focal_point[2];

    const double PI = 3.141592653589793238463;

    double elevation = atan2(vz, sqrt(vx * vx + vy * vy)) * 180.0 / PI;  // [-180, 180]

    double azimuth = atan2(vy, vx) * 180.0 / PI;  // [-180, 180]

    // update mesh mass center position
    double new_mass_center[3];
    new_mass_center[0] = camera_focal_point[0] + scene_movement[0];
    new_mass_center[1] = camera_focal_point[1] + scene_movement[1];
    new_mass_center[2] = camera_focal_point[2] + scene_movement[2];

    // mass center in scene renderer and transform it to viewport coordinate
    double vpp[3]{ new_mass_center[0], new_mass_center[1], new_mass_center[2] };
    world2view(scene_renderer, vpp[0], vpp[1], vpp[2]);
    // image center in background renderer and its viewport coordinate is (0, 0)

    // they are different renderers! image is on backgroundRenderer!
    double img_width = img_extent[1] - img_extent[0] + 1;
    double img_height = img_extent[3] - img_extent[2] + 1;
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

    return CameraParameters{
        azimuth,
        elevation,
        distance,
        inplane_rotation,
        { principal_point[0], principal_point[1] },

        { camera_position[0], camera_position[1], camera_position[2] },
        { camera_focal_point[0], camera_focal_point[1], camera_focal_point[2] },
        { new_mass_center[0], new_mass_center[1], new_mass_center[2] }
    };
}

void GetCameraPosition(double azimuth, double elevation, double distance, double cam_pos[3])
{
    cam_pos[0] = distance * cos(elevation) * cos(azimuth);
    cam_pos[1] = distance * cos(elevation) * sin(azimuth);
    cam_pos[2] = distance * sin(elevation);
}

void to_json(json& j, CameraParameters const& camera_parameters)
{
    j = json{ { "azimuth", camera_parameters.azimuth },
              { "elevation", camera_parameters.elevation },
              { "distance", camera_parameters.distance },
              { "inplane_rotation", camera_parameters.inplane_rotation },
              { "principal_point", camera_parameters.principal_point },

              { "camera_position", camera_parameters.camera_position },
              { "camera_focal_point", camera_parameters.camera_focal_point },
              { "model_mass_center_position", camera_parameters.model_mass_center_position },
            };
}

void from_json(json const& j, CameraParameters& camera_parameters)
{
    j.at("azimuth").get_to(camera_parameters.azimuth);
    j.at("elevation").get_to(camera_parameters.elevation);
    j.at("distance").get_to(camera_parameters.distance);
    j.at("inplane_rotation").get_to(camera_parameters.inplane_rotation);
    j.at("principal_point").get_to(camera_parameters.principal_point);

    j.at("camera_position").get_to(camera_parameters.camera_position);
    j.at("camera_focal_point").get_to(camera_parameters.camera_focal_point);
    j.at("model_mass_center_position").get_to(camera_parameters.model_mass_center_position);
}

void to_json(json& j, Metric const& metric)
{
    j = json{ { "model_name", metric.model_name },
              { "model_category", metric.model_category },
              { "truncated", metric.truncated },
              { "occluded", metric.occluded },
              { "camera_parameters", metric.camera_parameters },
              { "model_path", metric.model_path },
            };
}

void from_json(json const& j, Metric& metric)
{
    j.at("model_name").get_to(metric.model_name);
    j.at("model_category").get_to(metric.model_category);
    j.at("truncated").get_to(metric.truncated);
    j.at("occluded").get_to(metric.occluded);
    j.at("camera_parameters").get_to(metric.camera_parameters);
    j.at("model_path").get_to(metric.model_path);
}

void to_json(json& j, Metrics const& metrics)
{
    j = json{ { "image_name", metrics.image_name },
              { "image_path", metrics.image_path },
              { "metrics", metrics.metrics },
            };
}

void from_json(json const& j, Metrics& metrics)
{
    j.at("image_name").get_to(metrics.image_name);
    j.at("image_path").get_to(metrics.image_path);
    j.at("metrics").get_to(metrics.metrics);
}

void WriteMetricsToFile(std::string const& filepath, Metrics const& metrics)
{
    std::ofstream file(filepath);
    // TODO: handle error
    json j;
    to_json(j, metrics);
    file << j;
}

Metrics ReadMetricsFromFile(std::string const& filepath)
{
    std::ifstream file(filepath);
    // TODO: handle error
    json j = json::parse(file);
    Metrics m;
    from_json(j, m);
    return m;
}

std::ostream& operator<<(std::ostream& os, CameraParameters camera_parameters)
{
    json j;
    to_json(j, camera_parameters);
    os << std::setw(4) << j;
    return os;
}

std::ostream& operator<<(std::ostream& os, Metric metric)
{
    json j;
    to_json(j, metric);
    os << std::setw(4) << j;
    return os;
}

std::ostream& operator<<(std::ostream& os, Metrics metrics)
{
    json j;
    to_json(j, metrics);
    os << std::setw(4) << j;
    return os;
}
