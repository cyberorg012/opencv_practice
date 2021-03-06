//Background Subtraction (Rodney modified)
//#
//# @file bg_sub.cpp
//# @brief Background subtraction tutorial sample code
//# @author
//#

//opencv
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/videoio/videoio.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
//C
#include <stdio.h>
//C++
#include <iostream>
#include <sstream>

using namespace cv;
using namespace std;

// Global variables
Mat frame; //current frame
Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method

Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor
int keyboard; //input from keyboard

string strImageToSave; //rd_add;
Mat CannyOutput;
Mat drawing;
vector<vector<Point> > contours;
vector<Vec4i> hierarchy;

int thresh = 50; // 50 ,100
int max_thresh = 255;
RNG rng(12345);

//erosion & dilation
Mat beforNR, AfterNR;
Mat erosion_dst, dilation_dst;
Mat threshold_output;

int erosion_elem = 0;
int erosion_size = 1;	 //rd_test;
int dilation_elem = 0;
int dilation_size = 1;	 //rd_test;
int const max_elem = 2;
int const max_kernel_size = 21;

/** Function Headers */
void help();
void processVideo(char* videoFilename);
void processImages(char* firstFrameFilename);
void Erosion(int, void*);
void Dilation(int, void*);
double dM01;
double dM10;
double dArea;
int posX=0;
int posY=0;
int iLastX = -1;
int iLastY = -1;
Scalar colorWhite;
Scalar colorRed;
Scalar colorBlue;

void help()
{
	cout
		<< "--------------------------------------------------------------------------" << endl
		<< "This program shows how to use background subtraction methods provided by " << endl
		<< " OpenCV. You can process both videos (-vid) and images (-img)." << endl
		<< endl
		<< "Usage:" << endl
		<< "./bs {-vid <video filename>|-img <image filename>}" << endl
		<< "for example: ./bs -vid video.avi" << endl
		<< "or: ./bs -img /data/images/1.png" << endl
		<< "--------------------------------------------------------------------------" << endl
		<< endl;
		
		//rodney_test_command:
		//-vid D:/tmp/rd_test_VlcVideo/object_test.mp4
		//-vid D:/tmp/OpenCVVideo/people_768x576.avi
}

/**
* @function main
*/
int main(int argc, char* argv[])
{
	//print help information
	help();

	//check for the input parameter correctness
	if (argc != 3) {
		cerr << "Incorret input list" << endl;
		cerr << "exiting..." << endl;
		return EXIT_FAILURE;
	}

	//create GUI windows
	namedWindow("Frame");
	namedWindow("FG Mask MOG 2");
	//namedWindow("Capture Foreground");
	//
	namedWindow("1step_Erosion", WINDOW_AUTOSIZE);
	namedWindow("2step_Dilation", WINDOW_AUTOSIZE);

	colorWhite = Scalar(255, 255, 255);
	colorRed = Scalar(0, 0, 255);
	colorBlue = Scalar(255, 0, 0);

	//create Background Subtractor objects
	pMOG2 = createBackgroundSubtractorMOG2(); //MOG2 approach

	if (strcmp(argv[1], "-vid") == 0) {
		//input data coming from a video
		processVideo(argv[2]);
	}
	else if (strcmp(argv[1], "-img") == 0) {
		//input data coming from a sequence of images
		processImages(argv[2]);
	}
	else {
		//error in reading input parameters
		cerr << "Please, check the input parameters." << endl;
		cerr << "Exiting..." << endl;
		return EXIT_FAILURE;
	}

	/// Create Erosion Trackbar
	createTrackbar("Element:\n 0: Rect \n 1: Cross \n 2: Ellipse", "1step_Erosion",
		&erosion_elem, max_elem,
		Erosion);

	createTrackbar("Kernel size:\n 2n +1", "1step_Erosion",
		&erosion_size, max_kernel_size,
		Erosion);

	/// Create Dilation Trackbar
	createTrackbar("Element:\n 0: Rect \n 1: Cross \n 2: Ellipse", "2step_Dilation",
		&dilation_elem, max_elem,
		Dilation);

	createTrackbar("Kernel size:\n 2n +1", "2step_Dilation",
		&dilation_size, max_kernel_size,
		Dilation);	
	
	//destroy GUI windows
	destroyAllWindows();
	return EXIT_SUCCESS;
}

