#include "StudentDetecting.h"

void StudentDetecting::get_room_position(double *rx, double *ry, double x, double y)
{
	double angle_a = atan(s/h) - atan(y/f);
	*ry = h * tan(angle_a);
	*rx = (x / sqrt(f *f + y * y) )* sqrt (h * h + *ry * *ry);
}

double StudentDetecting::get_real_up_speed(double dis)
{
	return 460 * -2.0 / dis;
}

double StudentDetecting::get_real_up_area(double dis)
{
	return  200000 / dis / dis  * 700;
}

double StudentDetecting::get_real_down_speed(double dis)
{
	return 460 * -1.8 / dis;
}

double StudentDetecting::get_real_down_area(double dis)
{
	return  200000 / dis / dis  * 500;
}

StudentDetecting::StudentDetecting(KVConfig *cfg)
	: cfg_(cfg)
{
	h = 130;
	const char *h_str = cfg_->get_value("camera_height", NULL);
	if (h_str != NULL)
	{
		h = atof(h_str);
	}
		
	s = 440;
	const char *s_str = cfg_->get_value("image_center_to_camera", NULL);
	if (s_str != NULL)
	{
		s = atof(s_str);
	}

	f = 470;
	const char *focal_ = cfg_->get_value("cam_trace_f", NULL);
	const char *ccd_w =  cfg_->get_value("cam_trace_ccd_w", NULL);
	const char *video_w = cfg_->get_value("video_width", NULL);
	if (focal_ && ccd_w && video_w)
	{
		f = atof(focal_) / atof(ccd_w) * atof(video_w);
	}
	f=470;
	
}

StudentDetecting::~StudentDetecting(void)
{
}

