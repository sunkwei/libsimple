#include <stdio.h>
#include <stdlib.h>
#include "ptz_oper.h"
#include <math.h>

int my_ptz_stop(ptz_t *ptz, int notused)
{
	return ptz_stop(ptz);
}

Dir ptz_get_dir(const cv::Point &a, const cv::Point &b, pfn_ptz_move *pfn, const char **desc)
{
	/** 从 a --> b，要求必须使用相同坐标 :)
	 */
	double x = b.x - a.x, y = b.y - a.y;

	if (sqrt(pow(x, 2) + pow(y ,2)) < 5) {
		if (desc) *desc = "MIDDLE";
		return MIDDLE;
	}

	double at = atan2(y, x) + M_PI/8;
	if (at < 0.0) {
		at += 2 * M_PI;
	}
	at *= 180.0;
	at /= M_PI;				// 转换为角度

	int index = at / 45.0;	// 8 个方向，每个占45度
	
	if (index < 0 || index >= 8) {
		__asm int 3;
	}

	static const Dir _dirs[8] = { LEFT, UPLEFT, UP, UPRIGHT, RIGHT, DOWNRIGHT, DOWN, DOWNLEFT };
	static const char *_desc[8] = { "Left", "UP Left", "UP", "UP Right", "Right", "DOWN Right", "DOWN", "DOWN Left" };
	static pfn_ptz_move _funcs[8] = { ptz_left, ptz_upleft, ptz_up, ptz_upright, ptz_right, ptz_downright, ptz_down, ptz_downleft };

	if (desc) *desc = _desc[index];
	if (pfn) *pfn = _funcs[index];
	return _dirs[index];
}
