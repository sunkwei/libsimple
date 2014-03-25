#include "TeacherPtz.h"
#include "TeacherDetecting.h"
#include <assert.h>

TeacherPtz::TeacherPtz(KVConfig *cfg)
	: cfg_(cfg)
	, Ptz(cfg)
{
	running_state_ = 0;

	reseted_ = false;
	stable_ = false;

	smooth_ = false;
	if (atoi(cfg_->get_value("smooth_trace", "0")) != 0)
		smooth_ = true;

	policy_ = new TracePolicy(cfg_);
	
	ptz_left_ = atoi(cfg_->get_value("ptz_left", "0"));
	ptz_right_ = atoi(cfg_->get_value("ptz_right", "0"));
	ptz_v_ = atoi(cfg_->get_value("ptz_init_y", "0"));
	ptz_z_ = atoi(cfg_->get_value("ptz_init_z", "5000"));
	video_width_ = atoi(cfg_->get_value("video_width", "960"));
	
	// ���������䣬��ֹ��̨�ұ� ...
	areas_ = atoi(cfg_->get_value("teacher_area", "3"));
	if (areas_ == 0)
		areas_ = 3;

	if (ptz_left_ == 0) {
		ptz_left_ = -400;
		mylog("WARNING: %s: ptz_left NOT set, using default value=%d\n", __func__, ptz_left_);
	}
	if (ptz_right_ == 0) {
		ptz_right_ = 400;
		mylog("WARNING: %s: ptz_right NOT set, using default value=%d\n", __func__, ptz_right_);
	}

	calibration_left_edge_ = video_width_ / 2;
	calibration_right_edge_ = video_width_ / 2;

	load_calibration_edge();
	if (calibration_left_edge_ == calibration_right_edge_) {
		mylog("WARNING: %s: NO calibration data for '%s'????\n", __func__, cfg_->file_name());
	}

	area_length_ = (calibration_right_edge_ - calibration_left_edge_) / areas_;
	area_overlap_length_ = area_length_ / 4;	// FIXME: Ӧ�ô������ж�ȡ
	last_area_index_ = -1;	// 

	calc_calibration_area();
	assert(area_hori_.size() == areas_);
}

TeacherPtz::~TeacherPtz(void)
{
	delete policy_;
}

void TeacherPtz::load_calibration_edge()
{
	char key[64];
	for (int i = 2; i <= 3; i++) {
		snprintf(key, sizeof(key), "calibration_data%i", i);
		const char *pts = cfg_->get_value(key, 0);

		if (pts) {
			char *data = strdup(pts);
			char *p = strtok(data, ";");
			while (p) {
				// ÿ��Point ʹ"x,y" ��ʽ
				int x, y;
				if (sscanf(p, "%d,%d", &x, &y) == 2) {
					if (x < calibration_left_edge_) calibration_left_edge_ = x;
					if (x > calibration_right_edge_) calibration_right_edge_ = x;
				}

				p = strtok(0, ";");
			}
			free(data);
		}
	}
}

void TeacherPtz::calc_calibration_area()
{
	double min_angle = atof(cfg_->get_value("cam_trace_min_hangle", "0.075"));

	ca_.angle_left = (0 - ptz_left_) * min_angle * M_PI / 180.0;
	ca_.angle_right = ptz_right_ * min_angle * M_PI / 180;
	ca_.p_left = calibration_left_edge_;
	ca_.p_right = calibration_right_edge_;

	mylog("INFO: there are %d areas, [%.3f --> %.3f]/(%d, %d), [%d --> %d] with areas pos: \n\t", areas_,
		ca_.angle_left, ca_.angle_right, ptz_left_, ptz_right_,
		calibration_left_edge_, calibration_right_edge_);

	for (int i = 0; i < areas_; i++) {
		ca_.p = ca_.p_left + i * area_length_ + area_length_ / 2;
		double dh = TeacherDetecting::get_angle(ca_);

		int dhv = (int)(dh * 180.0 / M_PI / min_angle);

		area_hori_.push_back(ptz_left_ + dhv);

		Segment segment;
		segment.left = i * area_length_ - area_overlap_length_;
		segment.right = (i+1) * area_length_ + area_overlap_length_;
		
		if (segment.left < 0) segment.left = 0;
		if (segment.right > ca_.p_right) segment.right = (int)ca_.p_right;

		segments_.push_back(segment);

		mylog("%d:%d ", (int)ca_.p, area_hori_[i]);
	}

	mylog("\n");
}

void TeacherPtz::set_running_state(int state)
{
	running_state_ = state;

	if (running_state_ != 0) {
		reseted_ = false;
	}
}

cv::Point TeacherPtz::get_trace_last_pos() const
{
	return last_trace_pos_;
}

void TeacherPtz::turn_left_edge()
{
	if (ptz()) {
		ptz_set_absolute_position(ptz(), ptz_left_, ptz_v_, 30);
	}
}

void TeacherPtz::turn_right_edge()
{
	if (ptz()) {
		ptz_set_absolute_position(ptz(), ptz_right_, ptz_v_, 30);
	}
}

void TeacherPtz::one_loop()
{
}

int TeacherPtz::get_ptz_current_area()
{
	// ��ȡ��ǰ��ָ̨���Ǹ�����
	if (ptz()) {
		int h, v;
		ptz_get_current_position(ptz(), &h, &v);

		for (int i = 0; i < areas_; i++) {
			if (std::abs(h-area_hori_[i]) < 5) {
				return i;
			}
		}
	}

	return -1;
}