//#
//# @function Erosion
//#
void Erosion(int, void*)
{
	int erosion_type = 0;
	if (erosion_elem == 0){ erosion_type = MORPH_RECT; }
	else if (erosion_elem == 1){ erosion_type = MORPH_CROSS; }
	else if (erosion_elem == 2) { erosion_type = MORPH_ELLIPSE; }

	Mat element = getStructuringElement(erosion_type,
		Size(2 * erosion_size + 1, 2 * erosion_size + 1),
		Point(erosion_size, erosion_size));
	/// Apply the erosion operation
	erode(beforNR, erosion_dst, element);
	imshow("1step_Erosion", erosion_dst);
}

//#
//# @function Dilation
//#
void Dilation(int, void*)
{
	int dilation_type = 0;
	if (dilation_elem == 0){ dilation_type = MORPH_RECT; }
	else if (dilation_elem == 1){ dilation_type = MORPH_CROSS; }
	else if (dilation_elem == 2) { dilation_type = MORPH_ELLIPSE; }

	Mat element = getStructuringElement(dilation_type,
		Size(2 * dilation_size + 1, 2 * dilation_size + 1),
		Point(dilation_size, dilation_size));
	/// Apply the dilation operation
	//dilate(beforNR, dilation_dst, element);
	erode(erosion_dst, dilation_dst, element);
	imshow("2step_Dilation", dilation_dst);
}

/**
* @function processVideo
*/
void processVideo(char* videoFilename) {
	//create the capture object
	VideoCapture capture(videoFilename);
	if (!capture.isOpened()){
		//error in opening the video input
		cerr << "Unable to open video file: " << videoFilename << endl;
		exit(EXIT_FAILURE);
	}
	//read input data. ESC or 'q' for quitting
	while ((char)keyboard != 'q' && (char)keyboard != 27){
		//read the current frame
		if (!capture.read(frame)) {
			cerr << "Unable to read next frame." << endl;
			cerr << "Exiting..." << endl;
			exit(EXIT_FAILURE);
		}
		//update the background model
		pMOG2->apply(frame, fgMaskMOG2);
		//get the frame number and write it on the current frame
		stringstream ss;
		rectangle(frame, cv::Point(10, 2), cv::Point(100, 20),
			cv::Scalar(255, 255, 255), -1);
		ss << capture.get(CAP_PROP_POS_FRAMES);
		string frameNumberString = ss.str();
		putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
			FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

		beforNR = Mat::zeros(frame.size(), CV_8UC3);
		beforNR = fgMaskMOG2;
		
		Erosion(0, 0);
		Dilation(0, 0);

		AfterNR = dilation_dst;

		/// Detect edges using Threshold
		//threshold(AfterNR, threshold_output, thresh, 255, THRESH_BINARY);

		/// Detect edges using canny
		//Canny(fgMaskMOG2, CannyOutput, thresh, thresh*3, 3);
		//Canny(threshold_output, CannyOutput, thresh, thresh * 3, 3);
		Canny(AfterNR, CannyOutput, 150, 255 * 3, 3);

		/// Find contours
		findContours(CannyOutput, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));

		
		/// Approximate contours to polygons + get bounding rects and circles
		vector<vector<Point> > contours_poly(contours.size());
		vector<Rect> boundRect(contours.size());
		vector<Point2f>center(contours.size());
		vector<float>radius(contours.size());

		for (size_t i = 0; i < contours.size(); i++)
		{
			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
			minEnclosingCircle(contours_poly[i], center[i], radius[i]);
		}

		/// Draw polygonal contour + bonding rects + circles
		Mat drawing = Mat::zeros(CannyOutput.size(), CV_8UC3);
		Mat masking = Mat::zeros(CannyOutput.size(), CV_8UC3);
		Mat BlendImage = Mat::zeros(frame.size(), CV_8UC3);

		for (size_t i = 0; i< contours.size(); i++)
		{
			//Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			
			//drawContours(drawing, contours_poly, (int)i, color, 1, 8, vector<Vec4i>(), 0, Point());
			rectangle(drawing, boundRect[i].tl(), boundRect[i].br(), colorRed, 2, 8, 0);
			//circle(drawing, center[i], (int)radius[i], color, 2, 8, 0);

			//drawContours(masking, contours_poly, (int)i, colorwhite, 1, 8, vector<Vec4i>(), 0, Point());
			rectangle(masking, boundRect[i].tl(), boundRect[i].br(), colorWhite, 2, 8, 0);
		}		
		
		
		/// Get the moments		
		vector<Moments> mu(contours.size());
		for (int i = 0; i < contours.size(); i++)
		{
			mu[i] = moments(contours[i], false);

			///  Get the mass centers:
			dM01 = mu[i].m01;
			dM10 = mu[i].m10;
			dArea = mu[i].m00;

			/*
			// if the area <= 10000, I consider that the there are no object in the image and it's because of the noise, the area is not zero 
			if (dArea > 100)
			//if (dArea > 10000)
			{
				//calculate the position of the ball
				posX = dM10 / dArea;
				posY = dM01 / dArea;

				if (iLastX >= 0 && iLastY >= 0 && posX >= 0 && posY >= 0)
				{
					//Draw a red line from the previous point to the current point
					line(drawing, Point(posX, posY), Point(iLastX, iLastY), Scalar(0, 0, 255), 2);
				}

				iLastX = posX;
				iLastY = posY;
			}
			*/

			vector<Point2f> mc(contours.size());
			for (int i = 0; i < contours.size(); i++)
			{
				mc[i] = Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);

				circle(drawing, mc[i], 4, colorBlue, -1, 8, 0);
				circle(masking, mc[i], 4, colorWhite, -1, 8, 0);
			}
		}
		/// Draw contours
		/*
		drawing = Mat::zeros(CannyOutput.size(), CV_8UC3);		
		for (size_t i = 0; i< contours.size(); i++)
		{ 
			Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			drawContours(drawing, contours, (int)i, color, 2, 8, hierarchy, 0, Point()); 
			circle(drawing, mc[i], 4, color, -1, 8, 0);
		}
		*/

		namedWindow("Contours", CV_WINDOW_AUTOSIZE);
		imshow("Contours", drawing);
		
		BlendImage = frame - masking + drawing;
		//show the current frame and the fg masks
		//imshow("Frame", frame);
		imshow("Frame", BlendImage);
		imshow("FG Mask MOG 2", fgMaskMOG2);

		/// Calculate the area with the moments 00 and compare with the result of the OpenCV function
		/*
		printf("\t Info: Area and Contour Length \n");
		for (int i = 0; i< contours.size(); i++)
			printf(" * Contour[%d] - Area (M_00) = %.2f - Area OpenCV: %.2f - Length: %.2f \n",
			i, mu[i].m00, contourArea(contours[i]), arcLength(contours[i], true));
		*/

		//get the input from the keyboard
		//keyboard = waitKey(30);
		keyboard = waitKey(100);
		if (keyboard == 27) 
			break;
		if (keyboard == 'f'){
			//cvCopyImage(fgMaskMOG2, background);
			imshow("Capture Foreground", fgMaskMOG2);

			//rodney_test_add
			strImageToSave = "D:/tmp/output_MOG_" + frameNumberString + ".png";
			bool saved = imwrite(strImageToSave, fgMaskMOG2);
			if (!saved) {
				cerr << "Unable to save " << strImageToSave << endl;
			}
			cerr << "save... " << strImageToSave << endl;
		}
	}
	//delete capture object
	capture.release();
}

