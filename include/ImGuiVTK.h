#pragma once

#include "imgui.h"
#include <cstdio>
#include <cstdint>
#include <glad/glad.h> 

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkPropCollection.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGenericRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>

class ImGuiVTK
{
public:
	using IsCurrentCallbackFnType = void(vtkObject* caller, long unsigned int eventId, void* clientData, void* callData);

public:
	void Init();
	void ShutDown();

	void SetViewportSize(int w, int h);
	void Render();
	void AddProp(vtkSmartPointer<vtkProp> prop);
	void AddProps(vtkSmartPointer<vtkPropCollection> props);
	void RemoveProp(vtkSmartPointer<vtkProp> prop);
	void RemoveProps(vtkSmartPointer<vtkPropCollection> props);

	void SetCtrl(bool ctrl);
	void SetShift(bool shift);

private:
	void ProcessEvents();

public:
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> RenderWindow = nullptr;
	vtkSmartPointer<vtkGenericRenderWindowInteractor> Interactor = nullptr;
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> InteractorStyle = nullptr;
	vtkSmartPointer<vtkRenderer> Renderer = nullptr;
	static IsCurrentCallbackFnType IsCurrentCallbackFn;

private:
	GLuint FBOHdl = 0;
	GLuint RBOHdl = 0;
	GLuint TexHdl = 0;

	int ViewportSize[2] = { 640, 480 };
	std::string Title = "ModelView";
	bool Show = true;

	bool Ctrl = false;   // disabled
	bool Shift = false;  // disbaled
};
