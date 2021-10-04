#pragma once

#include <vtkSmartPointer.h>
#include <vtkNew.h>
#include <vtkNamedColors.h>
#include <vtkFixedPointVolumeRayCastMapper.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkPolyData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkContourValues.h>

#include <vtkPolyDataToImageStencil.h>
#include <vtkImageData.h>
#include <vtkImageStencil.h>
#include <vtkPointData.h>

#include <vtkActor2D.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>

#include <vtkPlane.h>

#include <vtkAxesActor.h>
#include <vtkCaptionActor2D.h>

#include <vtkOrientationMarkerWidget.h>

#include <vtkCubeAxesActor.h>

#include <vtksys/SystemTools.hxx>

#include <sstream>

namespace {
	// Require STL mesh data, need adjust spacing and sample distance for good volume rendering
	// https://vedo.embl.es/autodocs/_modules/vedo/volume.html
	vtkSmartPointer<vtkImageData> ConvertMeshPolyDataToImageData(vtkSmartPointer<vtkPolyData> polyData, 
		                                                         double const spacing[3])  // desired volume spacing
	{
		vtkNew<vtkImageData> whiteImage;
		double bounds[6];
		polyData->GetBounds(bounds);
		whiteImage->SetSpacing(spacing);

		// compute dimensions
		int dim[3];
		for (int i = 0; i < 3; i++)
		{
			dim[i] = static_cast<int>(
				ceil((bounds[i * 2 + 1] - bounds[i * 2]) / spacing[i]));
		}
		whiteImage->SetDimensions(dim);
		whiteImage->SetExtent(0, dim[0] - 1, 0, dim[1] - 1, 0, dim[2] - 1);

		double origin[3];
		origin[0] = bounds[0] + spacing[0] / 2;
		origin[1] = bounds[2] + spacing[1] / 2;
		origin[2] = bounds[4] + spacing[2] / 2;
		whiteImage->SetOrigin(origin);
		whiteImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

		// fill the image with foreground voxels:
		unsigned char inval = 255;
		unsigned char outval = 0;
		vtkIdType count = whiteImage->GetNumberOfPoints();
		for (vtkIdType i = 0; i < count; ++i)
		{
			whiteImage->GetPointData()->GetScalars()->SetTuple1(i, inval);
		}

		// polygonal data --> image stencil:
		vtkNew<vtkPolyDataToImageStencil> pol2stenc;
		pol2stenc->SetInputData(polyData);
		pol2stenc->SetOutputOrigin(origin);
		pol2stenc->SetOutputSpacing(spacing);
		pol2stenc->SetOutputWholeExtent(whiteImage->GetExtent());
		pol2stenc->Update();

		// cut the corresponding white image and set the background:
		vtkNew<vtkImageStencil> imgstenc;
		imgstenc->SetInputData(whiteImage);
		imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
		imgstenc->ReverseStencilOff();
		imgstenc->SetBackgroundValue(outval);
		imgstenc->Update();

		whiteImage->DeepCopy(imgstenc->GetOutput());

		return whiteImage;
	}

	enum class VolumeType {
		FixedPointVolumeRayCast,
		GPUVolumeRayCast,
		SmartVolume
	};

