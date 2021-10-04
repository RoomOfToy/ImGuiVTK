#pragma once

#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkSTLReader.h>
#include <vtkBYUReader.h>
#include <vtkSimplePointsReader.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyData.h>
#include <vtkSphereSource.h>

#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <string>
#include <iostream>

namespace {
    vtkSmartPointer<vtkPolyData> ReadPolyData(const char* fileName) {
        vtkSmartPointer<vtkPolyData> polyData;
        std::string extension =
            vtksys::SystemTools::GetFilenameLastExtension(std::string(fileName));

        // Drop the case of the extension
        std::transform(extension.begin(), extension.end(), extension.begin(),
            ::tolower);

        if (extension == ".ply")
        {
            vtkNew<vtkPLYReader> reader;
            reader->SetFileName(fileName);
            reader->Update();
            polyData = reader->GetOutput();
        }
        else if (extension == ".vtp")
        {
            vtkNew<vtkXMLPolyDataReader> reader;
            reader->SetFileName(fileName);
            reader->Update();
            polyData = reader->GetOutput();
        }
        else if (extension == ".obj")
        {
            vtkNew<vtkOBJReader> reader;
            reader->SetFileName(fileName);
            reader->Update();
            polyData = reader->GetOutput();
        }
        else if (extension == ".stl")
        {
            vtkNew<vtkSTLReader> reader;
            reader->SetFileName(fileName);
            reader->Update();
            polyData = reader->GetOutput();
        }
        else if (extension == ".vtk")
        {
            vtkNew<vtkPolyDataReader> reader;
            reader->SetFileName(fileName);
            reader->Update();
            polyData = reader->GetOutput();
        }
        else if (extension == ".g")
        {
            vtkNew<vtkBYUReader> reader;
            reader->SetGeometryFileName(fileName);
            reader->Update();
            polyData = reader->GetOutput();
        }
        else if (extension == ".xyz")
        {
            vtkNew<vtkSimplePointsReader> reader;
            reader->SetFileName(fileName);
            reader->Update();
            polyData = reader->GetOutput();
        }
        else
        {
            std::cerr << "Unsupported 3D Format, use default Sphere Source" << std::endl;
            vtkNew<vtkSphereSource> source;
            source->Update();
            polyData = source->GetOutput();
        }
        return polyData;
    }
}
