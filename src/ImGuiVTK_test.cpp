#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING  // vtk only targets C++11, so disable this iterator warning

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>  // to eliminate warning C4005: 'APIENTRY' : macro redefinition, glad.h should be included after Windows.h

#include <vtkSmartPointer.h>
#include <vtkActor.h>

// use Adobe spectrum ImGUI fork
#include "imgui_spectrum.h"  // use light theme in default
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiVTK.h"
#include "imfilebrowser.h"
#include "my_pipeline.h"
#include <stdio.h>

#include <glad/glad.h>

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int argc, char* argv[])
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Main Window", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    bool err = gladLoadGL() == 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    ImGui::Spectrum::StyleColorsSpectrum();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // setup vtk
    vtkNew<vtkGenericRenderWindowInteractor> interactor;
    auto widget = SetUpAxesWidget(interactor);
    ImGuiVTK instance;
    instance.Interactor = interactor;
    instance.Init();
    instance.SetCtrl(true);  // enable ctrl
    instance.SetShift(true);  // enable shift
    widget->SetEnabled(1);
    widget->InteractiveOn();
    auto props = vtkSmartPointer<vtkPropCollection>::New();

    // file browser
    ImGui::FileBrowser fileDialog;
    fileDialog.SetTitle("FileSelection");
    fileDialog.SetTypeFilters({ ".stl", ".obj" });  // mesh for volume rendering

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);

    std::string FileName;
    vtkSmartPointer<vtkPolyData> PolyData = nullptr;
    vtkSmartPointer<vtkImageData> ImgData = nullptr;

    float SpacingX = 0.1f, SpacingY = 0.1f, SpacingZ = 0.1f;
    float SampleDistance = 0.1f, ImgSampleDistance = 1.f;
    double Iso1 = 0.5, Iso2 = 1.5;
    ImVec4 Iso1Color = ImVec4(1.00f, 0.96f, 0.93f, 1.00f), Iso2Color = ImVec4(0.78f, 0.47f, 0.15f, 1.00f);

    const char* RayCastType[] = { "FixedPointVolumeRayCast", "GPUVolumeRayCast", "SmartVolume" };
    int CurrentRayCastType = 1;

    double ClipPlaneOrigin[3] = { 0 }, ClipPlaneNormal[3] = { 0 };

    bool GridOn = true;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // volume rendering adjustments
        ImGui::Begin("Rendering Config");
        ImGui::SliderFloat("SpacingX", &SpacingX, 0.0f, 1.0f);
        ImGui::SliderFloat("SpacingY", &SpacingY, 0.0f, 1.0f);
        ImGui::SliderFloat("SpacingZ", &SpacingZ, 0.0f, 1.0f);
        ImGui::InputFloat("SampleDistance", &SampleDistance);
        ImGui::SliderFloat("ImgSampleDistance", &ImgSampleDistance, 1.f, 100.f);
        ImGui::InputDouble("ISO1", &Iso1);
        ImGui::InputDouble("ISO2", &Iso2);
        ImGui::ColorEdit3("ISO1 Color", (float*)&Iso1Color);
        ImGui::ColorEdit3("ISO2 Color", (float*)&Iso2Color);
        ImGui::ListBox("RayCastType", &CurrentRayCastType, RayCastType, IM_ARRAYSIZE(RayCastType), 4);
        ImGui::Checkbox("GridOn", &GridOn);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.3f, 0.4f, 1.0f });
        if (ImGui::Button("Config") && PolyData != nullptr)
        {
            // clean up old props
            if (props->GetNumberOfItems() != 0) instance.RemoveProps(props);
            // Setup actor pipeline
            double spacing[3] = { SpacingX, SpacingY, SpacingZ };
            ImgData = ConvertMeshPolyDataToImageData(PolyData, spacing);
            double color1[3] = { Iso1Color.x, Iso1Color.y, Iso1Color.z };
            double color2[3] = { Iso2Color.x, Iso2Color.y, Iso2Color.z };
            props = SetupMyActorsForRayCast(FileName, ImgData, static_cast<VolumeType>(CurrentRayCastType), SampleDistance, ImgSampleDistance, Iso1, Iso2, color1, color2);
            if (GridOn)
                props->AddItem(GetCubeAxesActor(instance.Renderer->GetActiveCamera(), ImgData->GetBounds()));
            instance.AddProps(props);
            fileDialog.ClearSelected();
        }
        ImGui::PopStyleColor(1);
        ImGui::End();

        // clip
        ImGui::Begin("Clip");
        ImGui::InputScalarN("PlaneOrigin", ImGuiDataType_Double, ClipPlaneOrigin, 3, NULL, NULL, "%.6f");
        ImGui::InputScalarN("PlaneNormal", ImGuiDataType_Double, ClipPlaneNormal, 3, NULL, NULL, "%.6f");
        if (ImGui::Button("Clip") && PolyData != nullptr && props->GetNumberOfItems() != 0) {
            instance.RemoveProps(props);
            SetClipPlane(static_cast<vtkVolume*>(props->GetItemAsObject(0)), ClipPlaneOrigin, ClipPlaneNormal);
            instance.AddProps(props);
        }
        ImGui::SameLine();
        if (ImGui::Button("UnsetClip") && PolyData != nullptr && props->GetNumberOfItems() != 0) {
            instance.RemoveProps(props);
            UnSetClip(static_cast<vtkVolume*>(props->GetItemAsObject(0)));
            instance.AddProps(props);
        }
        ImGui::End();

        // file browser
        if (ImGui::Begin("FileBrowser"))
        {
            // open file dialog when user clicks this button
            if (ImGui::Button("Open File"))
                fileDialog.Open();
        }
        ImGui::End();
        fileDialog.Display();
        if (fileDialog.HasSelected())
        {
            // clean up old props
            if (props->GetNumberOfItems() != 0) instance.RemoveProps(props);
            // Setup actor pipeline
            FileName = fileDialog.GetSelected().string();
            PolyData = ReadPolyData(FileName.c_str());
            double spacing[3] = { SpacingX, SpacingY, SpacingZ };
            ImgData = ConvertMeshPolyDataToImageData(PolyData, spacing);
            double color1[3] = { Iso1Color.x, Iso1Color.y, Iso1Color.z };
            double color2[3] = { Iso2Color.x, Iso2Color.y, Iso2Color.z };
            props = SetupMyActorsForRayCast(FileName, ImgData, static_cast<VolumeType>(CurrentRayCastType), SampleDistance, ImgSampleDistance, Iso1, Iso2, color1, color2);
            if (GridOn)
                props->AddItem(GetCubeAxesActor(instance.Renderer->GetActiveCamera(), ImgData->GetBounds()));
            instance.AddProps(props);
            fileDialog.ClearSelected();
        }

        // Rendering

        instance.Render();
        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glScissor(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    instance.ShutDown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}