	vtkSmartPointer<vtkVolume> GetVolume(vtkSmartPointer<vtkImageData> imgData,
		                                 VolumeType type,
		                                 float sample_distance,
		                                 float img_sample_distance,
		                                 double iso1, double iso2,
		                                 double color1[3], double color2[3])
	{
		vtkNew<vtkNamedColors> colors;
		vtkNew<vtkColorTransferFunction> colorTransferFunction;
		vtkNew<vtkPiecewiseFunction> scalarOpacity;
		
		colorTransferFunction->RemoveAllPoints();
		colorTransferFunction->AddRGBPoint(iso2,
			color1[0],
			color1[1],
			color1[2]);
		colorTransferFunction->AddRGBPoint(iso1,
			color2[0],
			color2[1],
			color2[2]);

		scalarOpacity->AddPoint(iso1, 0.3);
		scalarOpacity->AddPoint(iso2, 0.6);

		vtkNew<vtkVolumeProperty> volumeProperty;
		volumeProperty->ShadeOn();
		volumeProperty->SetAmbient(0.4);
		volumeProperty->SetDiffuse(0.6);
		volumeProperty->SetSpecular(0.2);
		volumeProperty->SetInterpolationTypeToLinear();
		volumeProperty->SetColor(colorTransferFunction);
		volumeProperty->SetScalarOpacity(scalarOpacity);
		// Add some contour values to draw iso surfaces
		volumeProperty->GetIsoSurfaceValues()->SetValue(0, iso1);
		volumeProperty->GetIsoSurfaceValues()->SetValue(1, iso2);

		vtkNew<vtkVolume> volume;
		volume->SetProperty(volumeProperty);

		switch (type) {
		case VolumeType::FixedPointVolumeRayCast: {
			vtkNew<vtkFixedPointVolumeRayCastMapper> mapper;
			mapper->SetInputData(imgData);
			mapper->AutoAdjustSampleDistancesOff();
			mapper->SetSampleDistance(sample_distance);
			mapper->SetImageSampleDistance(img_sample_distance);
			mapper->SetBlendModeToComposite();

			volume->SetMapper(mapper);
			break;
		}
		case VolumeType::GPUVolumeRayCast: {
			vtkNew<vtkOpenGLGPUVolumeRayCastMapper> mapper;
			mapper->SetInputData(imgData);
			mapper->AutoAdjustSampleDistancesOff();
			mapper->SetSampleDistance(sample_distance);
			mapper->SetImageSampleDistance(img_sample_distance);
			mapper->SetBlendModeToIsoSurface();
			// vtkVolumeMapper::AVERAGE_INTENSITY_BLEND and ISOSURFACE_BLEND are only supported by the vtkGPUVolumeRayCastMapper with the OpenGL2 backend

			volume->SetMapper(mapper);
			break;
		}
		case VolumeType::SmartVolume: {
			vtkNew<vtkSmartVolumeMapper> mapper;
			mapper->SetInputData(imgData);
			mapper->AutoAdjustSampleDistancesOff();
			mapper->SetSampleDistance(sample_distance);
			mapper->SetRequestedRenderModeToDefault();  // use GPU if hardware supports else CPU
			//mapper->SetRequestedRenderModeToRayCast();  // ensure to use the software rendering mode (CPU) == FixedPointVolumeRayCast
			mapper->SetBlendModeToComposite();

			volume->SetMapper(mapper);
			break;
		}
		default: {
			vtkNew<vtkSmartVolumeMapper> mapper;
			mapper->SetInputData(imgData);
			mapper->AutoAdjustSampleDistancesOff();
			mapper->SetSampleDistance(sample_distance);
			mapper->SetRequestedRenderModeToDefault();
			mapper->SetBlendModeToComposite();

			volume->SetMapper(mapper);
			break;
		}
		}
		return volume;
	}
	
    vtkSmartPointer<vtkPropCollection> SetupMyActorsForRayCast(std::string const& imgDataName, 
		                                                       vtkSmartPointer<vtkImageData> imgData,
		                                                       VolumeType type,
		                                                       float sample_distance,
		                                                       float img_sample_distance,
		                                                       double iso1, double iso2,
		                                                       double color1[3], double color2[3])
	{
		// volume
		auto volume = GetVolume(imgData, type, sample_distance, img_sample_distance, iso1, iso2, color1, color2);

		// text
		vtkNew<vtkTextProperty> textProperty;
		textProperty->SetFontSize(16);
		textProperty->SetColor(0.3, 0.3, 0.3);

		vtkNew<vtkTextMapper> textMapper;
		textMapper->SetTextProperty(textProperty);
		int dim[3];
		imgData->GetDimensions(dim);
		std::ostringstream oss;
		oss << vtksys::SystemTools::GetFilenameName(imgDataName)
		    << "\nExtent X: " << dim[0]
			<< ", Y: " << dim[1]
			<< ", Z: " << dim[2];
		textMapper->SetInput(oss.str().c_str());

		vtkNew<vtkActor2D> textActor;
		textActor->SetMapper(textMapper);
		textActor->SetPosition(20, 20);

		vtkNew<vtkPropCollection> actors;
		actors->AddItem(volume);
		actors->AddItem(textActor);

		return actors;
    }