std::vector<StudentAction> StudentDetecting::one_frame(IplImage *img)
{
	std::vector<StudentAction> actions;
	// TODO: 这里实现所有 ....;
	cv::Mat pic(img);
	//cv::resize();
	cv::Mat gray;
	cv::Mat flow, yflow;
	std::vector<std::vector<cv::Point>> contours, contours_d;
	std::vector<cv::Rect> up_rects;
	cvtColor(pic, gray, CV_BGR2GRAY);
	if( pre_gray.data )
 	{
 		calcOpticalFlowFarneback(pre_gray, gray, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
 		yflow.create(flow.rows, flow.cols, CV_32FC1);
 		int ch[] = {1, 0};
		mixChannels(&flow, 1,  &yflow, 1, ch, 1);
  		cv::Mat result;
 		threshold(yflow, result, -1, 255, cv::THRESH_BINARY_INV);
		cv::Mat result_d;
		threshold(yflow, result_d, 1, 255, cv::THRESH_BINARY);
 		result.convertTo(result, CV_8UC1);	
		result_d.convertTo(result_d, CV_8UC1);
 		cv::Mat kernal = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
  		cv::morphologyEx(result, result, cv::MORPH_OPEN, kernal); 
		cv::morphologyEx(result, result, cv::MORPH_OPEN, kernal); 
		cv::morphologyEx(result, result, cv::MORPH_CLOSE, kernal);
		cv::morphologyEx(result, result, cv::MORPH_CLOSE, kernal);
		cv::morphologyEx(result_d, result_d, cv::MORPH_OPEN, kernal); 
		cv::morphologyEx(result_d, result_d, cv::MORPH_OPEN, kernal); 
		cv::morphologyEx(result_d, result_d, cv::MORPH_CLOSE, kernal);
		cv::morphologyEx(result_d, result_d, cv::MORPH_CLOSE, kernal);


		findContours(result, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
		findContours(result_d, contours_d, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
		for (int j = 0; j < contours.size(); j ++)
		{
			cv::Rect r0 = boundingRect(contours[j]);
			double x, y;
			x = r0.x + r0.width/2 - pic.cols/2;
			y = r0.y + r0.height - pic.rows/2;
			double rx, ry;
			get_room_position(&rx, &ry, x, y);
			double distance = sqrt(rx*rx + ry*ry);
			double speed = get_real_up_speed(distance);
			cv::Mat roi = yflow(r0);
			cv::Mat result2;
			threshold(roi, result2, speed, 255, cv::THRESH_BINARY_INV);
			result2.convertTo(result2, CV_8UC1);	
  			morphologyEx(result2, result2, cv::MORPH_OPEN, kernal); 
			morphologyEx(result2, result2, cv::MORPH_OPEN, kernal);
			morphologyEx(result2, result2, cv::MORPH_CLOSE, kernal);
			morphologyEx(result2, result2, cv::MORPH_CLOSE, kernal);
					
			std::vector<std::vector<cv::Point>> contours2;
			findContours(result2, contours2, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

			double area = get_real_up_area(distance);
			for (int i = 0; i < contours2.size(); i++)
			{
				if (contourArea(contours2[i]) < area)
					continue;

 				cv::Rect rect_bound = boundingRect(contours2[i]);
 				rect_bound.x += r0.x;
 				rect_bound.y += r0.y;
 				up_rects.push_back(rect_bound);	

 				for (int i = 0; i < pre_up_rects.size(); i++)
 				{
					cv::Rect r_intersect = rect_bound & pre_up_rects[i];
 					std::vector<std::vector<cv::Point>> contours_add;
	 				if (r_intersect.area() > area * 0.3)
	 				{
	 					cv::Mat roi = pre_yflow(r_intersect) + yflow(r_intersect);
	 					cv::Mat result_add;
	 					threshold(roi, result_add, speed * 2, 255, cv::THRESH_BINARY_INV);
	 					result_add.convertTo(result_add, CV_8UC1);				
	   					morphologyEx(result_add, result_add, cv::MORPH_OPEN, kernal); 
						morphologyEx(result_add, result_add, cv::MORPH_OPEN, kernal); 
	   					morphologyEx(result_add, result_add, cv::MORPH_CLOSE, kernal);
						morphologyEx(result_add, result_add, cv::MORPH_CLOSE, kernal);
	     				findContours(result_add, contours_add, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

	  					for (int i = 0; i < contours_add.size(); i++)
	  					{
	 						if (contourArea(contours_add[i]) > area * 0.2)
 							{
								StudentAction action;
								action.action = STUDENT_ACTION_STANDUP;
								action.pos = boundingRect(contours_add[i]);
								actions.push_back(action);
	 						} 									
	  					}
	 				}
				}							
			}					
		}

		for (int j = 0; j < contours_d.size(); j ++)
		{
			cv::Rect r0 = boundingRect(contours_d[j]);
			double x, y;
			x = r0.x + r0.width/2 - pic.cols/2;
			y = r0.y + r0.height - pic.rows/2;
			double rx, ry;
			get_room_position(&rx, &ry, x, y);
			double distance = sqrt(rx*rx + ry*ry);
			double speed = get_real_down_speed(distance);
			cv::Mat roi = yflow(r0);
			cv::Mat result2;
			threshold(roi, result2, speed, 255, cv::THRESH_BINARY);
			result2.convertTo(result2, CV_8UC1);	
  			morphologyEx(result2, result2, cv::MORPH_OPEN, kernal); 
			morphologyEx(result2, result2, cv::MORPH_OPEN, kernal);
			morphologyEx(result2, result2, cv::MORPH_CLOSE, kernal);
			morphologyEx(result2, result2, cv::MORPH_CLOSE, kernal);
					
			std::vector<std::vector<cv::Point>> contours2;
			findContours(result2, contours2, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

			double area = get_real_up_area(distance);
			for (int i = 0; i < contours2.size(); i++)
			{
				if (contourArea(contours2[i]) < area)
					continue;
 				cv::Rect rect_bound = boundingRect(contours2[i]);
 				rect_bound.x += r0.x;
 				rect_bound.y += r0.y;
				StudentAction action;
				action.action = STUDENT_ACTION_SITDOWN;
				action.pos = rect_bound;
				actions.push_back(action);
			}					
		}
	}

	std::swap(pre_gray, gray);
	std::swap(yflow, pre_yflow);
	std::swap(up_rects, pre_up_rects);
	return actions;
}
