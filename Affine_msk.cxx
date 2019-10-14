#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"

#include "itkImageRegistrationMethod.h"
#include "itkAffineTransform.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkCenteredTransformInitializer.h"

// Setup types 
const unsigned int nDims = 3 ; 
typedef itk::Image < int, nDims > ImageType ; 

int main(int argc, char * argv[])
 {
   // Verify command line arguments
   if( argc < 4 ) 
     {   
     std::cerr << "Usage: " << std::endl;
     std::cerr << argv[0] << " inputMovingImage inputFixedImage   outputRegisteredMovingImage" << std::endl; 
     return -1 ;
     }   
     
// Setup types
 
   // Create and setup a reader for moving image
   typedef itk::ImageFileReader < ImageType >  readerType;
   readerType::Pointer reader = readerType::New();
   reader->SetFileName( argv[1] );
   reader->Update();
   ImageType::Pointer movingImage = reader->GetOutput() ;
 
   // Same for the fixed image
   readerType::Pointer reader2 = readerType::New() ;
   reader2->SetFileName ( argv[2] ) ;
   reader2->Update() ;
   ImageType::Pointer fixedImage = reader2->GetOutput() ;


  // Setup for Registration - Metric, Optimizer, Interpolator, Registration Method
  typedef itk::ImageRegistrationMethod < ImageType, ImageType >  RegistrationWrapperType ;
  typedef itk::AffineTransform < double, 3 > TransformType ;
  typedef itk::MeanSquaresImageToImageMetric < ImageType, ImageType > MetricType ; // fixed, moving
  typedef itk::LinearInterpolateImageFunction < ImageType, double > InterpolatorType ; // moving, coord rep
  typedef itk::RegularStepGradientDescentOptimizer OptimizerType ;

  // Declare variables - set up all those Pointers
  RegistrationWrapperType::Pointer registration = RegistrationWrapperType::New() ;
  TransformType::Pointer transform = TransformType::New() ;
  MetricType::Pointer metric = MetricType::New() ;
  InterpolatorType::Pointer interpolator = InterpolatorType::New() ;
  OptimizerType::Pointer optimizer = OptimizerType::New() ;

  // Setup the Registration Process
  transform->SetIdentity() ;
 
  registration->SetMovingImage ( movingImage ) ;
  registration->SetFixedImage ( fixedImage ) ;
  registration->SetTransform ( transform ) ;
  registration->SetMetric ( metric ) ;
  registration->SetInterpolator ( interpolator ) ;
  registration->SetOptimizer ( optimizer ) ;
 
  registration->SetFixedImageRegion ( fixedImage->GetLargestPossibleRegion() ) ;

    // CenteredTransform - hoping this helps
  typedef itk::CenteredTransformInitializer < TransformType, ImageType, ImageType > TransformInitializerType ;
  TransformInitializerType::Pointer initializer = TransformInitializerType::New() ; 

  initializer->SetTransform( transform ) ;
  initializer->SetFixedImage (fixedImage ) ;
  initializer->SetMovingImage( movingImage ) ;
  initializer->GeometryOn() ; // uses center of mass vs GeometryOn()
  initializer->InitializeTransform() ;

  // Have registration use this initialized transform

  registration->SetInitialTransformParameters ( transform->GetParameters() ) ;

  // Run the registration
  try 
    {  
      registration->Update() ;
    }
  catch ( itk::ExceptionObject & excp ) 
    {
      std::cerr << "Error in registration" << std::endl;  
      std::cerr << excp << std::endl; 
    }

  // Update the transform 

  transform->SetParameters ( registration->GetLastTransformParameters() ) ; 

  // Apply the transform
  typedef itk::ResampleImageFilter < ImageType, ImageType > ResampleFilterType ;
  ResampleFilterType::Pointer filter = ResampleFilterType::New() ;
  filter->SetInput ( movingImage ) ;

  filter->SetTransform ( transform ) ;

  filter->SetSize ( movingImage->GetLargestPossibleRegion().GetSize() ) ;
  filter->SetReferenceImage ( fixedImage ) ;
  filter->UseReferenceImageOn() ;
  filter->Update() ;

  // write out the result to argv[3]
  typedef itk::ImageFileWriter < ImageType > writerType ;
  writerType::Pointer writer = writerType::New() ;
  writer->SetInput ( filter->GetOutput() ) ; 
  writer->SetFileName ( argv[3] ) ;
  writer->Update() ;

  // Done.
  return 0 ;
 }

