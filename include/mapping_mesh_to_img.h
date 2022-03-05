#pragma once

#include <vtkNew.h>
#include <vtkProperty.h>
#include <vtkImageActor.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkNamedColors.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCommand.h>
#include <vtkRendererCollection.h>
#include <iostream>


// two viewports: xmin, ymin, xmax, ymax [0, 0, 1, 0.5], [0, 0.5, 1, 1]

// render 0
void SetupModelRender(vtkSmartPointer<vtkRenderer> modelRenderer, vtkSmartPointer<vtkPolyData> meshData)
{
    vtkNew<vtkNamedColors> colors;

    // Create a mesh actor to display the mesh
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(meshData);

    vtkNew<vtkActor> meshActor;
    meshActor->SetMapper(mapper);

    // Wireframe
    meshActor->GetProperty()->SetRepresentationToWireframe();
    meshActor->GetProperty()->SetColor(colors->GetColor3d("Gold").GetData());

    modelRenderer->SetViewport(0, 0, 1, 0.5);
    modelRenderer->AddActor(meshActor);

    // TODO: scale polydata to fit the renderer size

    // Set interactor style for secene renderer
    modelRenderer->GetRenderWindow()->GetInteractor()->GetInteractorStyle()->SetDefaultRenderer(modelRenderer);
}

// scene: render 2, background: render 1
// return scene renderer
vtkSmartPointer<vtkRenderer> SetupSceneAndBackgroundRenders(vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow, vtkSmartPointer<vtkImageData> imgData, vtkSmartPointer<vtkPolyData> meshData)
{
    // Create an image actor to display the image
    vtkNew<vtkImageActor> imgActor;
    imgActor->SetInputData(imgData);

    vtkNew<vtkNamedColors> colors;

    // Create a mesh actor to display the mesh
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputData(meshData);

    vtkNew<vtkActor> meshActor;
    meshActor->SetMapper(mapper);
    meshActor->GetProperty()->SetAmbientColor(colors->GetColor3d("SaddleBrown").GetData());
    meshActor->GetProperty()->SetDiffuseColor(colors->GetColor3d("Sienna").GetData());
    meshActor->GetProperty()->SetSpecularColor(colors->GetColor3d("White").GetData());
    meshActor->GetProperty()->SetSpecular(0.51);
    meshActor->GetProperty()->SetDiffuse(0.7);
    meshActor->GetProperty()->SetAmbient(0.7);
    meshActor->GetProperty()->SetSpecularPower(30.0);
    meshActor->GetProperty()->SetOpacity(1.0);

    // Create a renderer to display the image in the background
    vtkNew<vtkRenderer> backgroundRenderer;
    vtkNew<vtkRenderer> sceneRenderer;

    backgroundRenderer->SetViewport(0, 0.5, 1, 1);
    sceneRenderer->SetViewport(0, 0.5, 1, 1);

    // Set up the render window and renderers such that there is
    // a background layer and a foreground layer
    backgroundRenderer->SetLayer(0);
    backgroundRenderer->InteractiveOff();
    sceneRenderer->SetLayer(1);
    renderWindow->SetNumberOfLayers(2);
    renderWindow->AddRenderer(backgroundRenderer);
    renderWindow->AddRenderer(sceneRenderer);

    // Add actors to the renderers
    sceneRenderer->AddActor(meshActor);
    backgroundRenderer->AddActor(imgActor);

    // Render once to figure out where the background camera will be
    renderWindow->Render();

    // Set up the background camera to fill the renderer with the image
    double origin[3];
    double spacing[3];
    int extent[6];
    imgData->GetOrigin(origin);
    imgData->GetSpacing(spacing);
    imgData->GetExtent(extent);

    vtkCamera* camera = backgroundRenderer->GetActiveCamera();
    camera->ParallelProjectionOn();

    double xc = origin[0] + 0.5 * (extent[0] + extent[1]) * spacing[0];
    double yc = origin[1] + 0.5 * (extent[2] + extent[3]) * spacing[1];
    // double xd = (extent[1] - extent[0] + 1)*spacing[0];
    double yd = (extent[3] - extent[2] + 1) * spacing[1];
    double d = camera->GetDistance();
    camera->SetParallelScale(0.5 * yd);
    camera->SetFocalPoint(xc, yc, 0.0);
    camera->SetPosition(xc, yc, d);

    // Render again to set the correct view
    renderWindow->Render();

    return sceneRenderer;
}
