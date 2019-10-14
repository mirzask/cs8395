#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"

#include "itkAffineTransform.h"
//#include "itkImageRegistrationMethod.h"
#include "itkMultiResolutionImageRegistrationMethod.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkCenteredTransformInitializer.h"

const unsigned int nDims = 3 ;
typedef itk::Image< unsigned int, nDims > ImageType;

class OptimizationObserver : public itk::Command
{
public:
  typedef OptimizationObserver Self ;
  typedef itk::Command Superclass ;
  typedef itk::SmartPointer < Self > Pointer ;
  itkNewMacro ( Self ) ;

  typedef itk::RegularStepGradientDescentOptimizer OptimizerType ; 
  typedef const OptimizerType * OptimizerPointerType ;

  void Execute (itk::Object *caller, const itk::EventObject &event)
  {
    Execute ( (const itk::Object *) caller, event ) ;
  }

  void Execute (const itk::Object *caller, const itk::EventObject &event)
  {
    OptimizerPointerType optimizer = dynamic_cast < OptimizerPointerType > ( caller ) ;
std::cout << "OptimizerObserver" << optimizer->GetCurrentIteration() << " " << optimizer->GetValue() << std::endl ;
  }

protected:
  OptimizationObserver() {} ;

};

class RegistrationObserver : public itk::Command
{
public:
  typedef RegistrationObserver Self ;
  typedef itk::Command Superclass ;
  typedef itk::SmartPointer < Self > Pointer ;
  itkNewMacro ( Self ) ;

  typedef itk::RegularStepGradientDescentOptimizer OptimizerType ; 
  typedef OptimizerType * OptimizerPointerType ;
  typedef itk::MultiResolutionImageRegistrationMethod < ImageType, ImageType > RegistrationWrapperType ;  // fixed, moving
  typedef RegistrationWrapperType * RegistrationPointerType ;

  void Execute (itk::Object *caller, const itk::EventObject &event)
  {
    RegistrationPointerType registrationWrapper = dynamic_cast < RegistrationPointerType > ( caller ) ;
    OptimizerPointerType optimizer = dynamic_cast < OptimizerPointerType > ( registrationWrapper->GetModifiableOptimizer() ) ;

    std::cout << "RegObserver" << optimizer->GetCurrentIteration() << std::endl ;

    int level = registrationWrapper->GetCurrentLevel() ;
    if ( level == 0 ) 
       optimizer->SetMaximumStepLength ( 0.00125 ) ; 
    else if ( level == 1 ) 
       optimizer->SetMaximumStepLength ( 0.125 / 2 ) ; 
    else
       optimizer->SetMaximumStepLength ( 0.25 / 2 ) ; 

  }

  void Execute (const itk::Object *caller, const itk::EventObject &event)
  {
  }

protected:
  RegistrationObserver() {} ;

};

