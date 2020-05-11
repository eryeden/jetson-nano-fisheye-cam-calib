/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <ros/ros.h>

#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>

#include <jetson-utils/gstCamera.h>

#include "image_converter.h"

#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>



// globals	
gstCamera* camera = NULL;

imageConverter* camera_cvt = NULL;
ros::Publisher* camera_pub = NULL;

uint8_t counter = 0;

// aquire and publish camera frame
bool aquireFrame()
{
	float* imgRGBA = NULL;

	// get the latest frame
	if( !camera->CaptureRGBA(&imgRGBA, 1000) )
	{
		ROS_ERROR("failed to capture camera frame");
		return false;
	}

	// assure correct image size
	if( !camera_cvt->Resize(camera->GetWidth(), camera->GetHeight()) )
	{
		ROS_ERROR("failed to resize camera image converter");
		return false;
	}

	// populate the message
	sensor_msgs::Image msg;

	if( !camera_cvt->Convert(msg, sensor_msgs::image_encodings::BGR8, imgRGBA) )
	{
		ROS_ERROR("failed to convert camera frame to sensor_msgs::Image");
		return false;
	}

	cv_bridge::CvImagePtr cv_ptr;
	cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
	cv::Mat gray;
	cv::cvtColor(cv_ptr->image, gray, CV_BGR2GRAY);
	gray.copyTo(cv_ptr->image);
	cv_ptr->encoding = sensor_msgs::image_encodings::MONO8;


	// publish the message
	//camera_pub->publish(msg);
	//camera_pub->publish(cv_ptr->toImageMsg());
	//ROS_INFO("published camera frame");
	if((counter++ % 2) == 0){
	  camera_pub->publish(cv_ptr->toImageMsg());
	  ROS_INFO("published camera frame");
	}
	return true;
}


// node main loop
int main(int argc, char **argv)
{
	ros::init(argc, argv, "cam1");
 
	ros::NodeHandle nh;
	ros::NodeHandle private_nh("~");

	/*
	 * retrieve parameters
	 */
	std::string camera_device = "/dev/video1";	// MIPI CSI camera by default

	private_nh.param<std::string>("device", camera_device, camera_device);
	
	ROS_INFO("opening camera device %s", camera_device.c_str());

	
	/*
	 * open camera device
	 */
	camera = gstCamera::Create(camera_device.c_str());

	if( !camera )
	{
		ROS_ERROR("failed to open camera device %s", camera_device.c_str());
		return 0;
	}


	/*
	 * create image converter
	 */
	camera_cvt = new imageConverter();

	if( !camera_cvt )
	{
		ROS_ERROR("failed to create imageConverter");
		return 0;
	}


	/*
	 * advertise publisher topics
	 */
	ros::Publisher camera_publisher = private_nh.advertise<sensor_msgs::Image>("image_raw", 2);
	camera_pub = &camera_publisher;

	/*
	 * start the camera streaming
	 */
	if( !camera->Open() )
	{
		ROS_ERROR("failed to start camera streaming");
		return 0;
	}


	/*
	 * start publishing video frames
	 */
	while( ros::ok() )
	{
		//if( raw_pub->getNumSubscribers() > 0 )
			aquireFrame();

		ros::spinOnce();
	}

	delete camera;
	return 0;
}

