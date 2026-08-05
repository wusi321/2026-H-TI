/****************************************************************************************
	MSPM0G3507电赛小车开源方案资源分配表--MSPM0学习中心交流群828746221		
	功能	单片机端口	外设端口
	无名创新地面站通讯	
		PA10-->UART0-TXD	USB转TTL-RXD
		PA11-->UART0-RXD	USB转TTL-TXD
	机器视觉MaixCAM Pro (115200, 8N1)
		PA8-UART1-TXD	A18-RX
		PA9-->UART1-RXD	A19-TX
	手机蓝牙APP地面站	
		PA21-UART2-TXD	蓝牙串口模块RXD
		PA22-->UART2-RXD	蓝牙串口模块TXD
	UART3 serial bus servo (115200, 8N1)
		PB2-UART3-TXD	servo DATA (write only)
		PB3-->UART3-RXD	unused
	12路灰度传感器FPC	
	  PA31-->P1
		PA28-->P2
		PA1-->P3
		PA0-->P4
		PA25-->P5
		PA24-->P6
		PB24-->P7
		PB23-->P8
		PB19-->P9
		PB18-->P10
		PA16-->P11
		PB13-->P12
	电机控制MPWM	
		PA4-A0-PWM-CH3	  右边电机调速INA1
		PA7-->A0-PWM-CH2	右边电机调速INA2
		PA3-->A0-PWM-CH1	左边电机调速INB1
		PB14-->A0-PWM-CH0	左边电机调速INB2		
	外部计时器控制GPIO
		PA15-->TIMER_GATE  高电平启动计时，低电平停止/暂停
	舵机控制SPWM	
		PB1-->A1-PWM-CH1	预留2
		PA23-->G7-PWM-CH0	预留3
		PA2-->G7-PWM-CH1	前轮舵机转向控制PWM
	编码器测速ENC	
		PB4-RIGHT-PULSE	  右边电机脉冲倍频输出P1
		PB5-->LEFT-PULSE	左边电机脉冲倍频输出P2
		PB6-->RIGHT-DIR	  右边电机脉冲鉴相输出D1
		PB7-->LEFT-DIR	  左边电机脉冲鉴相输出D2
	外置IMU接口IMU	
		PA29-I2C-SCL	MPU6050-SCL
		PA30-->I2C-SDA	MPU6050-SDA
		PB0-->HEATER	温控IO可选
	电池电压采集	
		PA26-ADC-VBAT	需要外部分压后才允许接入
****************************************************************************************/

#include "ti_msp_dl_config.h"
#include "headfile.h"
#include "ftServo.h"
#include "ball_balance.h"

#define BLUETOOTH_TELEMETRY_DIVIDER_20HZ       (5U)
#define BLUETOOTH_BALL_DIRECTION_DEADBAND_MM_S (5.0f)
#define BLUETOOTH_VEHICLE_STARTED_CM_S          (0.05f)
/* Encoder conversion still uses the tuned 2.5 cm radius.  Feed the ball
 * controller the physical speed/acceleration for the actual 3.3 cm wheel. */
#define BALL_BALANCE_WHEEL_RADIUS_SCALE         (3.3f / 2.5f)

static float bluetooth_ball_direction(void)
{
	if (!g_ball_balance_status.vision_valid) {
		return 2.0f;
	}
	if (g_ball_balance_status.filtered_velocity_mm_s >
	    BLUETOOTH_BALL_DIRECTION_DEADBAND_MM_S) {
		return 1.0f;
	}
	if (g_ball_balance_status.filtered_velocity_mm_s <
	    -BLUETOOTH_BALL_DIRECTION_DEADBAND_MM_S) {
		return -1.0f;
	}
	return 0.0f;
}

static uint8_t bluetooth_telemetry_active_task(void)
{
	static int16_t last_work_mode=32767;
	static uint8_t active_task=0U;

	if(sdk_work_mode!=last_work_mode)
	{
		last_work_mode=sdk_work_mode;
		active_task=0U;
	}

	if(active_task!=0U) return active_task;
	if(sdk_work_mode==3&&
	   g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_3&&
	   g_auto_vision_2026_status.phase!=AUTO_VISION_2026_PHASE_IDLE&&
	   g_auto_vision_2026_status.phase!=AUTO_VISION_2026_PHASE_TASK3_WAIT_CENTER)
	{
		active_task=3U;
	}
	else if((sdk_work_mode==4||sdk_work_mode==5||sdk_work_mode==6||
	         sdk_work_mode==7)&&
	        g_auto_vision_2026_status.task_id==(uint8_t)sdk_work_mode&&
	        speed_setup>BLUETOOTH_VEHICLE_STARTED_CM_S)
	{
		active_task=(uint8_t)sdk_work_mode;
	}
	return active_task;
}


