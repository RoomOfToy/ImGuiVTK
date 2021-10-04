#include "ImGuiVTK.h"

#include <vtkNew.h>
#include <vtkCallbackCommand.h>
#include <vtkCommand.h>

void ImGuiVTK::IsCurrentCallbackFn(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData) {
    bool* isCurrent = static_cast<bool*>(callData);
    *isCurrent = true;
}

void ImGuiVTK::Init() {
    if (Renderer == nullptr) {
        Renderer = vtkSmartPointer<vtkRenderer>::New();
        Renderer->ResetCamera();
        Renderer->SetBackground(0.39, 0.39, 0.39);
    }

    if (InteractorStyle == nullptr)
        InteractorStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    InteractorStyle->SetDefaultRenderer(Renderer);

    if (Interactor == nullptr)
        Interactor = vtkSmartPointer<vtkGenericRenderWindowInteractor>::New();
    Interactor->SetInteractorStyle(InteractorStyle);
    Interactor->EnableRenderOff();

    if (RenderWindow == nullptr)
        RenderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    RenderWindow->SetSize(ViewportSize);
    vtkNew<vtkCallbackCommand> isCurrentCallback;
    isCurrentCallback->SetCallback(IsCurrentCallbackFn);
    RenderWindow->AddObserver(vtkCommand::WindowIsCurrentEvent, isCurrentCallback);
    RenderWindow->SwapBuffersOn();
    RenderWindow->UseOffScreenBuffersOff();

    RenderWindow->AddRenderer(Renderer);
    RenderWindow->SetInteractor(Interactor);

    FBOHdl = 0;
    RBOHdl = 0;
    TexHdl = 0;
    ViewportSize[0] = 640;
    ViewportSize[1] = 480;
    Show = true;

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
}

void ImGuiVTK::ShutDown() {
    Renderer = nullptr;
    InteractorStyle = nullptr;
    Interactor = nullptr;
    RenderWindow = nullptr;

    glDeleteBuffers(1, &FBOHdl);
    glDeleteBuffers(1, &RBOHdl);
    glDeleteBuffers(1, &TexHdl);
}

void ImGuiVTK::SetViewportSize(int w, int h) {
    RenderWindow->SetShowWindow(Show);
    if (ViewportSize[0] == w && ViewportSize[1] == h)
        return;
    if (w == 0 || h == 0)
        return;

    ViewportSize[0] = w;
    ViewportSize[1] = h;

    glGenTextures(1, &TexHdl);
    glBindTexture(GL_TEXTURE_2D, TexHdl);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ViewportSize[0], ViewportSize[1], 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &RBOHdl);
    glBindRenderbuffer(GL_RENDERBUFFER, RBOHdl);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, ViewportSize[0], ViewportSize[1]);
    glBindRenderbuffer(GL_RENDERBUFFER, RBOHdl);

    glGenFramebuffers(1, &FBOHdl);
    glBindFramebuffer(GL_FRAMEBUFFER, FBOHdl);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexHdl, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBOHdl);
    RenderWindow->InitializeFromCurrentContext();
    RenderWindow->SetSize(ViewportSize);
    Interactor->SetSize(ViewportSize);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ImGuiVTK::ProcessEvents() {
    if (!ImGui::IsWindowFocused())
        return;

    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    ImVec2 wPos = ImGui::GetWindowPos();

    double xpos = static_cast<double>(io.MousePos[0]) - static_cast<double>(ImGui::GetWindowPos().x);
    double ypos = static_cast<double>(io.MousePos[1]) - static_cast<double>(ImGui::GetWindowPos().y);
    int ctrl = static_cast<int>(io.KeyCtrl);
    int shift = static_cast<int>(io.KeyShift);
    bool dclick = io.MouseDoubleClicked[0] || io.MouseDoubleClicked[1] || io.MouseDoubleClicked[2];

    Interactor->SetEventInformationFlipY(xpos, ypos, ctrl, shift, dclick);

    if (io.MouseClicked[ImGuiMouseButton_Left])
        Interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent, nullptr);
    else if (io.MouseReleased[ImGuiMouseButton_Left])
        Interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, nullptr);
    else if (io.MouseClicked[ImGuiMouseButton_Right])
        Interactor->InvokeEvent(vtkCommand::RightButtonPressEvent, nullptr);
    else if (io.MouseReleased[ImGuiMouseButton_Right])
        Interactor->InvokeEvent(vtkCommand::RightButtonReleaseEvent, nullptr);
    else if (io.MouseWheel > 0)
        Interactor->InvokeEvent(vtkCommand::MouseWheelForwardEvent, nullptr);
    else if (io.MouseWheel < 0)
        Interactor->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, nullptr);

    Interactor->InvokeEvent(vtkCommand::MouseMoveEvent, nullptr);
}

void ImGuiVTK::Render() {
    if (!ImGui::Begin(Title.c_str(), &Show, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        ImGui::End();
    else
    {
        SetViewportSize(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBOHdl); // required since we set BlitToCurrent = On.
        RenderWindow->Render();
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        ImGui::BeginChild("##Viewport", ImVec2(0.0f, -ImGui::GetTextLineHeightWithSpacing() - 16.0f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ProcessEvents();
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::Image((void*)TexHdl,
                     ImGui::GetContentRegionAvail(),
                     ImVec2(0, 1), ImVec2(1, 0));
        ImGui::EndChild();
        ImGui::End();
    }
}

void ImGuiVTK::AddProp(vtkSmartPointer<vtkProp> prop) {
    Renderer->AddActor(prop);
    Renderer->ResetCamera();
}

void ImGuiVTK::AddProps(vtkSmartPointer<vtkPropCollection> props) {
    props->InitTraversal();
    vtkProp* prop;
    vtkCollectionSimpleIterator sit;
    for (props->InitTraversal(sit);
        (prop = props->GetNextProp(sit));)
    {
        Renderer->AddActor(prop);
        Renderer->ResetCamera();
    }
}

void ImGuiVTK::RemoveProp(vtkSmartPointer<vtkProp> prop) {
    Renderer->RemoveActor(prop);
}

void ImGuiVTK::RemoveProps(vtkSmartPointer<vtkPropCollection> props) {
    props->InitTraversal();
    vtkProp* prop;
    vtkCollectionSimpleIterator sit;
    for (props->InitTraversal(sit);
        (prop = props->GetNextProp(sit));)
    {
        Renderer->RemoveActor(prop);
    }
}
