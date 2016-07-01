#include <Windows.h>
#include <Kinect.h>
#include <chrono>

#include <cmath>
#include <string>
#include <stdio.h>
#include <iostream>
#include <conio.h>

#include<process.h>
#include <direct.h>


#include <opencv2/highgui/highgui.hpp>
#include <opencv2\opencv.hpp>

#include "dirent.h"

#include <iostream>
#include <fstream>
#include <thread>

using namespace cv;
using namespace std;


template<class Interface>
inline void SafeRelease( Interface *& pInterfaceToRelease )
{
	if( pInterfaceToRelease != NULL ){
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

int fetchCurrentObservationNumber() {
	DIR *dirTrain;
	struct dirent *entTrain;
	char fileName[100];
	string filenameStr;
	string str1Train="C:/Users/efe/Desktop/Videos-HOP/Color/.";
	string str2Train="C:/Users/efe/Desktop/Videos-HOP/Color/..";
	int maxElement = -1;

	if ((dirTrain = opendir ("C:/Users/efe/Desktop/Videos-HOP/Color/"))) 
		{
			while ((entTrain = readdir (dirTrain)) != NULL) 
			{
				string onlyFile = entTrain->d_name;
				sprintf (fileName, "C:/Users/efe/Desktop/Videos-HOP/Color/%s", entTrain->d_name);	
				if(str1Train.compare(fileName) == 0 || str2Train.compare(fileName) == 0)
					continue;
					
				//filenameStr = fileName;
				std::size_t position = onlyFile.find(".");  
				filenameStr = onlyFile.substr (0,position);   
				int currentElement = std::stoi(filenameStr);
				if(currentElement > maxElement) {
					maxElement = currentElement;
				}
			}
		closedir (dirTrain);
		} 
		
	else {
			cout<<"Error exists"<<endl;
			perror ("");
		 }
	return maxElement+1;
}

int maximumTimeToBeSpent = 50;
int stop = 0;

void task1() {
	string colorFramesPath = "C:/Users/efe/Desktop/Videos-HOP/Color/";
	string depthFramesPath = "C:/Users/efe/Desktop/Videos-HOP/Depth/";
	string depthFramesNewPath = "C:/Users/efe/Desktop/Videos-HOP/YML-Depth/";

	string fileExtension = ".avi";
	double fps = 20;
	
	vector<Mat> colorFramesVector;
	vector<Mat> depthFramesVector;
	vector<Mat> depthBufferFramesVector;
	//std::vector<CameraSpacePoint> cameraSpacePoints
	std::vector<std::vector<CameraSpacePoint>> cameraSpacePointsForVideo;
	cv::setUseOptimized( true );
	// Sensor
	IKinectSensor* pSensor;
	HRESULT hResult = S_OK;
	HRESULT hResultCoordinateMapper = S_OK;
	hResult = GetDefaultKinectSensor( &pSensor );
	if( FAILED( hResult ) ){
		std::cerr << "Error : GetDefaultKinectSensor" << std::endl;
		return;
	}

	hResult = pSensor->Open();
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::Open()" << std::endl;
		return;
	}
	
	// Source
	IColorFrameSource* pColorSource;
	hResult = pSensor->get_ColorFrameSource( &pColorSource );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_ColorFrameSource()" << std::endl;
		return;
	}

	IDepthFrameSource* pDepthSource;
	hResult = pSensor->get_DepthFrameSource( &pDepthSource );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_DepthFrameSource()" << std::endl;
		return;
	}

	// Reader
	IColorFrameReader* pColorReader;
	hResult = pColorSource->OpenReader( &pColorReader );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IColorFrameSource::OpenReader()" << std::endl;
		return;
	}

	IDepthFrameReader* pDepthReader;
	hResult = pDepthSource->OpenReader( &pDepthReader );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IDepthFrameSource::OpenReader()" << std::endl;
		return;
	}

	// Description
	IFrameDescription* pColorDescription;
	hResult = pColorSource->get_FrameDescription( &pColorDescription );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IColorFrameSource::get_FrameDescription()" << std::endl;
		return;
	}

	// Coordinate Mapper
	ICoordinateMapper* pCoordinateMapper;
	hResultCoordinateMapper = pSensor->get_CoordinateMapper( &pCoordinateMapper );
	if( FAILED( hResultCoordinateMapper ) ){
		std::cerr << "Error : IKinectSensor::get_CoordinateMapper()" << std::endl;
		return;
	}

	IFrameDescription* pDepthDescription;
	hResult = pDepthSource->get_FrameDescription( &pDepthDescription );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IDepthFrameSource::get_FrameDescription()" << std::endl;
		return;
	}

	int colorWidth = 0;
	int colorHeight = 0;
	pColorDescription->get_Width( &colorWidth ); // 1920
	pColorDescription->get_Height( &colorHeight ); // 1080
	unsigned int colorBufferSize = colorWidth * colorHeight * 4 * sizeof( unsigned char );
	
	cv::Mat colorBufferMat( colorHeight, colorWidth, CV_8UC4 );
	cv::Mat colorMat( colorHeight, colorWidth, CV_8UC4 );
	cv::Mat colorMat3(colorHeight, colorWidth, CV_8UC3);
	//cv::namedWindow( "Color" );

	int depthWidth = 0;
	int depthHeight = 0;
	pDepthDescription->get_Width( &depthWidth ); // 512
	pDepthDescription->get_Height( &depthHeight ); // 424
	unsigned int depthBufferSize = depthWidth * depthHeight * sizeof( unsigned short );

	cv::Mat depthBufferMat( depthHeight, depthWidth, CV_16UC1 );
	cv::Mat depthMat( depthHeight, depthWidth, CV_8UC1 );
	//cv::namedWindow( "Depth" );

	//cv::Mat infraredMat( infraredHeight, infraredWidth, CV_8UC1 );
	//cv::namedWindow( "Infrared" );

	cv::Mat coordinateMapperMat( depthHeight, depthWidth, CV_8UC4 );

	unsigned short minDepth, maxDepth;
	pDepthSource->get_DepthMinReliableDistance( &minDepth );
	pDepthSource->get_DepthMaxReliableDistance( &maxDepth );

	IColorFrame* pColorFrame;
	Size colorFrameSize(static_cast<int>(colorWidth), static_cast<int>(colorHeight));
	Size depthFrameSize(static_cast<int>(depthWidth), static_cast<int>(depthHeight));

	string fileNameColor = colorFramesPath + std::to_string(fetchCurrentObservationNumber()) + fileExtension;
	string fileNameDepth = depthFramesPath + std::to_string(fetchCurrentObservationNumber()) + fileExtension;
	std::string newFolderPath = depthFramesNewPath + std::to_string(fetchCurrentObservationNumber());
	cout<<"Color:"<<fileNameColor<<endl<<"Depth: "<<fileNameDepth<<endl;
	
	VideoWriter colorFrameVideoWriter(fileNameColor, CV_FOURCC('P','I','M','1'), fps, colorFrameSize, true); //initialize the VideoWriter object 
	//VideoWriter depthFrameVideoWriter(fileNameDepth, CV_FOURCC('P','I','M','1'), fps, depthFrameSize, false); //initialize the VideoWriter object  
	if ( !colorFrameVideoWriter.isOpened() ) //if not initialize the VideoWriter successfully, exit the program
    {
       cout << "ERROR: Failed to write the video" << endl;
       return;
	}
	/*
	if ( !depthFrameVideoWriter.isOpened() ) //if not initialize the VideoWriter successfully, exit the program
    {
       cout << "ERROR: Failed to write the video" << endl;
       return;
	}
	*/
	int depthFrameSuccessFlag = 0, colorFrameSuccessFlag = 0;
	char c;
	//Mat currentColorFrame, currentDepthFrame;

	cv::Mat currentColorFrame(colorHeight,colorWidth, CV_8UC3);//cv::Mat colorMat3(colorHeight, colorWidth, CV_8UC3);
	cv::Mat currentDepthFrame(depthHeight, depthWidth,CV_8UC1);//cv::Mat depthMat( depthHeight, depthWidth, CV_8UC1 );
	cv::Mat currentDepthBufferFrame(depthHeight, depthWidth, CV_16UC1);
	

	//std::vector<CameraSpacePoint> cameraSpacePoints( colorHeight*colorWidth );

	while(stop != 1){
		depthFrameSuccessFlag = 0;
		colorFrameSuccessFlag = 0;
		auto begin = chrono::high_resolution_clock::now();
		// Color Frame
		pColorFrame = nullptr;
		hResult = pColorReader->AcquireLatestFrame( &pColorFrame );
		if( SUCCEEDED( hResult ) ){
			colorFrameSuccessFlag = 1;
			hResult = pColorFrame->CopyConvertedFrameDataToArray( colorBufferSize, reinterpret_cast<BYTE*>( colorBufferMat.data ), ColorImageFormat::ColorImageFormat_Bgra );
			if( SUCCEEDED( hResult ) ){
				cv::resize( colorBufferMat, colorMat, cv::Size(), 1, 1 );
			}
		}
		cvtColor( colorMat, colorMat3, CV_BGRA2BGR );

		// Depth Frame
		IDepthFrame* pDepthFrame = nullptr;
		HRESULT hResultDepth;
		hResult = pDepthReader->AcquireLatestFrame( &pDepthFrame );
		hResultDepth = hResult;
		if( SUCCEEDED( hResult ) ){
			depthFrameSuccessFlag = 1;
			hResult = pDepthFrame->AccessUnderlyingBuffer( &depthBufferSize, reinterpret_cast<UINT16**>( &depthBufferMat.data ) );
			if( SUCCEEDED( hResult ) ){
				depthBufferMat.convertTo( depthMat, CV_8U, -255.0f / 8000.0f, 255.0f );
				depthBufferMat.copyTo(currentDepthBufferFrame);
			}
		}

		SafeRelease( pColorFrame );
		SafeRelease( pDepthFrame );

		//cv::imshow( "Color", colorMat3 );
		//colorFrameVideoWriter.write(colorMat3); //writer the frame into the file
		//depthFrameVideoWriter.write(depthMat);
		//cv::imshow( "Depth", depthMat );
		
		
		colorMat3.copyTo(currentColorFrame);
		//depthMat.copyTo(currentDepthFrame);
		//depthBufferMat.copyTo(currentDepthBufferFrame);

		if(depthFrameSuccessFlag == 1 && colorFrameSuccessFlag == 1) {
			colorFramesVector.push_back(currentColorFrame);
			//depthFramesVector.push_back(currentDepthFrame);
			depthBufferFramesVector.push_back(currentDepthBufferFrame);
		}

		currentColorFrame.release();
		//currentDepthFrame.release();
		currentDepthBufferFrame.release();

		auto end = chrono::high_resolution_clock::now();    
		auto dur = end - begin;
		float ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		//cout<<"Time spent on frame: "<<ms<<" milliseconds."<<endl;

		int timeRemained = maximumTimeToBeSpent - ms;
		cout<<"Time remained: "<<timeRemained<<endl;
		
		if(timeRemained > 0) {
			Sleep(timeRemained);
		}
		
		auto last = chrono::high_resolution_clock::now();
		auto totalDuration = last - begin;
		cout<<"Spent: "<<std::chrono::duration_cast<std::chrono::milliseconds>(totalDuration).count()<<endl;
		//cout<<"Vector size: "<<colorFramesVector.size()<<endl;
		
		cout<<"Stop: "<<stop<<endl;
	} // end of main while loop

	if(colorFramesVector.size() == depthBufferFramesVector.size()) {
		cout<<"Color frame and depth buffer sizes are equal"<<endl;
	}

	int counter = 0;
	for(int i=0; i<colorFramesVector.size(); i++) {
		//cout<<"Counter: "<<i<<endl;
		colorFrameVideoWriter.write(colorFramesVector.at(i));
		//depthFrameVideoWriter.write(depthFramesVector.at(i));
	}
	
	mkdir(newFolderPath.c_str());

	//CreateDirectory(fullPath, NULL);
	// Write the depthBuffers into yml files
	for(int j=0; j<depthBufferFramesVector.size(); j++) {
		string fileNameDepthBuffer = newFolderPath + "/" + std::to_string(j) + ".yml";

		cv::FileStorage fs(fileNameDepthBuffer, cv::FileStorage::WRITE);
		fs << "yourMat" << depthBufferFramesVector.at(j);
	}

	SafeRelease(pColorSource);
	SafeRelease(pDepthSource);
	SafeRelease(pColorReader);
	SafeRelease(pDepthReader);
	SafeRelease(pColorDescription);
	SafeRelease(pDepthDescription);
	SafeRelease(pCoordinateMapper);
	if(pSensor){
		pSensor->Close();
	}
	SafeRelease(pSensor);
	cv::destroyAllWindows();

}

void task2() {
	cout<<"Second task"<<endl;	
	//char c = cin.get();
	char c;
	c=getch();
    if (c==27)
		stop = 1;
};

int main() {	
	
	/*
	cv::namedWindow( "test" );
	for(int i=0; i<colorFramesVector.size(); i++) {
		Mat currentFrame = colorFramesVector.at(i).clone();
		colorFrameVideoWriter.write(currentFrame);
		cv::imshow( "test", currentFrame );
	}
	*/
	thread t1(task1);
	thread t2(task2);
	t1.join();
	t2.join();



	system("PAUSE");
	return 0;
}