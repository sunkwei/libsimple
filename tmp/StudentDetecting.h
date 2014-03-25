#pragma once

#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include <opencv2/opencv.hpp>
#include <vector>

#define STUDENT_ACTION_STANDUP 1
#define STUDENT_ACTION_SITDOWN 2

struct StudentAction
{
	int action;		// 动作，STUDENT_ACTION_XXX
	cv::Rect pos;	// 在探测相机中的位置
};

/** 实现学生探测
 */
class StudentDetecting
{
	KVConfig *cfg_;
	double h;
	double s;
	double f;
	cv::Mat pre_gray;
	cv::Mat pre_yflow;
	std::vector<cv::Rect> pre_up_rects;

	void get_room_position(double *rx, double *ry, double x, double y);
	double get_real_up_speed(double dis);
	double get_real_up_area(double dis);
	double get_real_down_speed(double dis);
	double get_real_down_area(double dis);
public:

	StudentDetecting(KVConfig *cfg);
	~StudentDetecting(void);

	/** 根据 img 得到是否有学生站起，或者坐下，可能有多个学生的动作
	 */
	std::vector<StudentAction> one_frame(IplImage *img);
};
