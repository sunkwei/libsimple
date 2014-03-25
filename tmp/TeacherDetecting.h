#pragma once

#include <opencv2/opencv.hpp>
#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include <vector>
using namespace cv;
using namespace std;
/** ��ʦ��̽�⣬
		��������ʹ�ã����úڰ��·�һ��ͼ������
		
 */

//̽���˶�����ṹ��
typedef struct Pos
{
	double left;//���xֵ
	double right;//�Ҳ�xֵ
}Pos;
 
//�������ת���ǶȲ����ṹ��
typedef struct Cal_Angle
{
	double angle_left;//ת�����궨���������Ƕȣ����ȣ�
	double angle_right;//ת�����궨���Ҳ�����Ƕȣ����ȣ�
	double p_left;//�궨�����x������
	double p_right;//�궨���Ҳ�x������
	double p;//̽����x������
}Cal_Angle;

class TeacherDetecting
{
	KVConfig *cfg_;
	BackgroundSubtractorMOG bgsubtractor;
public:
	TeacherDetecting(KVConfig *cfg);
	~TeacherDetecting(void);
	 

	/** img Ϊ��ʦ̽��ͼ��
		roi Ϊimg�У���Ҫ���б������������򣬰���˳�����һ�������
		left, right: ΪĿ����roi�е�λ��
		����ҵ�Ŀ�꣬���� true�����򷵻� false
	 */
	bool one_frame(IplImage *img, const std::vector<cv::Point> &roi, vector<Pos> &r);
	vector<Pos> refineSegments(Mat img, Mat& mask);
	static int cmp_func(const CvPoint&a,const CvPoint&b);
	static int cmp_func_y(const CvPoint&a,const CvPoint&b);
	static int cmp_area(const Rect&a,const Rect&b);

	static double get_angle(Cal_Angle t);
};
