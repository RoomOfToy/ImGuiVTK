#pragma once

#include <vtkNew.h>
#include <vtkImageData.h>
#include <vtkImageReader2.h>
#include <vtkImageReader2Factory.h>
#include <vtkNamedColors.h>
#include <vtkSmartPointer.h>

vtkSmartPointer<vtkImageData> ReadImageData(const char* fileName)
{
    vtkNew<vtkImageReader2Factory> readerFactory;
    vtkSmartPointer<vtkImageReader2> imageReader;
    imageReader.TakeReference(readerFactory->CreateImageReader2(fileName));
    imageReader->SetFileName(fileName);
    imageReader->Update();
    vtkSmartPointer<vtkImageData> imageData;
    imageData = imageReader->GetOutput();
    return imageData;
}
