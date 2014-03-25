#include "TeacherDetecting.h"


TeacherDetecting::TeacherDetecting(KVConfig *cfg)
	: cfg_(cfg)
{
}


TeacherDetecting::~TeacherDetecting(void)
{
}

//由小到大排序
int TeacherDetecting::cmp_func(const CvPoint&a,const CvPoint&b) 
{ 	   
	return a.x<b.x;
}

int TeacherDetecting::cmp_func_y(const CvPoint&a,const CvPoint&b) 
{ 	   
	return a.y<b.y;
}

//面积从大到小排序
int TeacherDetecting::cmp_area(const Rect&a,const Rect&b) 
{ 	   
	return (a.width*a.height>b.width*b.height);
}
vector<Pos> TeacherDetecting::refineSegments(Mat img, Mat& mask)
{
	vector<Pos> pos;pos.clear();
	vector<Rect> r;r.clear();
	int niters = 3;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	Mat temp;
	dilate(mask, temp, Mat(), Point(-1,-1), niters);
	erode(temp, temp, Mat(), Point(-1,-1), niters);
	dilate(temp, temp, Mat(), Point(-1,-1), niters);
	findContours( temp, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );

	if(contours.size()>0)
	{
		vector<vector<Point>>::iterator it1;		
		for(it1 = contours.begin();it1!= contours.end();it1++)
		{
			vector<Point> cor = *it1;
			Rect t_rect = boundingRect(Mat(cor));

			if(t_rect.area()>(img.cols*230/960))
			{
				r.push_back(t_rect);//存储外接矩形框
			}		
		}
		//按矩形框面积由大到小排序
		std::sort(r.begin(),r.end(),cmp_area);
		//存储矩形框的左右x值
		if(r.size()>0)
		{
			for(int i=0;i<r.size();i++)
			{
				Rect rect = r[i];
				Pos p;p.left = rect.x;p.right= rect.x+rect.width;
				pos.push_back(p);
			}
			rectangle(img,Point(r[0].x,r[0].y),Point(r[0].x+r[0].width,r[0].y+r[0].height),Scalar(0,0,255),2);
		}
	}
	
	return pos;

}

bool TeacherDetecting::one_frame(IplImage *img, const std::vector<cv::Point> &roi, vector<Pos> &r)
{
	/** TODO: 这里找到目标 */
	Mat bgmask;
	bool object = false;	
	Mat tmp_frame(img);
	//resize(tmp_frame, tmp_frame, Size(960, 540));
	bgsubtractor(tmp_frame, bgmask, -1);
	vector<Pos> t = refineSegments(tmp_frame, bgmask);
	if(t.size()>0) 
	{
		r = t;
		object = true;
	}
	return object;
}


//返回左侧的角度（弧度）
double TeacherDetecting::get_angle(Cal_Angle t)
{
	double mid_len = (fabs)(t.p_right-t.p_left)/(tan(t.angle_left)+tan(t.angle_right));
	double left_len = mid_len*tan(t.angle_left);
	double m_l_angle = atan((left_len-(t.p-t.p_left))/mid_len);
	double angle = t.angle_left-m_l_angle;
	return angle;
}