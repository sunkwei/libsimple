#pragma once

#include <map>
#include <cc++/thread.h>

template<typename T = int>
class StateDuration
{
	T state_;
	double stamp_;

public:
	StateDuration()
	{
		stamp_ = -1.0;
	}

	void set_state(const T &s)
	{
		bool changed = false;

		if (stamp_ < 0.0) changed = true;
		else if (s != state_) changed = true;

		if (changed) {
			state_ = s;
			stamp_ = now();
		}
	}

	T state() const { return state_; }
	double duration() const { return now() - stamp_; }

private:
	double now() const
	{
		timeval tv;
		ost::gettimeofday(&tv, 0);
		return tv.tv_sec + tv.tv_usec/1000000.0;
	}
};

