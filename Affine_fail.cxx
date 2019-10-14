#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkResampleImageFilter.h"

#include "itkAffineTransform.h"
#include "itkImageRegistrationMethod.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkRegularStepGradientDescentOptimizer.h"

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
  const unsigned int nDims = 3 ;
  typedef itk::Image< unsigned int, nDims > ImageType;

  // Create and setup a reader for moving image
  typedef itk::ImageFileReader< ImageType >  readerType;
  readerType::Pointer reader = readerType::New(); 
  reader->SetFileName( argv[1] );
  reader->Update();
  ImageType::Pointer movingImage = reader->GetOutput() ;

  // Same for the fixed image
  readerType::Pointer reader2 = readerType::New() ;
  reader2->SetFileName ( argv[2] ) ;
  reader2->Update() ;
  ImageType::Pointer fixedImage = reader2->GetOutput() ;

  // Register images
  // Set up typedefs
  typedef itk::AffineTransform < double, 3 > TransformType ;
  typedef itk::ImageRegistrationMethod < ImageType, ImageType > RegistrationWrapperType ;  // fixed, moving
  typedef itk::MeanSquaresImageToImageMetric < ImageType, ImageType > MetricType ; // fixed, moving
  typedef itk::LinearInterpolateImageFunction < ImageType, double > InterpolatorType ; // moving, coord rep
  typedef itk::RegularStepGradientDescentOptimizer OptimizerType ;

  // Declare the variables
  RegistrationWrapperType::Pointer registrationWrapper = RegistrationWrapperType::New() ;

  TransformType::Pointer transform = TransformType::New() ;
  MetricType::Pointer metric = MetricType::New() ;
  InterpolatorType::Pointer interpolator = InterpolatorType::New() ;
  OptimizerType::Pointer optimizer = OptimizerType::New() ;

  // Connect the pipeline

  transform->SetIdentity() ;

  registrationWrapper->SetMovingImage ( movingImage ) ;
  registrationWrapper->SetFixedImage ( fixedImage ) ;
  registrationWrapper->SetTransform ( transform ) ;
  registrationWrapper->SetMetric ( metric ) ;
  registrationWrapper->SetInterpolator ( interpolator ) ;
  registrationWrapper->SetOptimizer ( optimizer ) ;

  registrationWrapper->SetInitialTransformParameters ( transform->GetParameters() ) ;
  registrationWrapper->SetFixedImageRegion ( fixedImage->GetLargestPossibleRegion() ) ;

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
