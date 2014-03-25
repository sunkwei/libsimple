#pragma once

#include <deque>
#include "TracePolicy.h"
#include "Ptz.h"
#include "TeacherDetecting.h"

/** 教师云台控制，自身为一工作线程，因为云台操作往往比较耗时，不应该阻塞图像处理 */
class TeacherPtz : public Ptz
{
public:
	// 探测到的目标的位置
	class DetectedData
	{
	public:
		DetectedData()
		{
			valid = false;
			stamp = now();
		}

		bool valid;		// 数据是否有效？
		double stamp;	// 时间戳

		std::vector<Pos> poss;	// 出现的框
	};

private:
	// 描述一个区段，使用 Segment 描述 area 的位置信息
	struct Segment
	{
		int left, right;
	};

	/** 保存目标与 area 的关系，持续时间，将用于判断目标的动作 
	 */
	class AreaDuration
	{
/*
	FIXME: 特别的，可能出现偶尔一帧没有目标，导致统计出错，所以需要
	排除掉非连续的切换 ...
	*/
#define CONTINIOUS_COUNT 2

		int area_;
		double duration_valid_, duration_invalid_;
		double duration_valid_area_;
		double last_;
		int contious_cnt_;
		int state_;	// -1: init, 0 valid, 1 invalid
		bool valid_;	// 是否有效状态

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

		// 如果id修改，则返回 true
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
					// 说明出现连续有效点，需要改变状态了
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
					// 说明出现连续无效点，需要修改状态
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

	bool reseted_;					// 云台是否已经初始化，防止连续设置云台
	bool stable_;		// 目标是否较长时间在固定 area 中
	bool smooth_;		// 是否平滑跟踪模式

	int running_state_;
	cv::Point last_trace_pos_;

	int ptz_left_, ptz_right_;	// 云台的最左最右的位置
	int ptz_v_, ptz_z_;	// 对应着标定的竖直
	int areas_;	// 教师位置的个数，不要总是不停的转云台 
	int area_length_;	// 每个区间的平均长度，
	int area_overlap_length_;	// 区域重叠长度，一般采用 area_length_ 的 1/4 ??
	std::vector<int> area_hori_;	// 云台对应每个area需要转动的水平位置

	int video_width_;
	int calibration_left_edge_, calibration_right_edge_; // 教师标定区的左边和右边

	Cal_Angle ca_;

	int last_area_index_;	// 最新指向区域
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
	double get_current_deflection();	// 返回当前角度到最左侧的偏差
	double get_current_half_view_angle();	// 返当前视角的一半弧度
	double get_target_deflection(const DetectedData &data);	// 返回目标距离最左侧的偏差

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
			// 当前状态持续时间
			return c - changed_stamp;
		}
	};

	ChkValidData chk_valid_data_;
	double last_valid_data_;

	void handle_invalid_data(double c);
	void handle_valid_data(double c, const DetectedData &data);
};
