#pragma once

#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include "TracePolicy.h"

// 保存教师探测数据
struct TeacherDetected
{
	double stamp;	// 数据有效时间戳
	cv::Point pt;	// 对应 camshift，在探测相机中的位置，如果 (-1, -1) 说明无效

	double stamp_size;	// 有效的头肩识别框的时间戳
	cv::Size hog_size;	// 头肩识别框的大小，如果 (-1, -1) 说明无效
};
typedef std::deque<TeacherDetected> TEACHERDETECTED;

/** 对教师数据进行分析，总是分析一段时间内，目标在探测区域的活动情况
 */
class TeacherAnalyze
{
	TracePolicy *policy_;
	KVConfig *cfg_;
	int width_, height_;
	int stable_width_, stable_height_, stable_top_;
	double duration_;	// 需要的时常
	TEACHERDETECTED data_;

public:
	TeacherAnalyze(KVConfig *cfg, TracePolicy *policy);
	~TeacherAnalyze(void);

	/// 更新数据，如果返回 true，说明可以分析数据了 ...
	bool update_data(const TEACHERDETECTED &data);

	double data_duration(double &valided) const;
	void data_valided(int *valid, int *invalid) const;	// 返回有效/无效点数
	void data_vec(int *h, int *v) const;				// 返回水平/竖直运动矢量，这个运动矢量可以描述模板运动的大致方向，绝对值越小说明活动范围越小
	bool data_first_valid(cv::Point &pt, double &stamp) const;					// 返回第一个有效的
	bool data_last_valid(cv::Point &pt, double &stamp) const;					// 返回最后一个有效的
	bool data_range(cv::Rect &range) const;										// 返回活动矩形
};