static int16_t bluetooth_telemetry_i16(float value)
{
    if (value > 32767.0f) return 32767;
    if (value < -32768.0f) return -32768;
    return (int16_t)((value >= 0.0f) ? (value + 0.5f) : (value - 0.5f));
}
int main(void)
{
  SYSCFG_DL_init();	      		//系统资源配置初始化	
	BallBalance_Init();       //Prepare MaixCAM parser before enabling UART1 IRQ
	usart_irq_config();         //串口中断配置
	ncontroller_set_priority(); //中断优先级设置	
	OLED_Init();						    //显示屏初始化
	nADC_Init();					      //ADC初始化
	nGPIO_Init();						    //蜂鸣器初始化
	w25qxx_gpio_init();         //板载w25q64初始化
	ctrl_params_init();			    //控制参数初始化
	trackless_params_init();    //硬件配置初始化
	simulation_pwm_init();      //模拟PWM初始化
	rc_range_init();				    //遥控器行程初始化
	Servo_Y_InitAtAngle(g_ball_balance_config.servo_neutral_angle_deg,
	                    g_ball_balance_config.servo_speed);
	auto_vision_2026_init();  //Hold the beam at neutral until a visual task starts
	while(ICM206xx_Init());	    //加速度计/陀螺仪初始化
	rgb_init();							    //RGB灯初始化	
	Encoder_Init();					    //编码器资源初始化
	Button_Init();					    //板载按键初始化
	PPM_Init();							    //接收机PPM信号初始化
	timer_irq_config();
  while(1)
  {
		screen_display();//屏幕显示
		adc_statemachine();//adc采集状态机
  }
}





/***************************************
函数名:	void duty_200hz(void)
说明: 200hz实时任务函数
入口:	无
出口:	无
备注:	无
作者:	无名创新
***************************************/
void maple_duty_200hz(void)
{
	rc_data_input();							 //遥控器PPM数据处理
	get_wheel_speed();					   //获取轮胎转速
	sdk_duty_run();					  		 //SDK总任务控制
	motor_output(speed_ctrl_mode); //控制器输出
	BallBalance_UpdateVehicleMotionCmps(
		BALL_BALANCE_WHEEL_RADIUS_SCALE*0.5f*
			(smartcar_imu.left_motor_speed_cmps+
			 smartcar_imu.right_motor_speed_cmps),
		(trackless_output.unlock_flag==LOCK)?0.0f:
			BALL_BALANCE_WHEEL_RADIUS_SCALE*0.5f*
				(speed_expect[0]+speed_expect[1]),
		smartcar_imu.yaw_gyro_enu);
	BallBalance_Control200Hz();     //MaixCAM ball loop updates the servo target
	Servo_Y_Loop();                 //200 Hz caller, 100 Hz UART output
	imu_data_sampling();					 //加速度计、陀螺仪数据获采集
	get_battery_voltage();				 //ADC数据获取
	trackless_ahrs_update();			 //ahrs姿态更新
	imu_temperature_ctrl();				 //传感器恒温控制
	read_button_state_all();       //按键状态读取
	battery_voltage_detection();	 //电池电压检测
  laser_light_work(&beep);       //电源板蜂鸣器驱动
	bling_working(0);							 //RGB灯状态机
}

/***************************************
函数名:	void duty_1000hz(void)
说明: 1000hz实时任务函数
入口:	无
出口:	无
备注:	无
作者:	无名创新
***************************************/
void duty_1000hz(void)
{
	if(sdk_work_mode==15)
	{
		gpio_input_check_channel_12_with_handle();//检测12路灰度灰度管状态,带赛道信息处理
	}
	else if(sdk_work_mode==2||sdk_work_mode==4||
	        sdk_work_mode==5||sdk_work_mode==6||sdk_work_mode==7)
	{
		gpio_input_check_channel_12_stop();
	}
	else
	{
		//gpio_input_check_channel_12();//检测12路灰度灰度管状态
		//gpio_input_check_channel_7();
		gpio_input_check_channel_12_2024();
	}
	gpio_input_check_from_vision();//openmv机器视觉信息获取
	simulation_pwm_output();//模拟pwm输出
}


