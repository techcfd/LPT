/*
Real-time particle tracking system:
This is the main code for the real-time particle tracking system. 
This creates the streaming pipeline of tasks including:
	1) data aquisition from camera
	2) image processing
	3) object detection in each image
	4) camera correspondence solver
	5) 3D reconstruction
	6) Temporal tracking
	7) Visualization

 copyright: Douglas Barker 2011
*/

#include "core.hpp"
#include "imageproc.hpp"
#include "datagen.hpp"
#include "dataaquisition.hpp"
#include "correspond.hpp"
#include "tracking.hpp"
#include "visualization.hpp"
//--All particle tracking functionality is loaded in namespace lpt::

using namespace std;

int main(int argc, char** argv) {
	
	string input = (argc > 1 ? argv[1] : "../../../data/input/");
	string output = (argc > 2 ? argv[2] : "../../../data/output/");
	string cameras_file = input + "cameras.yaml"; //"../../../data/pivstd/4cams/cameras.yaml"; //
	string camera_pairs_file = input + "camera_pairs.yaml"; //"../../../data/pivstd/4cams/camera_pairs.yaml"; //
	
	lpt::StreamingPipeline pipeline;
	pipeline.setQueueCapacity(1000);

	auto processor = lpt::ImageProcessor::create();
    lpt::ImageProcess::Ptr blur = lpt::GaussianBlur::create(3);
    lpt::ImageProcess::Ptr thresh = lpt::Threshold::create(20);
	processor->addProcess( blur );
	processor->addProcess( thresh );
	
	auto detector = lpt::FindContoursDetector::create();
	
	auto camera_system = lpt::Optitrack::create();

	auto matcher = lpt::PointMatcher::create();
	matcher->params.match_threshold = 2.0;
	matcher->params.match_threshold = 20;

	auto matcher_cuda =  lpt::PointMatcherCUDA::create();
	matcher_cuda->params.match_threshold = 2.0; //pixels
	matcher_cuda->params.match_thresh_level = 20;

	auto tracker = lpt::Tracker::create();
    tracker->setCostCalculator(lpt::CostMinimumAcceleration::create());
	tracker->params.min_radius = 4.0; //mm
	tracker->params.min_radius_level = 4;
	tracker->params.max_radius = 25.0; //mm
	tracker->params.max_radius_level = 25;
    tracker->params.KF_sigma_a = 1E-5;
    tracker->params.KF_sigma_z = 1E-1;

    bool KalmanFilter = false;

	auto visualizer = lpt::Visualizer::create();
	double grid_side_length = 400;	// mm
	double grid_width = 150;		// mm
	int cell_counts[3] = {40, 15, 15};
	visualizer->getVolumeGrid()->setGridOrigin(-145, 33, -105);		// nozzle exit: (X, Y, Z) = (105, -165, -100);
	//visualizer->getVolumeGrid()->setGridOrigin(14, -23, -80);
	visualizer->getVolumeGrid()->setGridCellCounts(cell_counts[0], cell_counts[1], cell_counts[2]);
	//visualizer->getVolumeGrid()->setGridDimensions( cell_counts[0]*grid_side_length/cell_counts[2], grid_side_length, grid_side_length); // mm
	visualizer->getVolumeGrid()->setGridDimensions(grid_side_length, grid_width, grid_width);
	//visualizer->getVolumeGrid()->setGridDimensions(200, 200, 100);
	
	pipeline.setInputDataPath(input);
	pipeline.setOutputDataPath(output);
    pipeline.setKalmanFilter(KalmanFilter);
	pipeline.attachCameraSystem(camera_system);
	pipeline.attachImageProcessor(processor);
	pipeline.attachDetector(detector);
	pipeline.attachMatcher(matcher_cuda);
	pipeline.attachTracker(tracker);
	pipeline.attachVisualizer(visualizer);
	
	bool on = pipeline.initialize();

	if (on){ 
		camera_system->loadCameraParams(cameras_file);
		camera_system->loadCameraPairParams(camera_pairs_file);
		pipeline.run();
	} else 
		cout << "System could not initialize: Shutting down" << endl;
	
	cout << "Finished" << endl;
	return 0;
}

