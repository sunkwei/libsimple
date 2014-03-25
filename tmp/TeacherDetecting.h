#pragma once

#include <opencv2/opencv.hpp>
#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include <vector>
using namespace cv;
using namespace std;
/** 教师的探测，
		初步决定使用：利用黑板下方一条图像，生成
		
 */

//探测运动区域结构体
typedef struct Pos
{
	double left;//左侧x值
	double right;//右侧x值
}Pos;
 
//求摄像机转动角度参数结构体
typedef struct Cal_Angle
{
	double angle_left;//转动到标定区左侧所需角度（弧度）
	double angle_right;//转动到标定区右侧所需角度（弧度）
	double p_left;//标定区左侧x轴坐标
	double p_right;//标定区右侧x轴坐标
	double p;//探测点的x轴坐标
}Cal_Angle;

class TeacherDetecting
{
	KVConfig *cfg_;
	BackgroundSubtractorMOG bgsubtractor;
public:
	TeacherDetecting(KVConfig *cfg);
	~TeacherDetecting(void);
	 

	/** img 为教师探测图像；
		roi 为img中，需要进行背景剪除的区域，按照顺序，组成一个多边形
		left, right: 为目标在roi中的位置
		如果找到目标，返回 true，否则返回 false
	 */
	bool one_frame(IplImage *img, const std::vector<cv::Point> &roi, vector<Pos> &r);
	vector<Pos> refineSegments(Mat img, Mat& mask);
	static int cmp_func(const CvPoint&a,const CvPoint&b);
	static int cmp_func_y(const CvPoint&a,const CvPoint&b);
	static int cmp_area(const Rect&a,const Rect&b);

	static double get_angle(Cal_Angle t);
};