/***************************************
函数名:	void duty_100hz(void)
说明: 100hz实时任务函数
入口:	无
出口:	无
备注:	无
作者:	无名创新
***************************************/
void duty_100hz(void)
{
	static uint8_t bluetooth_telemetry_divider = 0U;
	static uint16_t bluetooth_telemetry_sequence = 0U;
	uint8_t telemetry_task=bluetooth_telemetry_active_task();

	if(telemetry_task==0U)
	{
		bluetooth_telemetry_divider=0U;
	}
	else if(++bluetooth_telemetry_divider >=
	    BLUETOOTH_TELEMETRY_DIVIDER_20HZ) {
		BluetoothTelemetryV2 telemetry;
		uint8_t telemetry_flags = 0U;
		bluetooth_telemetry_divider = 0U;
		if (g_ball_balance_status.vision_valid)
			telemetry_flags |= BLUETOOTH_TELEMETRY_FLAG_VISION_VALID;
		if (g_ball_balance_status.controller_active)
			telemetry_flags |= BLUETOOTH_TELEMETRY_FLAG_CONTROLLER_ACTIVE;
		if (g_ball_balance_status.vehicle_feedforward_enabled)
			telemetry_flags |= BLUETOOTH_TELEMETRY_FLAG_FEEDFORWARD;
		if (g_ball_balance_status.minimum_move_active)
			telemetry_flags |= BLUETOOTH_TELEMETRY_FLAG_MINIMUM_MOVE;
		if (g_ball_balance_status.velocity_limit_active)
			telemetry_flags |= BLUETOOTH_TELEMETRY_FLAG_VELOCITY_LIMIT;
		if (g_auto_vision_2026_status.task_complete)
			telemetry_flags |= BLUETOOTH_TELEMETRY_FLAG_TASK_COMPLETE;
		if (g_auto_vision_2026_status.request_safe_level)
			telemetry_flags |= BLUETOOTH_TELEMETRY_FLAG_ROUTE_SAFE;
		telemetry.task_id = telemetry_task;
		telemetry.phase = g_auto_vision_2026_status.phase;
		telemetry.route_mode = g_auto_vision_2026_status.route_mode;
		telemetry.flags = telemetry_flags;
		telemetry.sequence = bluetooth_telemetry_sequence++;
		telemetry.ball_position_mm = bluetooth_telemetry_i16(
			g_ball_balance_status.filtered_position_mm);
		telemetry.ball_velocity_mm_s = bluetooth_telemetry_i16(
			g_ball_balance_status.filtered_velocity_mm_s);
		telemetry.target_position_mm = g_auto_vision_2026_status.target_mm;
		telemetry.vehicle_measured_speed_mm_s = bluetooth_telemetry_i16(
			g_ball_balance_status.vehicle_measured_speed_cmps * 10.0f);
		telemetry.vehicle_command_speed_mm_s = bluetooth_telemetry_i16(
			g_ball_balance_status.vehicle_command_speed_cmps * 10.0f);
		telemetry.vehicle_measured_acceleration_mm_s2 = bluetooth_telemetry_i16(
			g_ball_balance_status.vehicle_measured_acceleration_mm_s2);
		telemetry.vehicle_command_acceleration_mm_s2 = bluetooth_telemetry_i16(
			g_ball_balance_status.vehicle_command_acceleration_mm_s2);
		telemetry.left_motor_speed_mm_s = bluetooth_telemetry_i16(
			BALL_BALANCE_WHEEL_RADIUS_SCALE *
			smartcar_imu.left_motor_speed_cmps * 10.0f);
		telemetry.right_motor_speed_mm_s = bluetooth_telemetry_i16(
			BALL_BALANCE_WHEEL_RADIUS_SCALE *
			smartcar_imu.right_motor_speed_cmps * 10.0f);
		telemetry.yaw_deg_centi = bluetooth_telemetry_i16(
			smartcar_imu.rpy_deg[YAW] * 100.0f);
		telemetry.yaw_rate_dps_centi = bluetooth_telemetry_i16(
			smartcar_imu.yaw_gyro_enu * 100.0f);
		telemetry.servo_angle_deg_centi = bluetooth_telemetry_i16(
			g_ball_balance_status.servo_angle_command_deg * 100.0f);
		telemetry.servo_speed = (int16_t)g_ball_balance_status.servo_speed_command;
		telemetry.feedforward_servo_deg_centi = bluetooth_telemetry_i16(
			g_ball_balance_status.vehicle_feedforward_servo_deg * 100.0f);
		telemetry.turn_compensation_servo_deg_centi = bluetooth_telemetry_i16(
			g_ball_balance_status.vehicle_turn_compensation_servo_deg * 100.0f);
		bluetooth_app_send_v2(&telemetry);
	}
	NCLink_SEND_StateMachine();//无名创新地面站发送	
}


/***************************************
函数名:	void duty_10hz(void)
说明: 10hz实时任务函数
入口:	无
出口:	无
备注:	无
作者:	无名创新
***************************************/
void duty_10hz(void)
{
#if 0
	//手机端app地面站发送	
	bluetooth_app_send(smartcar_imu.rpy_deg[ROL],
										 smartcar_imu.rpy_deg[PIT],
										 smartcar_imu.rpy_deg[YAW],
										 speed_setup,
										 smartcar_imu.left_motor_speed_cmps,
										 smartcar_imu.right_motor_speed_cmps,
										 smartcar_imu.state_estimation.pos.x,
										 smartcar_imu.state_estimation.pos.y);
#endif
}
