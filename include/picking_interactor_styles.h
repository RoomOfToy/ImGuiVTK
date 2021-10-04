#pragma once

#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>

#include <vtkInteractorStyleTrackballCamera.h>

#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkOpenGLRenderer.h>

#include <vtkNamedColors.h>
#include <vtkSphereSource.h>

#include <vtkPointPicker.h>
#include <vtkCellPicker.h>

namespace {
    // point picker
    class PointInteractorStyle : public vtkInteractorStyleTrackballCamera {
    public:
        static PointInteractorStyle* New();
        vtkTypeMacro(PointInteractorStyle, vtkInteractorStyleTrackballCamera);
        void OnLeftButtonDown() override {
            memcpy(startPickedPos, this->GetInteractor()->GetEventPosition(), 2 * sizeof(int));
            // Forward events
            vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
        }
        void OnLeftButtonUp() override {
            auto pos = this->GetInteractor()->GetEventPosition();
            auto dist2 = pow(startPickedPos[0] - pos[0], 2) + pow(startPickedPos[1] - pos[1], 2);
            // make sure not button down/up as drag
            if (dist2 < 9) {
                vtkSmartPointer<vtkRenderer> renderer = this->GetCurrentRenderer();
                auto picker = vtkSmartPointer<vtkPointPicker>::New();
                picker->Pick(pos[0], pos[1], 0, renderer);
                int id = picker->GetPointId();
                // point picked
                if (id != -1) {
                    RemovePickedActor();
                    //double point[3];
                    //picker->GetPickPosition(point);
                    double* point = picker->GetActor()->GetMapper()->GetInput()->GetPoint(id);
                    auto actor = vtkSmartPointer<vtkActor>::New();
                    actor->GetProperty()->SetAmbient(1.0);
                    actor->GetProperty()->SetDiffuse(1.0);
                    actor->GetProperty()->SetSpecular(1.0);
                    auto colors = vtkSmartPointer<vtkNamedColors>::New();
                    auto sphereSource = vtkSmartPointer<vtkSphereSource>::New();
                    sphereSource->SetCenter(point[0], point[1], point[2]);
                    // selected point radius
                    sphereSource->SetRadius(0.1);
                    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                    mapper->SetInputConnection(sphereSource->GetOutputPort());
                    actor->SetMapper(mapper);
                    // set selected point color to red
                    actor->GetProperty()->SetColor(colors->GetColor3d("Red").GetData());
                    renderer->AddActor(actor);
                    this->lastPickedActor = actor;
                }
            }
            // Forward events
            vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
        }
        void RemovePickedActor() {
            if (lastPickedActor != nullptr)
                this->GetCurrentRenderer()->RemoveActor(lastPickedActor);
        }
    private:
        vtkSmartPointer<vtkActor> lastPickedActor;
        int startPickedPos[2];
    };

    vtkStandardNewMacro(PointInteractorStyle);

    // actor picker
    class ActorInteractorStyle : public vtkInteractorStyleTrackballCamera {
    public:
        static ActorInteractorStyle* New();
        vtkTypeMacro(ActorInteractorStyle, vtkInteractorStyleTrackballCamera);
        void SetActors(std::vector<vtkSmartPointer<vtkActor>> _actors) {
            actors = _actors;
        }
        void OnLeftButtonDown() override {
            memcpy(startPickedPos, this->GetInteractor()->GetEventPosition(), 2 * sizeof(int));
            // Forward events
            vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
        }
        void OnLeftButtonUp() override {
            auto pos = this->GetInteractor()->GetEventPosition();
            auto dist2 = pow(startPickedPos[0] - pos[0], 2) + pow(startPickedPos[1] - pos[1], 2);
            // make sure not button down/up as drag
            if (dist2 < 9) {
                vtkSmartPointer<vtkRenderer> renderer = this->GetCurrentRenderer();
                auto picker = vtkSmartPointer<vtkPointPicker>::New();
                picker->Pick(pos[0], pos[1], 0, renderer);
                int id = picker->GetPointId();
                // actor picked
                if (id != -1) {
                    ResetActors();
                    picker->GetActor()->GetProperty()->SetColor(1, 0, 0);
                }
            }
            // Forward events
            vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
        }
        void ResetActors() {
            for (auto actor : actors)
                actor->GetProperty()->SetColor(0, 1, 0);
        }
    private:
        std::vector<vtkSmartPointer<vtkActor>> actors;
        int startPickedPos[2];
    };

    vtkStandardNewMacro(ActorInteractorStyle);
}
