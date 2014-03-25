#pragma once

#include <deque>
#include "TracePolicy.h"
#include "Ptz.h"
#include "TeacherDetecting.h"

/** ��ʦ��̨���ƣ�����Ϊһ�����̣߳���Ϊ��̨���������ȽϺ�ʱ����Ӧ������ͼ���� */
class TeacherPtz : public Ptz
{
public:
	// ̽�⵽��Ŀ���λ��
	class DetectedData
	{
	public:
		DetectedData()
		{
			valid = false;
			stamp = now();
		}

		bool valid;		// �����Ƿ���Ч��
		double stamp;	// ʱ���

		std::vector<Pos> poss;	// ���ֵĿ�
	};

private:
	// ����һ�����Σ�ʹ�� Segment ���� area ��λ����Ϣ
	struct Segment
	{
		int left, right;
	};

	/** ����Ŀ���� area �Ĺ�ϵ������ʱ�䣬�������ж�Ŀ��Ķ��� 
	 */
	class AreaDuration
	{
/*
	FIXME: �ر�ģ����ܳ���ż��һ֡û��Ŀ�꣬����ͳ�Ƴ���������Ҫ
	�ų������������л� ...
	*/
#define CONTINIOUS_COUNT 2

		int area_;
		double duration_valid_, duration_invalid_;
		double duration_valid_area_;
		double last_;
		int contious_cnt_;
		int state_;	// -1: init, 0 valid, 1 invalid
		bool valid_;	// �Ƿ���Ч״̬

	public:
		AreaDuration()
		{
			area_ = -1;
			duration_valid_ = 0.0;
			duration_invalid_ = 0.0;
			duration_valid_area_ = 0.0;
			last_ = -1.0;
			contious_cnt_ = CONTINIOUS_COUNT;
			valid_ = false;
		}

		// ���id�޸ģ��򷵻� true
		void set_area(int id)
		{
			bool changed = (area_ != id);
			area_ = id;
			if (changed) duration_valid_area_ = 0.0;
		}

		void set_valid_stamp(double stamp)
		{
			if (last_ > 0.0) {
				duration_valid_ += stamp - last_;
				duration_valid_area_ += stamp - last_;
			}

			last_ = stamp;

			if (!valid_) {
				if (--contious_cnt_ == 0) {
					// ˵������������Ч�㣬��Ҫ�ı�״̬��
					contious_cnt_ = CONTINIOUS_COUNT;
					valid_ = true;
					duration_invalid_ = 0.0;
				}
			}
			else {
				contious_cnt_ = CONTINIOUS_COUNT;
			}
		}

		void set_invalid_stamp(double stamp)
		{
			if (last_ > 0.0)
				duration_invalid_ += stamp - last_;

			last_ = stamp;

			if (valid_) {
				if (--contious_cnt_ == 0) {
					// ˵������������Ч�㣬��Ҫ�޸�״̬
					contious_cnt_ = CONTINIOUS_COUNT;
					valid_ = false;
					duration_valid_area_ = 0.0;
					duration_valid_ = 0.0;
				}
			}
			else {
				contious_cnt_ = CONTINIOUS_COUNT;
			}
		}

		bool is_valid() const { return valid_; }

		double valid_duration() const
		{
			if (valid_)
				return duration_valid_;
			else
				return 0.0;
		}

		double valid_duration_area() const
		{
			if (valid_)
				return duration_valid_area_;
			else
				return 0.0;
		}

		double invalid_duration() const
		{
			if (!valid_)
				return duration_invalid_;
			else
				return 0.0;
		}

		int area() const
		{
			return area_;
		}
	};

private:

	KVConfig *cfg_;
	TracePolicy *policy_;

	bool reseted_;					// ��̨�Ƿ��Ѿ���ʼ������ֹ����������̨
	bool stable_;		// Ŀ���Ƿ�ϳ�ʱ���ڹ̶� area ��
	bool smooth_;		// �Ƿ�ƽ������ģʽ

	int running_state_;
	cv::Point last_trace_pos_;

	int ptz_left_, ptz_right_;	// ��̨���������ҵ�λ��
	int ptz_v_, ptz_z_;	// ��Ӧ�ű궨����ֱ
	int areas_;	// ��ʦλ�õĸ�������Ҫ���ǲ�ͣ��ת��̨ 
	int area_length_;	// ÿ�������ƽ�����ȣ�
	int area_overlap_length_;	// �����ص����ȣ�һ����� area_length_ �� 1/4 ??
	std::vector<int> area_hori_;	// ��̨��Ӧÿ��area��Ҫת����ˮƽλ��

	int video_width_;
	int calibration_left_edge_, calibration_right_edge_; // ��ʦ�궨������ߺ��ұ�

	Cal_Angle ca_;

	int last_area_index_;	// ����ָ������
	std::vector<Segment> segments_;

	AreaDuration timeout_chker_;

public:
	TeacherPtz(KVConfig *cfg);
	~TeacherPtz(void);

	void set_detected_data(const DetectedData &data);
	void set_detected_data2(const DetectedData &data);
	void set_running_state(int state);
	void smooth_tracing(const DetectedData &data);

	cv::Point get_trace_last_pos() const;

	void turn_left_edge();
	void turn_right_edge();

	bool is_stable() const;

private:
	virtual void one_loop();
	void load_calibration_edge();
	void calc_calibration_area();
	int area_changed(int offset, bool &changed);
	int get_ptz_current_area();
	int get_area_from_data(const DetectedData &data, bool &changed);
	double get_current_deflection();	// ���ص�ǰ�Ƕȵ�������ƫ��
	double get_current_half_view_angle();	// ����ǰ�ӽǵ�һ�뻡��
	double get_target_deflection(const DetectedData &data);	// ����Ŀ�����������ƫ��

	struct ChkValidData
	{
#define VALID_CONTIOUS 5
		bool last_valid, curr;
		int contious_cnt;
		double changed_stamp;

		ChkValidData()
		{
			curr = false;
			last_valid = false;
			contious_cnt = VALID_CONTIOUS;
			changed_stamp = now();
		}

		bool update(bool d, double stamp)
		{
			if (d == last_valid) {
				if (d == curr) {
					return curr;
				}
				
				contious_cnt --;
				if (contious_cnt <= 0) {
					contious_cnt = VALID_CONTIOUS;
					curr = d;
					changed_stamp = stamp;
					return curr;
				}
				else {
					return curr;
				}
			}
			else {
				last_valid = d;
				contious_cnt = VALID_CONTIOUS;
				return curr;
			}
		}

		double duration(double c)
		{
			// ��ǰ״̬����ʱ��
			return c - changed_stamp;
		}
	};

	ChkValidData chk_valid_data_;
	double last_valid_data_;

	void handle_invalid_data(double c);
	void handle_valid_data(double c, const DetectedData &data);
};