	void SetClipPlane(vtkVolume* volume, double origin[3], double normal[3]) {
		vtkNew<vtkPlane> plane;
		plane->SetOrigin(origin);
		plane->SetNormal(normal);
		volume->GetMapper()->AddClippingPlane(plane);
	}

	void UnSetClip(vtkVolume* volume) {
		volume->GetMapper()->RemoveAllClippingPlanes();
	}

	vtkSmartPointer<vtkOrientationMarkerWidget> SetUpAxesWidget(vtkRenderWindowInteractor* interactor) {
		vtkNew<vtkNamedColors> colors;
		// axes
		vtkNew<vtkAxesActor> axes;
		axes->SetCylinderRadius(0.03);
		axes->SetShaftTypeToCylinder();
		axes->SetTotalLength(1.5, 1.5, 1.5);

		// widget
		vtkNew<vtkOrientationMarkerWidget> widget;
		double rgba[4]{ 0.0, 0.0, 0.0, 0.0 };
		colors->GetColor("Carrot", rgba);
		widget->SetOutlineColor(rgba[0], rgba[1], rgba[2]);
		widget->SetOrientationMarker(axes);
		widget->SetInteractor(interactor);
		widget->SetViewport(0.0, 0.0, 0.4, 0.4);
		return widget;
	}

	vtkSmartPointer<vtkCubeAxesActor> GetCubeAxesActor(vtkCamera* camera, double bounds[6]) {
		vtkNew<vtkNamedColors> colors;
		vtkColor3d actorColor = colors->GetColor3d("Tomato");
		vtkColor3d axis1Color = colors->GetColor3d("Salmon");
		vtkColor3d axis2Color = colors->GetColor3d("PaleGreen");
		vtkColor3d axis3Color = colors->GetColor3d("LightSkyBlue");

		vtkNew<vtkCubeAxesActor> cubeAxesActor;
		cubeAxesActor->SetUseTextActor3D(1);
		cubeAxesActor->SetBounds(bounds);
		cubeAxesActor->SetCamera(camera);
		cubeAxesActor->GetTitleTextProperty(0)->SetColor(axis1Color.GetData());
		cubeAxesActor->GetTitleTextProperty(0)->SetFontSize(48);
		cubeAxesActor->GetLabelTextProperty(0)->SetColor(axis1Color.GetData());

		cubeAxesActor->GetTitleTextProperty(1)->SetColor(axis2Color.GetData());
		cubeAxesActor->GetLabelTextProperty(1)->SetColor(axis2Color.GetData());

		cubeAxesActor->GetTitleTextProperty(2)->SetColor(axis3Color.GetData());
		cubeAxesActor->GetLabelTextProperty(2)->SetColor(axis3Color.GetData());

		cubeAxesActor->DrawXGridlinesOn();
		cubeAxesActor->DrawYGridlinesOn();
		cubeAxesActor->DrawZGridlinesOn();

		cubeAxesActor->SetGridLineLocation(cubeAxesActor->VTK_GRID_LINES_FURTHEST);

		cubeAxesActor->XAxisMinorTickVisibilityOff();
		cubeAxesActor->YAxisMinorTickVisibilityOff();
		cubeAxesActor->ZAxisMinorTickVisibilityOff();

		cubeAxesActor->SetFlyModeToStaticEdges();

		return cubeAxesActor;
	}
}
