#include <vtkSmartPointer.h>
#include <vtkActor.h>

// use Adobe spectrum ImGUI fork
#include "imgui_spectrum.h"  // use light theme in default
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiVTK.h"
#include "imfilebrowser.h"
#include "load3d.h"
#include "loadimg.h"
#include "mapping_mesh_to_img.h"
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
    ImGuiVTK instance;
    instance.Interactor = interactor;
    instance.Init();
    auto props = vtkSmartPointer<vtkPropCollection>::New();

    // file browser
    ImGui::FileBrowser imgFileDialog;
    imgFileDialog.SetTitle("ImgFileSelection");
    imgFileDialog.SetTypeFilters({ ".jpg", "jpeg", ".bmp", ".png", ".tif", ".pnm" });
    ImGui::FileBrowser meshFileDialog;
    meshFileDialog.SetTitle("MeshFileSelection");
    meshFileDialog.SetTypeFilters({ ".stl", "obj" });

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImVec2 button_size(ImGui::GetFontSize() * 7.0f, 0.0f);

    std::string MeshFileName, ImgFileName;
    vtkSmartPointer<vtkPolyData> PolyData = nullptr;
    vtkSmartPointer<vtkImageData> ImgData = nullptr;
    vtkSmartPointer<vtkRenderer> SceneRender = nullptr;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // file browser
        if (ImGui::Begin("MeshFileBrowser"))
        {
            if (ImGui::Button("Open Mesh File"))
                meshFileDialog.Open();
        }
        ImGui::End();
        meshFileDialog.Display();
        if (meshFileDialog.HasSelected())
        {
            // clean up old props
            if (props->GetNumberOfItems() != 0) instance.RemoveProps(props);
            // Setup actor pipeline
            MeshFileName = meshFileDialog.GetSelected().string();
            PolyData = ReadPolyData(MeshFileName.c_str());
            SetupModelRender(instance.Renderer, PolyData);
            // TODO: setup two renderers on ImGuiVTK instance.init() and make change mesh / image easier (one props for one)
            //props = SetupMyActorsForRayCast(FileName, ImgData, static_cast<VolumeType>(CurrentRayCastType), SampleDistance, ImgSampleDistance, Iso1, Iso2, color1, color2);
            instance.AddProps(props);
            meshFileDialog.ClearSelected();
        }
        if (ImGui::Begin("ImgFileBrowser"))
        {
            if (ImGui::Button("Open Image File"))
                imgFileDialog.Open();
        }
        ImGui::End();
        imgFileDialog.Display();
        if (imgFileDialog.HasSelected())
        {
            // clean up old props
            if (props->GetNumberOfItems() != 0) instance.RemoveProps(props);
            // Setup actor pipeline
            ImgFileName = imgFileDialog.GetSelected().string();
            ImgData = ReadImageData(ImgFileName.c_str());
            //props = SetupMyActorsForRayCast(FileName, ImgData, static_cast<VolumeType>(CurrentRayCastType), SampleDistance, ImgSampleDistance, Iso1, Iso2, color1, color2);
            SceneRender = SetupSceneAndBackgroundRenders(instance.RenderWindow, ImgData, PolyData);
            instance.AddProps(props);
            imgFileDialog.ClearSelected();
        }

        if (ImGui::Begin("Overlay"))
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.3f, 0.4f, 1.0f });
            if (ImGui::Button("Update Overlay") && SceneRender != nullptr)
            {
                vtkNew<vtkCamera> new_camera;

                new_camera->DeepCopy(instance.Renderer->GetActiveCamera());  // make a deep copy to break the camera dependecy of two renderers

                // auto camera = instance.Renderer->GetActiveCamera();
                // double v[3], u[2];
                // camera->GetFocalPoint(v);
                // new_camera->SetFocalPoint(v);
                // camera->GetPosition(v);
                // new_camera->SetPosition(v);
                // camera->GetViewUp(v);
                // new_camera->SetViewUp(v);
                // new_camera->SetViewAngle(camera->GetViewAngle());
                // camera->GetClippingRange(u);
                // new_camera->SetClippingRange(u);

                SceneRender->SetActiveCamera(new_camera);
            }
            ImGui::PopStyleColor(1);
        }
        ImGui::End();

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