
// Standard includes
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>


#include "polyfit.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


//#include <stdio.h>
//using namespace cv;


// Hard code pespective tranform matrix and inverse perspective transform matrix
// Pixel coordinates to inches and vice versa
float data_M[3][3] = {{ -2.72856341e-01,   8.56199838e-03,   1.81547469e+02},
							{-3.46017550e-04,   5.23860756e-02,  -2.01215341e+02},
							{2.25802375e-04,  -5.62182736e-03,   1.00000000e+00} };
float data_Minv[3][3] = { 7.54893714e+01,   7.20171427e+01,   7.86049637e+02 ,
								3.15507435e+00,   2.19615255e+01,   3.84620008e+03 , 
								6.91603907e-04,   1.07202263e-01,   1.00000000e+00 };

using namespace std;
cv::Point2f xformPoint(float pix_row, float pix_col, cv::Mat M);// transforms a point by perspective xform M
cv::Mat markTrackLine(cv::Mat image, std::vector<float> coeffs, cv::Mat);// plots a line on an image
std::vector<float> getRightRail(cv::Mat dpt, cv::Mat M);// gets line coefficients fo right rail
std::vector<float> getLeftRail(cv::Mat dpt, cv::Mat M);// gets line coefficients fo left rail

int main(int argc, char **argv) {


	cv::Mat img;
	cv::Mat dpt;
	std::string depthPaths = "D:\\TrackDetection\\Images_to_4.5m\\image_depth_*.jpg";
	std::string imgPaths = "D:\\TrackDetection\\Images_to_4.5m\\image_left_*.jpg";
	std::vector<cv::String> depthNames;
	std::vector<cv::String> imgNames;
	cv::glob(depthPaths, depthNames);
	cv::glob(imgPaths, imgNames);

	



	cv::Mat M(3, 3, CV_32FC1,&data_M);
	cv::Mat Minv(3, 3, CV_32FC1,&data_Minv);

	//std::cout << M;
	//std::cout << "Minv : " << Minv;

	for (size_t i = 0; i < depthNames.size(); i++) {
		//std::cout << "Depth File Name : "<< depthNames[i]<<" \t Image File Name :"<<imgNames[i]<< "\n";

		dpt = cv::imread(depthNames[i]);
		img = cv::imread(imgNames[i]);

		std::vector<float> right_coeffs = getRightRail(dpt, M);
		cv::Mat marked = markTrackLine(img, right_coeffs, Minv);

		//std::cout << x_coordinates[0] << std::endl;
		cv::imshow("marked", marked);
		cv::waitKey(1);

	}

    return 0;
}



// converts pixels to real world, or real world to pixel based on perspective xform M
// see open cv warpPerspective
cv::Point2f xformPoint(float x, float y,cv::Mat M) {
	cv::Point2f newPoint;

	float den = (M.at<float>(2, 0)*x + M.at<float>(2, 1)*y + M.at<float>(2, 2));
	newPoint.x = (M.at<float>(0,0)*x + M.at<float>(0,1)*y + M.at<float>(0,2)) / den;
	newPoint.y = (M.at<float>(1, 0)*x + M.at<float>(1, 1)*y + M.at<float>(1, 2)) / den;

	//float den = matrix[2][0]*x + matrix[2][1]*y +matrix[2][2];
	//newPoint.x = (matrix[0][0]*x + matrix[0][1]*y + matrix[0][2]) / den;
	//newPoint.y = (matrix[1][0]*x + matrix[1][1]*y + matrix[1][2]) / den;

	return newPoint;
}

// creates a vector of points from a to b containing numPoints points
std::vector<float> linespace(float a, float b, int numPoints) {
	std::vector<float> line;
	float step = (b - a) / (numPoints - 1);
	while (a <= b) {

		line.push_back(a);
		a += step;
	}
	return line;
}

// 
cv::Mat markTrackLine(cv::Mat image, std::vector<float> coeffs, cv::Mat Minv) {
	cv::Mat color_line = cv::Mat::zeros(image.rows, image.cols, CV_32FC3);
	cv::Mat result;
	image.copyTo(result);

	std::vector<float> worldy;
	std::vector<float> worldx;
	std::vector<cv::Point2f> world;
	std::vector<cv::Point2f> plot;
	cv::Point2f point1;
	cv::Point2f point2;

	float y1 = 0;
	float y2 = 140;
	float x1 = coeffs[0];
	float x2 = coeffs[1] * 140 + coeffs[0]; // x at 140 inches

	point1 = xformPoint(x1, y1, Minv);
	point2 = xformPoint(x2, y2, Minv);

	cv::line(result, point1, point2, cv::Scalar(0, 0, 255), 20, 2);

	//cv::addWeighted(image, 1, color_line, 0.5, 0, result);

	return result;
}





std::vector<float> getRightRail(cv::Mat dpt, cv::Mat M){
	// convert depth image to grayscale
	cv::Mat depth_gray;
	cv::cvtColor(dpt, depth_gray, CV_BGR2GRAY);

	std::vector<float> x_coordinates;// holds the x (right is positive x) location of rail points, inches
	std::vector<float> y_coordinates;// holds the y (forward is positive y) location of rail points, inches
	std::vector<float> line_coeffs; // holds m and b for line y = mx +b
	cv::Point2f world;
	for (int row = 0; row < (9*depth_gray.rows/10); row++)
	{
		for ( int col = ( depth_gray.cols - 1 ) ; col > (depth_gray.cols/2); col -- )
		{
			if (depth_gray.at<uchar>(row,col ) > 100)
			{
				if (col != depth_gray.cols - 1)
				{
					world = xformPoint(col, row, M);

					x_coordinates.push_back(world.x);
					y_coordinates.push_back(world.y);
				}

				break;
			}
		}
	}

	line_coeffs = polyfit(y_coordinates, x_coordinates, 1);

	return line_coeffs;
}

std::vector<float> getLeftRail(cv::Mat dpt, cv::Mat M){
	// convert depth image to grayscale
	cv::Mat depth_gray;
	cv::cvtColor(dpt, depth_gray, CV_BGR2GRAY);

	std::vector<float> x_coordinates;// holds the x (right is positive x) location of rail points, inches
	std::vector<float> y_coordinates;// holds the y (forward is positive y) location of rail points, inches
	std::vector<float> line_coeffs; // holds m and b for line y = mx +b
	cv::Point2f world;

	for (int row = 0; row < (9*depth_gray.rows/10); row++)
	{
		for ( int col = 0 ; col < (depth_gray.cols/2); col ++ )
		{
			if (depth_gray.at<uchar>(row,col ) > 100)
			{
				if (col != depth_gray.cols - 1)
				{
					world = xformPoint(col, row, M);

					x_coordinates.push_back(world.x);
					y_coordinates.push_back(world.y);
				}
				break;
			}
		}
	}

	line_coeffs = polyfit(y_coordinates, x_coordinates, 1);

	return line_coeffs;
}