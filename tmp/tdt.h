#ifndef _zonekey_teacher_detact_trace__HH
#define _zonekey_teacher_detact_trace__HH

/** 教师图像探测跟踪：
		基本原理：
			输入探测摄像机的图像，经过分析，找到老师的位置，控制跟踪摄像机进行跟踪。使用内置的策略，通知进行电影模式的切换

		切换策略:
			保证电影模式中，不出现云台转动和镜头拉伸的镜头。
			1. 电影输出：当教师不动（或范围很小）时，教师近景，当老师走动时，教师全景；
			2. 当全景时，跟踪相机跟踪教师；
 */

#ifdef __cplusplus
extern "C" {
#endif // c++

typedef struct tdt_t tdt_t;

typedef enum tdt_actions
{
	/** TODO: 重新定义通知编码， ....
	 */

	/// 以下作废 :(
	TDT_ACTION_SWITCH_TO_FULLVIEW = 1,	// 电影应该切换到全景模式，info 为""
	TDT_ACTION_SWITCH_TO_CLOSEUP = 2,	// 电影应该切换到近景模式，info 为""

	TDT_TARGET_FOUND = 101,		// 发现跟踪目标，info 将为 "%d,%d" 指出目标的坐标
	TDT_TARGET_SIZE = 102,		// 发现目标头肩大小，info 为 "%d,%d" 指出目标大小
} tdt_actions;

/** 回调通知，
 */
typedef void(*pfn_tdt_notify)(void *opaque, tdt_actions action_code, const char *info);

/** 返回实例 
	@param cfg_name: 配置文件名字
	@param notify: 回调函数
	@param opaque: 回调函数的第一个参数
	@return dtd实例，0 失败
 */
tdt_t *tdt_open(const char *cfg_name, pfn_tdt_notify notify, void *opaque);
void tdt_close(tdt_t *ctx);

/** 是否启用 */
void tdt_enable(tdt_t *ctx, int enable);
int tdt_is_enable(tdt_t *ctx);

/** 写入教师/学生探测摄像机yv12图像数据，数据大小可以变化 ...
	
	@param ctx: tdt_open() 返回的句柄
	@param width：图像宽度
	@param height: 图像高度
	@param fmt: 图像格式，目前必须 0，相当于 yv12
	@param data: 图像数据
	@param stride: 数据每行的字节数
	@param stamp： 时间戳，必须为 “秒”..
	@return 0 成功，否则失败
 */
int tdt_write_teacher_pic(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);
int tdt_write_student_pic(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);

/** 写入教师/学生跟踪相机的yv12图像数据 */
int tdt_write_teacher_pic_tracing(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);
int tdt_write_student_pic_tracing(tdt_t *ctx, int width, int height, int fmt, unsigned char *data[4], int stride[4], double stamp);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif // c++

#endif // _zonekey_teacher_detact_trace__HH