/**
* @function processImages
*/
void processImages(char* fistFrameFilename) {
	//read the first file of the sequence
	frame = imread(fistFrameFilename);
	if (frame.empty()){
		//error in opening the first image
		cerr << "Unable to open first image frame: " << fistFrameFilename << endl;
		exit(EXIT_FAILURE);
	}
	//current image filename
	string fn(fistFrameFilename);
	//read input data. ESC or 'q' for quitting
	while ((char)keyboard != 'q' && (char)keyboard != 27){
		//update the background model
		pMOG2->apply(frame, fgMaskMOG2);
		//get the frame number and write it on the current frame
		size_t index = fn.find_last_of("/");
		if (index == string::npos) {
			index = fn.find_last_of("\\");
		}
		size_t index2 = fn.find_last_of(".");
		string prefix = fn.substr(0, index + 1);
		string suffix = fn.substr(index2);
		string frameNumberString = fn.substr(index + 1, index2 - index - 1);
		istringstream iss(frameNumberString);
		int frameNumber = 0;
		iss >> frameNumber;
		rectangle(frame, cv::Point(10, 2), cv::Point(100, 20),
			cv::Scalar(255, 255, 255), -1);
		putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
			FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
		//show the current frame and the fg masks
		imshow("Frame", frame);
		imshow("FG Mask MOG 2", fgMaskMOG2);
		//get the input from the keyboard
		keyboard = waitKey(30);
		//search for the next image in the sequence
		ostringstream oss;
		oss << (frameNumber + 1);
		string nextFrameNumberString = oss.str();
		string nextFrameFilename = prefix + nextFrameNumberString + suffix;
		//read the next frame
		frame = imread(nextFrameFilename);
		if (frame.empty()){
			//error in opening the next image in the sequence
			cerr << "Unable to open image frame: " << nextFrameFilename << endl;
			exit(EXIT_FAILURE);
		}
		//update the path of the current frame
		fn.assign(nextFrameFilename);
	}
}