int TeacherPtz::area_changed(int offset, bool &changed)
{
	int index;
	changed = false;

	if (last_area_index_ == -1) {
		index = offset / area_length_;
		assert(index < areas_);
		changed = true;
		last_area_index_ = index;
	}
	else {
		if (offset < segments_[last_area_index_].left) {
			last_area_index_--;
			changed = true;
		}
		else if (offset > segments_[last_area_index_].right) {
			last_area_index_++;
			changed = true;
		}

		index = last_area_index_;
	}

	return index;
}

bool TeacherPtz::is_stable() const
{
	return stable_;
}

int TeacherPtz::get_area_from_data(const TeacherPtz::DetectedData &d, bool &changed)
{
	changed = false;
	assert(d.valid);
	assert(d.poss.size() > 0);

	Pos lr = d.poss[0];
	assert(lr.left >= calibration_left_edge_ && lr.right <= calibration_right_edge_ && lr.left < lr.right);
	int offset = (int)((lr.left + lr.right) / 2) - calibration_left_edge_;
	return area_changed(offset, changed);
}

double TeacherPtz::get_current_deflection()
{
	if (!ptz()) return 0.0;
	int h, v;
	ptz_get_current_position(ptz(), &h, &v);

	assert(h >= ptz_left_);
	double delta = (h - ptz_left_) * ptz_params_.min_angle_hori * M_PI / 180.0;	// ת��Ϊ����
	return delta;
}

double TeacherPtz::get_current_half_view_angle()
{
	return ptz_params_.view_angle_hori / 2.0 / current_scale() * M_PI / 180.0;
}

double TeacherPtz::get_target_deflection(const DetectedData &data)
{
	ca_.p = (data.poss[0].left + data.poss[0].right) / 2;
	return TeacherDetecting::get_angle(ca_);
}

void TeacherPtz::smooth_tracing(const DetectedData &data)
{
	if (!ptz()) return;

	if (data.valid) {
		double td = get_target_deflection(data);
		double cd = get_current_deflection();
		double fva = get_current_half_view_angle();
		
		if (cd - fva / 5 > td)
			ptz_left(ptz(), 1);
		else if (cd + fva / 5 < td)
			ptz_right(ptz(), 1);
		else
			ptz_stop(ptz());
	}
	else {
		if (ptz())	ptz_stop(ptz());
	}
}

void TeacherPtz::set_detected_data(const DetectedData &data)
{
	if (smooth_) {
		smooth_tracing(data);
	}
	else {
		data.valid ? timeout_chker_.set_valid_stamp(data.stamp) : timeout_chker_.set_invalid_stamp(data.stamp);
		if (timeout_chker_.is_valid()) {
			reseted_ = false;	// ֻҪ������Ч,���п�����Ҫ���¹�λ

			if (data.valid) {
				bool changed;
				int index = get_area_from_data(data, changed);
				timeout_chker_.set_area(index);
				if (changed) {
					if (ptz())
						add(task_ptz_set_position(ptz(), area_hori_[index], ptz_v_, ptz_z_));
				}

				if (timeout_chker_.valid_duration_area() > policy_->time_to_closeview()) {
					stable_ = true;
				}
				else {
					stable_ = false;
				}
			}
		}
		else {
			// �Ƿ���Ҫ�ص�ȫ����
			if (timeout_chker_.invalid_duration() > policy_->time_to_fullview_after_no_target()) {
				stable_ = false;
			}

			// �Ƿ���Ҫ��̨��λ
			if (timeout_chker_.invalid_duration() > policy_->time_to_reset_ptz()) {
				stable_ = false;

				if (this->running_state_ == 0) {
					if (!reseted_) {
						reseted_ = true;
						reset_ptz();
						last_area_index_ = -1;
					}
				}
			}
		}

		double dv = timeout_chker_.valid_duration(), dva = timeout_chker_.valid_duration_area(), div = timeout_chker_.invalid_duration();
		fprintf(stderr, "== valid: %.3f, valid area: %.3f, invlaid: %.3f \r", dv, dva, div);
	}
}

void TeacherPtz::set_detected_data2(const DetectedData &data)
{
	double ct = now();

	bool valid = chk_valid_data_.update(data.valid, ct);

	if (!valid) handle_invalid_data(ct);
	else {
		if (!data.valid) return;
		else handle_valid_data(ct, data);
	}
}

void TeacherPtz::handle_invalid_data(double c)
{
	// ����Ƿ�ʱ��û��Ŀ�꣬��̨�Ƿ���Ҫ��λ
	if (chk_valid_data_.duration(c) > policy_->time_to_reset_ptz()) {
		// ��Ҫ��ֹ��ͣ�� reset 
		if (!reseted_) {
			reset_ptz();
			reseted_ = true;
		}
	}
}

void TeacherPtz::handle_valid_data(double c, const DetectedData &data)
{
	reseted_ = false;

	/** Ŀ����Ч������Ŀ���Ƿ�����Ұ��Χ�У�
			1. ���Ŀ������Ұ�У���ƽ���Ľ�Ŀ��ת�����ģ�
			2. ���Ŀ�겻����Ұ�У������ת������Ŀ��������Ұ�У�
	 */

}
