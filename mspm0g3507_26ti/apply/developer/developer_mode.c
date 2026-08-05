#include "headfile.h"
#include "sdk.h"
#include "subtask.h"
#include "user.h"
#include "developer_mode.h"

int16_t sdk_work_mode=0;


#define wheel_space_cm  12.8f//轮间距  12.8cm



void sdk_duty_run(void)
{
	static int16_t last_sdk_work_mode=32767;
	uint8_t visual_mode_active=
		(sdk_work_mode==3||sdk_work_mode==4||sdk_work_mode==5||
		 sdk_work_mode==6||sdk_work_mode==7||sdk_work_mode==27);

	/* Reset static lap phases whenever the selected work mode changes. */
	if(sdk_work_mode!=last_sdk_work_mode)
	{
		last_sdk_work_mode=sdk_work_mode;
		flight_subtask_reset();
		if(g_auto_vision_2026_status.task_id!=AUTO_VISION_2026_TASK_IDLE)
			auto_vision_2026_stop();
	}

	if(!visual_mode_active&&
	   g_auto_vision_2026_status.task_id!=AUTO_VISION_2026_TASK_IDLE)
	{
		auto_vision_2026_stop();
	}

	if(trackless_output.init==0)
	{		
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output=0;
		trackless_output.init=1;
		flight_subtask_reset();//复位sdk子任务状态量
	}
	if(smartcar_imu.imu_convergence_flag!=1&&sdk_work_mode!=26&&
	   sdk_work_mode!=27&&sdk_work_mode!=28) return;//姿态解算系统就位
	
	switch(sdk_work_mode)
	{
		case -10://初始调试模式，用于确定电机运动方向时使用
		{
			speed_ctrl_mode=0;  //直接开环输出指定PWM数值，用于调试电机方向
			motion_ctrl_pwm=motion_test_pwm_default;//默认输出百分之50占空的pwm
		}
		break;
		case -9://初始调试模式，用于确定舵机中值时使用
		{
			speed_ctrl_mode=0;  //直接开环输出指定PWM数值，用于调试电机方向
			motion_ctrl_pwm=0;//默认输出百分之50占空的pwm
			steer_servo_pwm_m1p3(trackless_motor.servo_median_value2);
		}
		break;
		case -2://调速测试模式——速度期望来源于遥控
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定
			speed_expect[0]=speed_setup;//左边轮子速度期望
			speed_expect[1]=speed_setup;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);				
		}
		break;
		case -1://调速测试模式——速度期望来源于按键设定
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			speed_expect[0]=speed_setup;//左边轮子速度期望
			speed_expect[1]=speed_setup;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);				
		}
		break;
		case 0://遥控控制
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			trackless_output.yaw_ctrl_mode=ROTATE;//偏航控制模式
			trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//偏航期望来源于横滚杆给定		
			steer_control(&turn_ctrl_pwm);
			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定	
			//期望速度
			speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//左边轮子速度期望
			speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);				
		}
		break;		
		case 1://基于灰度管的自主寻迹
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			gray_turn_control_200hz(&turn_ctrl_pwm);//基于灰度对管的转向控制
			//期望速度
			speed_expect[0]=speed_setup+turn_ctrl_pwm*turn_scale;//左边轮子速度期望
			speed_expect[1]=speed_setup-turn_ctrl_pwm*turn_scale;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);		
		}
		break;		
		case 2://2026 H task 2: follow one lap and stop at point A
		{
			auto_drive_2026_task2();
//			speed_ctrl_mode=1;//速度控制方式为两轮单独控制			
//			flight_subtask_1();
//			steer_control(&turn_ctrl_pwm);
//			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定	
//			//期望速度
//			speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//左边轮子速度期望
//			speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//右边轮子速度期望
//			//速度控制
//			speed_control_100hz(speed_ctrl_mode);		
		}
		break;
		case 3://2026 H task 3: vehicle stopped, move ball between targets
		{
			auto_drive_2026_task3();
//			speed_ctrl_mode=1;//速度控制方式为两轮单独控制			
//			flight_subtask_1();
//			steer_control(&turn_ctrl_pwm);
//			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定	
//			//期望速度
//			speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//左边轮子速度期望
//			speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//右边轮子速度期望
//			//速度控制
//			speed_control_100hz(speed_ctrl_mode);		
		}
		break;
		case 4://2026 H task 4: drive from point A to point B
		{
			auto_drive_2026_task4();
//			speed_ctrl_mode=1;//速度控制方式为两轮单独控制			
//			flight_subtask_3();
//			steer_control(&turn_ctrl_pwm);
//			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定	
//			//期望速度
//			speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//左边轮子速度期望
//			speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//右边轮子速度期望
//			//速度控制
//			speed_control_100hz(speed_ctrl_mode);		
		}
		break;
		case 5://2026 H task 5: follow one lap and stop at point A
		{
			auto_drive_2026_task5();
//			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
//			flight_subtask_4();
//			steer_control(&turn_ctrl_pwm);
//			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定	
//			//期望速度
//			speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//左边轮子速度期望
//			speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//右边轮子速度期望
//			//速度控制
//			speed_control_100hz(speed_ctrl_mode);		
		}
		break;
		case 6://2026 H task 6: follow one lap and hold captured ball position
		{
			auto_drive_2026_task6();
//			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
//			flight_subtask_5();
//			steer_control(&turn_ctrl_pwm);	
//			//期望速度
//			speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//左边轮子速度期望
//			speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//右边轮子速度期望
//			//速度控制
//			speed_control_100hz(speed_ctrl_mode);
		}
		break;		
		case 7://2026 H task 7: independent copy of task 6 for tuning
		{
			auto_drive_2026_task7();
		}
		break;
		case 8://基于两轮差速模型的速度、角速度控制,用于机载计算机ROS端发生运动指令控制下位机差速平台
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
	
			trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//偏航期望来源于横滚杆给定
			turn_ctrl_pwm=trackless_output.yaw_outer_control_output*DEG2RAD;//期望角速度转换成弧度制
			
			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定
			//期望速度
			speed_expect[0]=speed_setup-turn_ctrl_pwm*wheel_space_cm*0.5f;//左边轮子速度期望
			speed_expect[1]=speed_setup+turn_ctrl_pwm*wheel_space_cm*0.5f;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);	
		}
		break;
		case 9://地面站航点控制模式，通过无名创新地面站V1.0.6版本发布航点
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			position_control(3.0f,10);
			turn_ctrl_pwm=steer_gyro_output;
			speed_setup=distance_ctrl.output;
			//期望速度
			speed_expect[0]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//左边轮子速度期望
			speed_expect[1]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);			
		}
		break;		
		case 10://OPENMV视觉自主寻迹
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			vision_turn_control_50hz(&turn_ctrl_pwm);//基于OPENMV视觉处理的转向控制
			//期望速度
			speed_expect[0]=speed_setup+turn_ctrl_pwm*turn_scale;//左边轮子速度期望
			speed_expect[1]=speed_setup-turn_ctrl_pwm*turn_scale;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);		
		}
		break;
		case 11://双电机+前轮舵机转向遥控控制
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+RC_Data.rcdata[RC_YAW_CHANNEL]-1500);	
			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定	
			//期望速度
			speed_expect[0]=speed_setup;//左边轮子速度期望
			speed_expect[1]=speed_setup;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);	
		}	
		break;
		case 12://双电机+前轮舵机转向，视觉自主寻迹
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			vision_turn_control_50hz(&turn_ctrl_pwm);//基于OPENMV视觉处理的转向控制
			steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+turn_ctrl_pwm);	
			//期望速度
			speed_expect[0]=speed_setup;//左边轮子速度期望
			speed_expect[1]=speed_setup;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);	
		}
		break;
		case 13://倒车入库
		{
			auto_reverse_stall_park();
		}
		break;
		case 14://侧方停车
		{
			auto_parallel_park();
		}		
		break;
		case 15://2022年7月份省赛小车跟随行驶系统赛道,内外圈交替循迹
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			gray_turn_control_200hz(&turn_ctrl_pwm);//基于灰度对管的转向控制
			//期望速度
			speed_expect[0]=speed_setup+turn_ctrl_pwm*turn_scale;//左边轮子速度期望
			speed_expect[1]=speed_setup-turn_ctrl_pwm*turn_scale;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);		
		}
		break;
		case 16://2024年电赛H题第1问
		{
			auto_drive_smartcar_duty1();
		}
		break;
		case 17://2024年电赛H题第2问
		{
			auto_drive_smartcar_duty2();
		}
		break;
		case 18://2024年电赛H题第3问
		{
			auto_drive_smartcar_duty3(1);
		}
		break;
		case 19://2024年电赛H题第4问
		{
			auto_drive_smartcar_duty4(4);
		}
		break;
		case 20://2024年电赛H题发挥部分
		{
			auto_nav_point(1);
		}
		break;
		case 21://直接偏航控制-原地掉头学习案例
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制			
			flight_subtask_yaw_angle_ctrl(180);
			steer_control(&turn_ctrl_pwm);
			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定	
			//期望速度
			speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//左边轮子速度期望
			speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);	
		}
		break;
		case 27://MaixCAM debug: vehicle stopped, hold ball at center
		{
			auto_drive_2026_vision_debug();
			//auto_drive_2026_task3();
		}
		break;
		case 28://PA15 external timer gate diagnostic: force high
		{
			speed_ctrl_mode=1;
			speed_setup=0;
			speed_expect[0]=0;
			speed_expect[1]=0;
			speed_control_100hz(speed_ctrl_mode);
			Timer_Gate_Set(1U);
		}
		break;

		default:
		{
			speed_ctrl_mode=1;//速度控制方式为两轮单独控制
			trackless_output.yaw_ctrl_mode=ROTATE;//偏航控制模式
			trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//偏航期望来源于横滚杆给定		
			steer_control(&turn_ctrl_pwm);
			speed_setup=RC_Data.rc_rpyt[RC_PITCH];//速度期望来源于俯仰杆给定	
			//期望速度
			speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//左边轮子速度期望
			speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//右边轮子速度期望
			//速度控制
			speed_control_100hz(speed_ctrl_mode);			
		}
	}
}
