#include "TeacherAnalyze.h"
#include <assert.h>
#include <algorithm>

TeacherAnalyze::TeacherAnalyze(KVConfig *cfg, TracePolicy *policy)
	: cfg_(cfg)
	, policy_(policy)
{
	width_ = atoi(cfg_->get_value("video_width", "960")), height_ = atoi(cfg_->get_value("video_height", "540"));
	stable_width_ = atoi(cfg_->get_value("stable_width", "500"));
	stable_height_ = atoi(cfg_->get_value("stable_height", "300"));
	stable_top_ = atoi(cfg_->get_value("stable_top", "100"));
	duration_ = policy->analyze_interval();
}

TeacherAnalyze::~TeacherAnalyze(void)
{
}

bool TeacherAnalyze::update_data(const TEACHERDETECTED &data)
{
	assert(data.size() >= 2);
	double last = data.back().stamp;
	double first = last - duration_ - 1.0;

	data_.clear();
	for (TEACHERDETECTED::const_iterator it = data.begin(); it != data.end(); ++it) {
		if (it->stamp >= first)
			data_.push_back(*it);
	}

	double valid;
	return data_duration(valid) >= duration_;
}

double TeacherAnalyze::data_duration(double &valid) const
{
	if (data_.size() >= 2) {
		cv::Point pt1, pt2;
		double s1, s2;
		if (data_first_valid(pt1, s1) && data_last_valid(pt2, s2)) {
			valid = s2 - s1;
		}
		else {
			valid = 0.0;
		}
		return data_.back().stamp - data_.front().stamp;
	}
	else {
		valid = 0.0;
		return 0.0;
	}
}

void TeacherAnalyze::data_valided(int *valid, int *invalid) const
{
	*valid = 0, *invalid = 0;

	TEACHERDETECTED::const_iterator it;
	for (it = data_.begin(); it != data_.end(); ++it) {
		if (it->pt.x != -1) {
			(*valid)++;
		}
		else {
			(*invalid)++;
		}
	}
}

bool TeacherAnalyze::data_first_valid(cv::Point &pt, double &stamp) const
{
	TEACHERDETECTED::const_iterator it;
	for (it = data_.begin(); it != data_.end(); ++it) {
		if (it->pt.x != -1) {
			pt = it->pt;
			stamp = it->stamp;
			return true;
		}
	}
	return false;
}

bool TeacherAnalyze::data_last_valid(cv::Point &pt, double &stamp) const
{
	TEACHERDETECTED::const_reverse_iterator it;
	for (it = data_.rbegin(); it != data_.rend(); ++it) {
		if (it->pt.x != -1) {
			pt = it->pt;
			stamp = it->stamp;
			return true;
		}
	}
	return false;
}

void TeacherAnalyze::data_vec(int *h, int *v) const
{
	*h = 0, *v = 0;

	/** XXX: 这里简单的使用有效点之间的差值和作为运动矢量
	 */
	bool started = false;
	int x, y;

	TEACHERDETECTED::const_iterator it;
	for (it = data_.begin(); it != data_.end(); ++it) {
		if (it->pt.x != -1) {
			if (!started) {
				started = true;
			}
			else {
				*h += it->pt.x - x;
				*v += it->pt.y - y;
			}
			x = it->pt.x, y = it->pt.y;
		}
	}
}

bool TeacherAnalyze::data_range(cv::Rect &range) const
{
	range = cv::Rect(-1, -1, 0, 0);

	int l = -1, r, t, b;

	TEACHERDETECTED::const_iterator it;
	for (it = data_.begin(); it != data_.end(); ++it) {
		if (it->pt.x != -1) {
			if (l == -1) {
				// 第一个有效点
				l = it->pt.x;
				r = it->pt.x;
				t = it->pt.y;
				b = it->pt.y;
			}
			else {
				l = std::min<int>(it->pt.x, l);
				r = std::max<int>(it->pt.x, r);
				t = std::min<int>(it->pt.y, t);
				b = std::max<int>(it->pt.y, b);

				range = cv::Rect(l, t, r-l, b-t);
			}
		}
	}

	return range.width != 0;
}
