#ifndef __SUBTASK_H
#define __SUBTASK_H


/**********************************************************************************************************************/
#define track_speed_cmps_default  20				//默认侧面轨迹寻迹时的巡航速度10 
#define start_point_adjust1_default -2				//识别到倒车入库视觉特征点后，向前或者向后需要调整的距离-8.0f

#define forward_distance_cm_default 20				//倒车入库第一个动作执行的距离——轮胎左打并前进
#define backward_distance1_cm_default 27			//倒车入库第二个动作执行的距离——轮胎右打并后退
#define backward_distance2_cm_default 30      //倒车入库第三个动作执行的距离——轮胎回正并后退

#define out_forward_distance1_cm_default 25		//倒车入库后出库第一动作执行距离——轮胎回正并前进
#define out_forward_distance2_cm_default 40   //倒车入库后出库第二动作执行距离——轮胎右打并前进

/**********************************************************************************************************************/
#define start_point_adjust2_default 25////识别到侧方入库视觉特征点后，向前或者向后需要调整的距离30  25
#define parallel_backward_distance1_cm_default 22.0f//侧方入库第一个动作执行的距离——前轮右打时，回退距离  20 25
#define parallel_backward_distance2_cm_default 17.5f//侧方入库第二个动作执行的距离——前轮回正时，回退距离  20
#define parallel_backward_distance3_cm_default 15.0f//侧方入库第三个动作执行的距离——前轮左打时，回退距离	15
/**********************************************************************************************************************/


typedef struct
{
	float _track_speed_cmps;
	float _start_point_adjust1;
	float _forward_distance_cm;
	float _backward_distance1_cm;
	float _backward_distance2_cm;
	float _out_forward_distance1_cm;
	float _out_forward_distance2_cm;
	float _start_point_adjust2;
	float _parallel_backward_distance1_cm;
	float _parallel_backward_distance2_cm;
	float _parallel_backward_distance3_cm;
}_park_params;




void flight_subtask_reset(void);


void flight_subtask_1(void);
void flight_subtask_2(void);
void flight_subtask_3(void);
void flight_subtask_4(void);
void flight_subtask_5(void);
void auto_reverse_stall_park(void);
void auto_parallel_park(void);


uint8_t flight_subtask_yaw_angle_ctrl(float target_angle);
void auto_drive_smartcar_duty1(void);
void auto_drive_smartcar_duty2(void);	
void auto_drive_smartcar_duty3(uint16_t times);
void auto_drive_smartcar_duty4(uint16_t times);
void auto_nav_point(uint16_t times);
void auto_drive_2026_task2(void);
void auto_drive_2026_task3(void);
void auto_drive_2026_task4(void);
void auto_drive_2026_task5(void);
void auto_drive_2026_task6(void);
void auto_drive_2026_task7(void);
void auto_drive_2026_vision_debug(void);

extern _park_params park_params;


#define distance_ctrl_speed_max     50   //50
#define self_guided_tracking_speed         30.0f //track_vel default
#define self_guided_tracking_speed_legacy  60.0f //previous default, migration only
#define move_diagonal_angle1        35   //38.65
#define move_diagonal_distance1     125  //128
#define move_diagonal_angle2        35   //38.65
#define move_diagonal_distance2     125  //128

extern float _distance_ctrl_speed_max,_self_guided_tracking_speed,_move_diagonal_angle1,_move_diagonal_distance1,_move_diagonal_angle2,_move_diagonal_distance2;



typedef enum
{
	AUTO_VISION_2026_TASK_IDLE=0,
	AUTO_VISION_2026_TASK_2=2,
	AUTO_VISION_2026_TASK_3=3,
	AUTO_VISION_2026_TASK_4=4,
	AUTO_VISION_2026_TASK_5=5,
	AUTO_VISION_2026_TASK_6=6,
	AUTO_VISION_2026_TASK_7=7,
	AUTO_VISION_2026_TASK_DEBUG_CENTER=8
}AutoVision2026TaskId;

typedef enum
{
	AUTO_VISION_2026_PHASE_IDLE=0,
	AUTO_VISION_2026_PHASE_HOLD_CENTER,
	AUTO_VISION_2026_PHASE_TASK3_WAIT_CENTER,
	AUTO_VISION_2026_PHASE_TASK3_TO_CENTER,
	AUTO_VISION_2026_PHASE_TASK3_TO_POSITIVE,
	AUTO_VISION_2026_PHASE_TASK3_TO_NEGATIVE,
	AUTO_VISION_2026_PHASE_TASK3_HOLD_NEGATIVE,
	AUTO_VISION_2026_PHASE_TASK6_CAPTURE,
	AUTO_VISION_2026_PHASE_TASK6_HOLD,
	AUTO_VISION_2026_PHASE_TASK67_SELECT
}AutoVision2026Phase;

typedef enum
{
	AUTO_VISION_2026_ROUTE_STOP=0,
	AUTO_VISION_2026_ROUTE_ONE_LAP_STOP_A,
	AUTO_VISION_2026_ROUTE_TO_B,
	AUTO_VISION_2026_ROUTE_ONE_LAP_PASS_A
}AutoVision2026RouteMode;

typedef struct
{
	int16_t position_tolerance_mm;
	int16_t velocity_tolerance_mm_s;
	int16_t positive_target_mm;
	int16_t negative_target_mm;
	int16_t ball_center_limit_mm;
	uint16_t settle_time_ms;
	uint16_t vision_timeout_ms;
	float task3_positive_position_gain_scale;
}AutoVision2026Config;

typedef struct
{
	uint8_t task_id;
	uint8_t phase;
	uint8_t route_mode;
	uint8_t balance_enabled;
	uint8_t request_safe_level;
	uint8_t task_complete;
	uint8_t deadline_exceeded;
	int16_t target_mm;
	int16_t position_error_mm;
	uint32_t task_start_ms;
}AutoVision2026Status;

extern AutoVision2026Config g_auto_vision_2026_config;
extern AutoVision2026Status g_auto_vision_2026_status;

void auto_vision_2026_init(void);
uint8_t auto_vision_2026_start(uint8_t task_id);
void auto_vision_2026_update(uint8_t route_complete);
void auto_vision_2026_stop(void);
void auto_vision_2026_task2(uint8_t route_complete);
void auto_vision_2026_task3(void);
void auto_vision_2026_task4(uint8_t route_complete);
void auto_vision_2026_task5(uint8_t route_complete);
void auto_vision_2026_task6(uint8_t route_complete);
void auto_vision_2026_task7(uint8_t route_complete);
void auto_vision_2026_debug_center(void);
#endif
