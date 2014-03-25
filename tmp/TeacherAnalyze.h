#pragma once

#include "../libzqpkt_img_fifo/simple_student_trace/KVConfig.h"
#include "TracePolicy.h"

// �����ʦ̽������
struct TeacherDetected
{
	double stamp;	// ������Чʱ���
	cv::Point pt;	// ��Ӧ camshift����̽������е�λ�ã���� (-1, -1) ˵����Ч

	double stamp_size;	// ��Ч��ͷ��ʶ����ʱ���
	cv::Size hog_size;	// ͷ��ʶ���Ĵ�С����� (-1, -1) ˵����Ч
};
typedef std::deque<TeacherDetected> TEACHERDETECTED;

/** �Խ�ʦ���ݽ��з��������Ƿ���һ��ʱ���ڣ�Ŀ����̽������Ļ���
 */
class TeacherAnalyze
{
	TracePolicy *policy_;
	KVConfig *cfg_;
	int width_, height_;
	int stable_width_, stable_height_, stable_top_;
	double duration_;	// ��Ҫ��ʱ��
	TEACHERDETECTED data_;

public:
	TeacherAnalyze(KVConfig *cfg, TracePolicy *policy);
	~TeacherAnalyze(void);

	/// �������ݣ�������� true��˵�����Է��������� ...
	bool update_data(const TEACHERDETECTED &data);

	double data_duration(double &valided) const;
	void data_valided(int *valid, int *invalid) const;	// ������Ч/��Ч����
	void data_vec(int *h, int *v) const;				// ����ˮƽ/��ֱ�˶�ʸ��������˶�ʸ����������ģ���˶��Ĵ��·��򣬾���ֵԽС˵�����ΧԽС
	bool data_first_valid(cv::Point &pt, double &stamp) const;					// ���ص�һ����Ч��
	bool data_last_valid(cv::Point &pt, double &stamp) const;					// �������һ����Ч��
	bool data_range(cv::Rect &range) const;										// ���ػ����
};
