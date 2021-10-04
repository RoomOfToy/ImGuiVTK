#pragma once

#include <vtkNew.h>
#include <vtkProperty.h>

#include <vtkRenderer.h>
#include <vtkNamedColors.h>
#include <vtkLight.h>
#include <vtkShadowMapBakerPass.h>
#include <vtkShadowMapPass.h>
#include <vtkCameraPass.h>
#include <vtkSequencePass.h>
#include <vtkRenderPassCollection.h>
#include <vtkOpenGLRenderer.h>
#include <vtkCamera.h>

#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>

#include <vtkActor2D.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>

#include <vtkCubeSource.h>

#include <vtksys/SystemTools.hxx>

#include <array>

namespace {
    vtkNew<vtkRenderer> SetupMyShadowRender() {
        vtkNew<vtkNamedColors> colors;
        colors->SetColor("HighNoonSun", 1.0, 1.0, .9843, 1.0); // Color temp. 5400k
        colors->SetColor("100W Tungsten", 1.0, .8392, .6667,
            1.0); // Color temp. 2850k

        vtkNew<vtkRenderer> renderer;
        renderer->SetBackground(colors->GetColor3d("Silver").GetData());

        vtkNew<vtkLight> light1;
        light1->SetFocalPoint(0, 0, 0);
        light1->SetPosition(0, 10, 5);
        light1->SetColor(colors->GetColor3d("HighNoonSun").GetData());
        light1->SetIntensity(0.3);
        renderer->AddLight(light1);

        vtkNew<vtkLight> light2;
        light2->SetFocalPoint(0, 0, 0);
        light2->SetPosition(10, 10, 10);
        light2->SetColor(colors->GetColor3d("100W Tungsten").GetData());
        light2->SetIntensity(0.8);
        renderer->AddLight(light2);


        // FIXME: vtkShaderProgram.cxx:453    ERR| vtkShaderProgram (00000160A2A8CCF0): 0(174) : error C1503: undefined variable "vertexVC"
        // this problem is for .xyz file ...
        vtkNew<vtkShadowMapPass> shadows;

        vtkNew<vtkSequencePass> seq;

        vtkNew<vtkRenderPassCollection> passes;
        passes->AddItem(shadows->GetShadowMapBakerPass());
        passes->AddItem(shadows);
        seq->SetPasses(passes);

        vtkNew<vtkCameraPass> cameraP;
        cameraP->SetDelegatePass(seq);

        // tell the renderer to use our render pass pipeline
        vtkOpenGLRenderer* glrenderer =
            dynamic_cast<vtkOpenGLRenderer*>(renderer.GetPointer());
        glrenderer->SetPass(cameraP);


        // set init camera pose
        renderer->GetActiveCamera()->SetPosition(5, 10, 10);
        renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
        renderer->GetActiveCamera()->SetViewUp(0, 0, 1);
        renderer->ResetCamera();
        renderer->GetActiveCamera()->Dolly(2.25);
        renderer->ResetCameraClippingRange();

        return renderer;
    }

    vtkSmartPointer<vtkPropCollection> SetupMyActorsForShadow(vtkSmartPointer<vtkPolyData> polyData, std::string const& polyDataName) {
        vtkNew<vtkNamedColors> colors;
        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputData(polyData);

        vtkNew<vtkActor> actor;
        actor->SetMapper(mapper);
        actor->GetProperty()->SetAmbientColor(
            colors->GetColor3d("SaddleBrown").GetData());
        actor->GetProperty()->SetDiffuseColor(colors->GetColor3d("Sienna").GetData());
        actor->GetProperty()->SetSpecularColor(colors->GetColor3d("White").GetData());
        actor->GetProperty()->SetSpecular(0.51);
        actor->GetProperty()->SetDiffuse(0.7);
        actor->GetProperty()->SetAmbient(0.7);
        actor->GetProperty()->SetSpecularPower(30.0);
        actor->GetProperty()->SetOpacity(1.0);

        // add a plane
        std::array<double, 6> bounds;
        polyData->GetBounds(bounds.data());

        std::array<double, 3> range;
        range[0] = bounds[1] - bounds[0];  // x
        range[1] = bounds[3] - bounds[2];  // y
        range[2] = bounds[5] - bounds[4];  // z
        double expand = 1.0;
        auto thickness = range[2] * 0.1;
        vtkNew<vtkCubeSource> plane;
        plane->SetCenter((bounds[1] + bounds[0]) / 2.0, bounds[2] - thickness / 2.0,
            (bounds[5] + bounds[4]) / 2.0);
        plane->SetXLength(bounds[1] - bounds[0] + (range[0] * expand));
        plane->SetYLength(thickness);
        plane->SetZLength(bounds[5] - bounds[4] + (range[2] * expand));

        vtkNew<vtkPolyDataMapper> planeMapper;
        planeMapper->SetInputConnection(plane->GetOutputPort());

        vtkNew<vtkActor> planeActor;
        planeActor->SetMapper(planeMapper);

        // text
        vtkNew<vtkTextProperty> textProperty;
        textProperty->SetFontSize(16);
        textProperty->SetColor(0.3, 0.3, 0.3);

        vtkNew<vtkTextMapper> textMapper;
        textMapper->SetTextProperty(textProperty);
        textMapper->SetInput(vtksys::SystemTools::GetFilenameName(polyDataName).c_str());

        vtkNew<vtkActor2D> textActor;
        textActor->SetMapper(textMapper);
        textActor->SetPosition(20, 20);

        vtkNew<vtkPropCollection> actors;
        actors->AddItem(actor);
        actors->AddItem(planeActor);
        actors->AddItem(textActor);

        return actors;
    }
}