int main(int argc, char * argv[])
{
  // Verify command line arguments
  if( argc < 4 )
    {
    std::cerr << "Usage: " << std::endl;
    std::cerr << argv[0] << " inputMovingImage inputFixedImage outputRegisteredMovingImage" << std::endl; 
    return -1 ;
    }
 
  // Setup types
  // Intensities too high - convert to greyscale
  typedef itk::RGBToLuminanceImageFilter < ImageType, ImageType > GreyF ilterType;
  GreyFilterType::Pointer greyFilter = GreyFilterType::New() ;
  

  // Create and setup a reader for moving image
  typedef itk::ImageFileReader< ImageType >  readerType;
  readerType::Pointer reader = readerType::New(); 
  reader->SetFileName( argv[1] );
  reader->Update();
  ImageType::Pointer movingImage = reader->GetOutput() ;

  // Convert moving image to greyscale


  // Same for the fixed image
  readerType::Pointer reader2 = readerType::New() ;
  reader2->SetFileName ( argv[2] ) ;
  reader2->Update() ;
  ImageType::Pointer fixedImage = reader2->GetOutput() ;

  // Register images
  // Set up typedefs
  typedef itk::AffineTransform < double, 3 > TransformType ;
//typedef itk::ImageRegistrationMethod < ImageType, ImageType > RegistrationWrapperType ;  // fixed, moving
  typedef itk::MultiResolutionImageRegistrationMethod < ImageType, ImageType > RegistrationWrapperType ;  // fixed, moving
  typedef itk::MeanSquaresImageToImageMetric < ImageType, ImageType > MetricType ; // fixed, moving
  typedef itk::LinearInterpolateImageFunction < ImageType, double > InterpolatorType ; // moving, coord rep
  typedef itk::RegularStepGradientDescentOptimizer OptimizerType ;

  // Declare the variables
  RegistrationWrapperType::Pointer registrationWrapper = RegistrationWrapperType::New() ;

  TransformType::Pointer transform = TransformType::New() ;
  MetricType::Pointer metric = MetricType::New() ;
  InterpolatorType::Pointer interpolator = InterpolatorType::New() ;
  OptimizerType::Pointer optimizer = OptimizerType::New() ;

// Optimization observer
  OptimizationObserver::Pointer observer = OptimizationObserver::New() ;
  optimizer->AddObserver ( itk::IterationEvent(), observer ) ;

// Registration observer
RegistrationObserver::Pointer regObserver = RegistrationObserver::New() ;
registrationWrapper->AddObserver ( itk::IterationEvent(), regObserver ) ;

  // Connect the pipeline

  std::cout << optimizer->GetMinimumStepLength() << " " << optimizer->GetMaximumStepLength() << " " << optimizer->GetNumberOfIterations() << " " << optimizer->GetGradientMagnitudeTolerance() << std::endl ;

  transform->SetIdentity() ;

  registrationWrapper->SetMovingImage ( movingImage ) ;
  registrationWrapper->SetFixedImage ( fixedImage ) ;
  registrationWrapper->SetTransform ( transform ) ;
  registrationWrapper->SetMetric ( metric ) ;
  registrationWrapper->SetInterpolator ( interpolator ) ;
  registrationWrapper->SetOptimizer ( optimizer ) ;

  // Better initialization - hopefully
  typedef itk::CenteredTransformInitializer < TransformType, ImageType, ImageType > TransformInitializerType ;
  TransformInitializerType::Pointer initializer = TransformInitializerType::New() ; 
  initializer->SetTransform( transform ) ;
  initializer->SetFixedImage( fixedImage ) ;
  initializer->SetMovingImage( movingImage ) ;
  initializer->MomentsOn() ;
  initializer->InitializeTransform() ; 

  registrationWrapper->SetInitialTransformParameters ( transform->GetParameters() ) ;
  registrationWrapper->SetFixedImageRegion ( fixedImage->GetBufferedRegion() ) ;

  // Set Scales + Optimizer Stuff
  // double translationScale = 1.0 / 1000.0;

 // typedef OptimizerType::ScalesType OptimizerScalesType ;
//  OptimizerScalesType optimizerScales(transform->GetNumberOfParameters() ) ;
//  optimizerScales[0] = 1.0;
//  optimizerScales[1] = 1.0 ;
//  optimizerScales[3] = 1.0 ;
//  optimizerScales[4] =  translationScale;
//  optimizerScales[5] =  translationScale;
//
//  optimizer->SetScales (optimizerScales ) ;
  optimizer->MinimizeOn() ;

  registrationWrapper->SetNumberOfLevels ( 3 ) ;
  // Run the registration
  try 
    {  
      registrationWrapper->Update() ;
    }
  catch ( itk::ExceptionObject & excp ) 
    {
      std::cerr << "Error in registration" << std::endl;  
      std::cerr << excp << std::endl; 
    }

  // Update the transform 

  transform->SetParameters ( registrationWrapper->GetLastTransformParameters() ) ; 

  // Apply the transform
  typedef itk::ResampleImageFilter < ImageType, ImageType > ResampleFilterType ;
  ResampleFilterType::Pointer filter = ResampleFilterType::New() ;
  filter->SetInput ( movingImage ) ;

  //filter->SetTransform ( dynamic_cast < const TransformType * > ( registrationWrapper->GetOutput() ) ) ;
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
