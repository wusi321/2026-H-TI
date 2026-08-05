#include "headfile.h"
#include "subtask.h"
#include "ball_balance.h"

#define SUBTASK_NUM 20
#define flight_subtask_delta 5//5ms
#define AUTO_VISION_2026_STOP_BALANCE_HOLD_MS 1800U
#define AUTO_VISION_2026_STOP_BALANCE_SPEED_CMPS 2.0f



uint16_t flight_subtask_cnt[SUBTASK_NUM]={0};//�����������̼߳��������������ڿ���ÿ���������̵߳�ִ��
uint32_t flight_global_cnt[SUBTASK_NUM]={0}; //������������ȫ�ּ����������Խ��λ��ƫ�������ж��жϺ����Ƿ񵽴�
uint32_t execute_time_ms[SUBTASK_NUM]={0};//������������ִ��ʱ�䣬������������ĳ�����̵߳�ִ��ʱ��

void flight_subtask_reset(void)
{
	for(uint16_t i=0;i<SUBTASK_NUM;i++)
	{
		flight_subtask_cnt[i]=0;
		execute_time_ms[i]=0;
		flight_global_cnt[i]=0;
	}
	Timer_Gate_Set(0U);
}

void flight_subtask_reset_num(uint16_t num)
{
	flight_subtask_cnt[num]=0;
	execute_time_ms[num]=0;
	flight_global_cnt[num]=0;
}


void flight_subtask_1(void)//˳ʱ��ת90��
{
	static uint8_t n=0;
	if(flight_subtask_cnt[n]==0)
	{
		trackless_output.yaw_ctrl_mode=CLOCKWISE;
		trackless_output.yaw_ctrl_start=1;
		trackless_output.yaw_outer_control_output  =90;//˳ʱ��90��	
		flight_subtask_cnt[n]=1;		
	}
	else if(flight_subtask_cnt[n]==1)
	{
		trackless_output.yaw_ctrl_mode=CLOCKWISE;
		trackless_output.yaw_outer_control_output  =0;
		
		if(trackless_output.yaw_ctrl_end==1)  flight_subtask_cnt[n]=2;//ִ����Ϻ��л�����һ�׶�	
	}
	else if(flight_subtask_cnt[n]==2)
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
	}
	else//��������
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];		
	}
}

void flight_subtask_2(void)//��ʱ��ת90��
{
	static uint8_t n=1;
	if(flight_subtask_cnt[n]==0)
	{
		trackless_output.yaw_ctrl_mode=ANTI_CLOCKWISE;
		trackless_output.yaw_ctrl_start=1;
		trackless_output.yaw_outer_control_output  =90;//��ʱ��90��
		flight_subtask_cnt[n]=1;		
	}
	else if(flight_subtask_cnt[n]==1)
	{
		trackless_output.yaw_ctrl_mode=ANTI_CLOCKWISE;
		trackless_output.yaw_outer_control_output  =0;
		
		if(trackless_output.yaw_ctrl_end==1)  flight_subtask_cnt[n]=2;//ִ����Ϻ��л�����һ�׶�	
	}
	else if(flight_subtask_cnt[n]==2)
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
	}
	else//��������
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];		
	}
}



//��30deg/s�Ľ��ٶ�˳ʱ��ת��3000ms����ɺ���
void flight_subtask_3(void)
{
	static uint8_t n=2;
	if(flight_subtask_cnt[n]==0)
	{
		
		trackless_output.yaw_ctrl_mode=CLOCKWISE_TURN;
		trackless_output.yaw_ctrl_start=1;
		trackless_output.yaw_outer_control_output  =30;//��30deg/s�Ľ��ٶ�˳ʱ��ת��3000ms
		trackless_output.execution_time_ms=3000;//ִ��ʱ��
		flight_subtask_cnt[n]=1;		
	}
	else if(flight_subtask_cnt[n]==1)
	{
		trackless_output.yaw_ctrl_mode=CLOCKWISE_TURN;
		trackless_output.yaw_outer_control_output  =0;
		
	  if(trackless_output.yaw_ctrl_end==1)  flight_subtask_cnt[n]=2;//ִ����Ϻ��л�����һ�׶�		
	}
	else if(flight_subtask_cnt[n]==2)
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
	}
	else
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
	}
}

//��30deg/s�Ľ��ٶ���ʱ��ת��3000ms
void flight_subtask_4(void)
{
	static uint8_t n=3;
	if(flight_subtask_cnt[n]==0)
	{
		trackless_output.yaw_ctrl_mode=ANTI_CLOCKWISE_TURN;
		trackless_output.yaw_ctrl_start=1;
		trackless_output.yaw_outer_control_output  =30;//��30deg/s�Ľ��ٶ�˳ʱ��ת��3000ms
		trackless_output.execution_time_ms=3000;//ִ��ʱ��
		flight_subtask_cnt[n]=1;		
	}
	else if(flight_subtask_cnt[n]==1)
	{
		trackless_output.yaw_ctrl_mode=ANTI_CLOCKWISE_TURN;
		trackless_output.yaw_outer_control_output  =0;
		
	  if(trackless_output.yaw_ctrl_end==1)  flight_subtask_cnt[n]=2;//ִ����Ϻ��л�����һ�׶�		
	}
	else if(flight_subtask_cnt[n]==2)
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
	}
	else
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
	}
}



void flight_subtask_5(void)
{
	static uint8_t n=4;
	if(flight_subtask_cnt[n]==0)
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
		speed_setup=40;//ǰ��
		if(rangefinder.distance<50)//ǰ������Ƚ�С������ת��
		{
			speed_setup=0;//ֹͣ
			flight_subtask_cnt[n]=1;	
		}			
	}
	else if(flight_subtask_cnt[n]==1)
	{
		trackless_output.yaw_ctrl_mode=CLOCKWISE;
		trackless_output.yaw_ctrl_start=1;
		trackless_output.yaw_outer_control_output  =90;//˳ʱ��90��
		
		speed_setup=0;//ֹͣ		
		flight_subtask_cnt[n]=2;		
	}
	else if(flight_subtask_cnt[n]==2)
	{
		trackless_output.yaw_ctrl_mode=CLOCKWISE;
		trackless_output.yaw_outer_control_output  =0;
		
		if(trackless_output.yaw_ctrl_end==1)  flight_subtask_cnt[n]=3;//ִ����Ϻ��л�����һ�׶�	
	}
	else if(flight_subtask_cnt[n]==3)//ת����Ϻ󣬼����ж�ǰ������
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
		if(rangefinder.distance<50)//ǰ������Ƚ�С������ת��
		{
			flight_subtask_cnt[n]=1;//����ת��	
		}
		else
		{
			flight_subtask_cnt[n]=0;//�ָ�ǰ��	
		}
	}
	else//��������
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
		speed_setup=RC_Data.rc_rpyt[RC_PITCH];//�ٶ�������Դ�ڸ����˸���
	}
} 


#define start_point_precision_cm 0.5f
#define distance_precision_cm 1.0f
#define steer_execute_time_ms 500
#define steer_value_default 300
#define freedom_time_ms 5000					//�ڳ�������ͣ��ʱ��



_park_params park_params={
	._track_speed_cmps=track_speed_cmps_default,
	._start_point_adjust1=start_point_adjust1_default,
	._forward_distance_cm=forward_distance_cm_default,
	._backward_distance1_cm=backward_distance1_cm_default,
	._backward_distance2_cm=backward_distance2_cm_default,
	._out_forward_distance1_cm=out_forward_distance1_cm_default,
	._out_forward_distance2_cm=out_forward_distance2_cm_default,
	._start_point_adjust2=start_point_adjust2_default,
	._parallel_backward_distance1_cm=parallel_backward_distance1_cm_default,
	._parallel_backward_distance2_cm=parallel_backward_distance2_cm_default,
	._parallel_backward_distance3_cm=parallel_backward_distance3_cm_default
};


void auto_reverse_stall_park(void)
{
	static uint8_t n=5;
	static float steer_gradient_cnt=0;
	static float servo_ctrl_value=300;
	if(flight_subtask_cnt[n]==0)//��һ�׶�����Ѱ��
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		vision_turn_control_50hz(&turn_ctrl_pwm);//����OPENMV�Ӿ�������ת�����
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+turn_ctrl_pwm);	
		//�����ٶ�
		speed_expect[0]=park_params._track_speed_cmps;//��������ٶ�����
		speed_expect[1]=park_params._track_speed_cmps;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);

		//�ж�;������������
		if(camera1.carpark_num==3)//������2
		{
			flight_subtask_cnt[n]++;
			//����ǰ��5cm
			distance_ctrl.expect=smartcar_imu.state_estimation.distance+park_params._start_point_adjust1;			
		}
	}
	else if(flight_subtask_cnt[n]==1)//�Ӿ�ʶ�𵽳����������,ִ�м���ǰ�����ߺ���
	{
		if(park_params._start_point_adjust1>0)//�������ǰ������ʼ�����㣬�����Ѱ��
		{
			vision_turn_control_50hz(&turn_ctrl_pwm);//����OPENMV�Ӿ�������ת�����
		}
		else turn_ctrl_pwm=0;//��̥�������������
		
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+turn_ctrl_pwm);	
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		if(flight_global_cnt[n]<20)//����20������λ��ƫ���С,����Ϊλ�ÿ������
		{
			if(ABS(distance_ctrl.error)<start_point_precision_cm)	flight_global_cnt[n]++;	
			else flight_global_cnt[n]/=2;			
		}
		else 
		{
			flight_global_cnt[n]=0;
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=-steer_value_default;
			steer_gradient_cnt=execute_time_ms[n];
		}
	}
	else if(flight_subtask_cnt[n]==2)//�ڶ��׶�ͣ����,�������
	{
		float steer_gradient_value=(float)((steer_gradient_cnt-execute_time_ms[n])/steer_gradient_cnt);
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value*steer_gradient_value);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			distance_ctrl.expect=smartcar_imu.state_estimation.distance+park_params._forward_distance_cm;
		}		
	}
	else if(flight_subtask_cnt[n]==3)//�����׶�
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=steer_value_default;
			//�����ٶ�
			//speed_expect[0]=0;//��������ٶ�����
			//speed_expect[1]=0;//�ұ������ٶ�����
			steer_gradient_cnt=execute_time_ms[n];	
		}
	}
	else	if(flight_subtask_cnt[n]==4)//���Ľ׶�
	{
		float steer_gradient_value0_1=(float)((steer_gradient_cnt-execute_time_ms[n])/steer_gradient_cnt);//��0���䵽1
		float steer_gradient_value_n1_p1=(steer_gradient_value0_1-0.5f)/0.5f;//��-1���䵽1
		
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value*steer_gradient_value_n1_p1);
		
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
	  //�����ٶ�
		speed_expect[0]=0;//��������ٶ�����
		speed_expect[1]=0;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			distance_ctrl.expect=smartcar_imu.state_estimation.distance-park_params._backward_distance1_cm;
		}		
	}
	else if(flight_subtask_cnt[n]==5)//����׶�
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=steer_value_default;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			steer_gradient_cnt=execute_time_ms[n];		
		}
	}
	else	if(flight_subtask_cnt[n]==6)//�����׶�
	{
		float steer_gradient_value0_1=(float)((steer_gradient_cnt-execute_time_ms[n])/steer_gradient_cnt);//��0���䵽1
		float steer_gradient_value_1_0=1.0f-steer_gradient_value0_1;//��1���䵽0
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value*steer_gradient_value_1_0);
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			distance_ctrl.expect=smartcar_imu.state_estimation.distance-park_params._backward_distance2_cm;
		}		
	}
	else if(flight_subtask_cnt[n]==7)//����׶�
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=freedom_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=0;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����		
		}
	}
	else if(flight_subtask_cnt[n]==8)//�ڳ�βͣ��5S
	{
		//ת�����
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2);
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			distance_ctrl.expect=smartcar_imu.state_estimation.distance+park_params._out_forward_distance1_cm;
		}		
	}
	else if(flight_subtask_cnt[n]==9)//������ǰ��
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=300;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����		
		}	
	}
	else if(flight_subtask_cnt[n]==10)
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value);
		speed_control_100hz(speed_ctrl_mode);
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			distance_ctrl.expect=smartcar_imu.state_estimation.distance+park_params._out_forward_distance2_cm;
		}		
	}
	else if(flight_subtask_cnt[n]==11)//����ת��
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=0;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����		
		}	
	}
	else if(flight_subtask_cnt[n]==12)//����������������һ����
	{
		for(uint16_t i=0;i<4;i++)
		{
			camera1.carpark_flag[0][i]=0;
			camera1.carpark_flag[1][i]=0;
		}
		camera1.carpark_num=0;
		sdk_work_mode+=1;
	}
	else
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2);
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);	
	}
}



void auto_parallel_park(void)
{
	static uint8_t n=6;	
	static float steer_gradient_cnt=0;
	static float servo_ctrl_value=300;
	
	if(flight_subtask_cnt[n]==0)//��һ�׶�����Ѱ��
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		vision_turn_control_50hz(&turn_ctrl_pwm);//����OPENMV�Ӿ�������ת�����
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+turn_ctrl_pwm);	
		//�����ٶ�
		speed_expect[0]=park_params._track_speed_cmps;//��������ٶ�����
		speed_expect[1]=park_params._track_speed_cmps;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);

		//�ж�;������������
		if(camera1.carpark_num==3)//������2
		{
			flight_subtask_cnt[n]++;
			//����ǰ��5cm
			distance_ctrl.expect=smartcar_imu.state_estimation.distance+park_params._start_point_adjust2;			
		}
	}
	else if(flight_subtask_cnt[n]==1)//ִ�м���ǰ��10cm
	{
		if(park_params._start_point_adjust2>0)//�������ǰ������ʼ�����㣬�����Ѱ��
		{
			vision_turn_control_50hz(&turn_ctrl_pwm);//����OPENMV�Ӿ�������ת�����
		}
		else turn_ctrl_pwm=0;//��̥�������������
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+turn_ctrl_pwm);	
		
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		if(flight_global_cnt[n]<20)
		{
			if(ABS(distance_ctrl.error)<start_point_precision_cm)	flight_global_cnt[n]++;	
			else flight_global_cnt[n]/=2;			
		}
		else 
		{
			flight_global_cnt[n]=0;
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=steer_value_default;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			steer_gradient_cnt=execute_time_ms[n];
		}
	}
	else if(flight_subtask_cnt[n]==2)//�ڶ��׶�ɲ����ת��
	{
		float steer_gradient_value=(float)((steer_gradient_cnt-execute_time_ms[n])/steer_gradient_cnt);
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value*steer_gradient_value);
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			distance_ctrl.expect=smartcar_imu.state_estimation.distance-park_params._parallel_backward_distance1_cm;
		}		
	}
	else if(flight_subtask_cnt[n]==3)//�����׶�
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=steer_value_default;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			steer_gradient_cnt=execute_time_ms[n];	
		}
	}
	else if(flight_subtask_cnt[n]==4)//��ͷ����
	{
		float steer_gradient_value0_1=(float)((steer_gradient_cnt-execute_time_ms[n])/steer_gradient_cnt);//��0���䵽1
		float steer_gradient_value_1_0=1.0f-steer_gradient_value0_1;//��1���䵽0
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value*steer_gradient_value_1_0);
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			distance_ctrl.expect=smartcar_imu.state_estimation.distance-park_params._parallel_backward_distance2_cm;
		}			
	}
	else if(flight_subtask_cnt[n]==5)//��ֱ����
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=-steer_value_default;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			steer_gradient_cnt=execute_time_ms[n];	
		}	
	}
	else if(flight_subtask_cnt[n]==6)//����������
	{
		float steer_gradient_value=(float)((steer_gradient_cnt-execute_time_ms[n])/steer_gradient_cnt);
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value*steer_gradient_value);
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			distance_ctrl.expect=smartcar_imu.state_estimation.distance-park_params._parallel_backward_distance3_cm;
		}		
	}
	else if(flight_subtask_cnt[n]==7)//���������򲢺���
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=freedom_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=0;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
		}		
	}
	else if(flight_subtask_cnt[n]==8)//����ֱ�ӻ���,ԭ�صȴ�5s
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2);
		speed_setup=0;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=-steer_value_default;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			steer_gradient_cnt=execute_time_ms[n];
		}	
	}
	else if(flight_subtask_cnt[n]==9)//��̥�����׼������
	{
		float steer_gradient_value=(float)((steer_gradient_cnt-execute_time_ms[n])/steer_gradient_cnt);
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value*steer_gradient_value);	
		//�����ٶ�
		speed_expect[0]=0;//��������ٶ�����
		speed_expect[1]=0;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);	
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=-steer_value_default;	
			distance_ctrl.expect=smartcar_imu.state_estimation.distance+park_params._parallel_backward_distance3_cm;
		}		
	}
	else if(flight_subtask_cnt[n]==10)//����������ǰ��
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=-steer_value_default;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			steer_gradient_cnt=execute_time_ms[n];
		}		
	}
	else if(flight_subtask_cnt[n]==11)//��̥�𽥻��������ǰ��
	{
		float steer_gradient_value0_1=(float)((steer_gradient_cnt-execute_time_ms[n])/steer_gradient_cnt);//��0���䵽1
		float steer_gradient_value_1_0=1.0f-steer_gradient_value0_1;//��1���䵽0
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value*steer_gradient_value_1_0);
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			distance_ctrl.expect=smartcar_imu.state_estimation.distance+park_params._parallel_backward_distance2_cm;
		}			
	}
	else if(flight_subtask_cnt[n]==12)//���������ǰ��
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=steer_value_default;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			steer_gradient_cnt=execute_time_ms[n];
		}		
	}
	else if(flight_subtask_cnt[n]==13)//��̥���Ҵ�
	{
		float steer_gradient_value=(float)((steer_gradient_cnt-execute_time_ms[n])/steer_gradient_cnt);
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value*steer_gradient_value);	
		//�����ٶ�
		speed_expect[0]=0;//��������ٶ�����
		speed_expect[1]=0;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);	
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=steer_execute_time_ms/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=steer_value_default;	
			distance_ctrl.expect=smartcar_imu.state_estimation.distance+park_params._parallel_backward_distance1_cm;
		}		
	}
	else if(flight_subtask_cnt[n]==14)//��̥���Ҵ�����ǰ��
	{
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+servo_ctrl_value);
		//�������
		distance_control();
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup;//��������ٶ�����
		speed_expect[1]=speed_setup;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			execute_time_ms[n]=3000/flight_subtask_delta;//������ִ��ʱ��;
			servo_ctrl_value=0;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
		}		
	}
	else if(flight_subtask_cnt[n]==15)//����Ѳ��
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		vision_turn_control_50hz(&turn_ctrl_pwm);//����OPENMV�Ӿ�������ת�����
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2+turn_ctrl_pwm);	
		//�����ٶ�
		speed_expect[0]=park_params._track_speed_cmps;//��������ٶ�����
		speed_expect[1]=park_params._track_speed_cmps;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);	
		
		if(execute_time_ms[n]>0) execute_time_ms[n]--;
		if(execute_time_ms[n]==0) 
		{
			flight_subtask_cnt[n]++;
			servo_ctrl_value=0;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
		}	
	}
	else//Ѳ�߽�����ֹͣ
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		steer_servo_pwm_m1p3(trackless_motor.servo_median_value2);
		//�ٶȿ���
		speed_expect[0]=0;//��������ٶ�����
		speed_expect[1]=0;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);		
	}
}



//2024�����H��-�Զ���ʻС��demo
uint8_t flight_subtask_yaw_angle_ctrl(float target_angle)//���Ժ���Ƕȿ���,������Ϊ��ʱ�뷽����ת0~360
{
	uint8_t finish_flag=0;
	static uint8_t n=7;
	if(flight_subtask_cnt[n]==0)
	{
		trackless_output.yaw_ctrl_mode=AZIMUTH;
		trackless_output.yaw_ctrl_start=1;
		trackless_output.yaw_outer_control_output  =target_angle;//�����Ƕ�
		flight_subtask_cnt[n]=1;		
	}
	else if(flight_subtask_cnt[n]==1)
	{
		trackless_output.yaw_ctrl_mode=AZIMUTH;
		trackless_output.yaw_outer_control_output  =0;
		
		if(trackless_output.yaw_ctrl_end==1)  flight_subtask_cnt[n]=2;//ִ����Ϻ��л�����һ�׶�	
	}
	else if(flight_subtask_cnt[n]==2)
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];
		finish_flag=1;
	}
	else//��������
	{
		trackless_output.yaw_ctrl_mode=ROTATE;
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];		
	}
	return finish_flag;
}




void auto_drive_smartcar_duty1(void)
{
	static uint8_t n=8;	
	if(flight_subtask_cnt[n]==0)
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		flight_subtask_cnt[n]=1;		
	}
	else if(flight_subtask_cnt[n]==1)
	{
		distance_ctrl.expect=smartcar_imu.state_estimation.distance+100;
		flight_subtask_cnt[n]=2;
	}
	else if(flight_subtask_cnt[n]==2)
	{
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		
		//�������
		distance_control_with_speed_limit(50);
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);

		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
		}	
	}
	else
	{
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);				
	}
}
	 


//#define self_guided_tracking_speed 60//60
//#define distance_ctrl_speed_max 80   //50
//#define move_diagonal_angle    35 //38.65
//#define move_diagonal_distance 125//128

#define speed_zero_check 1.0f
float _distance_ctrl_speed_max,_self_guided_tracking_speed,_move_diagonal_angle1,_move_diagonal_distance1,_move_diagonal_angle2,_move_diagonal_distance2;
void auto_drive_smartcar_duty2(void)
{
	static uint8_t n=9;	
	if(flight_subtask_cnt[n]==0)
	{
		//1���������
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		flight_subtask_cnt[n]=1;		
	}
	else if(flight_subtask_cnt[n]==1)
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		//������������
		distance_ctrl.expect=smartcar_imu.state_estimation.distance+100;
		//�������
		distance_control_with_speed_limit(_distance_ctrl_speed_max);
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		flight_subtask_cnt[n]=2;
	}
	else if(flight_subtask_cnt[n]==2)
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
			
		distance_control_with_speed_limit(_distance_ctrl_speed_max);//�������
		speed_setup=distance_ctrl.output;//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		//ʵʱ�жϾ�������Ƿ�����
		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
		}	
	}
	else if(flight_subtask_cnt[n]==3)//��ǰ���ٶȿ��Ƶ�0��Ϊת�������׼��
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);

		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
			
		if(ABS(speed_error[0])<speed_zero_check&&ABS(speed_error[1])<speed_zero_check)//�ٶȿ������
		{
			flight_subtask_cnt[n]++;
			road_miss_flag=0;
			road_miss_cnt=0;			
		}
	}
	else if(flight_subtask_cnt[n]==4)
	{
		gray_turn_control_200hz(&turn_ctrl_pwm);//���ڻҶȶԹܵ�ת�����
		
		speed_setup=_self_guided_tracking_speed;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*turn_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*turn_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);	

		if(road_miss_flag==1)//�Ҷȴ��������ߣ����׶�ѭ����� 
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
		}
	}
	else if(flight_subtask_cnt[n]==5)//��ǰ���ٶȿ��Ƶ�0��Ϊת�������׼��
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);

		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
			
		if(ABS(speed_error[0])<speed_zero_check&&ABS(speed_error[1])<speed_zero_check)//�ٶȿ������
		{
			flight_subtask_cnt[n]++;	
		}
	}
	else if(flight_subtask_cnt[n]==6)
	{
		if(flight_subtask_yaw_angle_ctrl(180))//�жϺ�����Ƿ�ִ����� 
		{
			flight_subtask_cnt[n]++;//�̼߳������Լ�
			flight_subtask_reset_num(7);//��λ
		}
		steer_control(&turn_ctrl_pwm);
		speed_setup=RC_Data.rc_rpyt[RC_PITCH];//�ٶ�������Դ�ڸ����˸���	
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);			
	}
	else if(flight_subtask_cnt[n]==7)
	{		
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		//������������
		distance_ctrl.expect=smartcar_imu.state_estimation.distance+100;
		//�������
		distance_control_with_speed_limit(_distance_ctrl_speed_max);
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		flight_subtask_cnt[n]++;
	}
	else if(flight_subtask_cnt[n]==8)
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
			
		distance_control_with_speed_limit(_distance_ctrl_speed_max);//�������
		speed_setup=distance_ctrl.output;//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		//ʵʱ�жϾ�������Ƿ�����
		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
			road_miss_flag=0;
			road_miss_cnt=0;
		}	
	}
	else if(flight_subtask_cnt[n]==9)
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		gray_turn_control_200hz(&turn_ctrl_pwm);//���ڻҶȶԹܵ�ת�����
		
		speed_setup=_self_guided_tracking_speed;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*turn_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*turn_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);	

		if(road_miss_flag==1)//�Ҷȴ��������ߣ����׶�ѭ����� 
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
		}
	}
	else if(flight_subtask_cnt[n]==10)//��ǰ���ٶȿ��Ƶ�0
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);

		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
	}
	else
	{
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);				
	}
}
	 

void auto_drive_smartcar_duty3(uint16_t times)
{
	static uint8_t n=10;	
	if(flight_subtask_cnt[n]==0)
	{
		//1���������
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		flight_subtask_cnt[n]=1;		
	}
	else if(flight_subtask_cnt[n]==1)
	{
		if(flight_subtask_yaw_angle_ctrl(-_move_diagonal_angle1))//�жϺ�����Ƿ�ִ����� 
		{
			flight_subtask_cnt[n]++;//�̼߳������Լ�
			flight_subtask_reset_num(7);//��λ
		}
		steer_control(&turn_ctrl_pwm);
		speed_setup=RC_Data.rc_rpyt[RC_PITCH];//�ٶ�������Դ�ڸ����˸���	
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);			
	}
	else if(flight_subtask_cnt[n]==2)
	{		
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		//������������
		distance_ctrl.expect=smartcar_imu.state_estimation.distance+_move_diagonal_distance1;
		//�������
		distance_control_with_speed_limit(_distance_ctrl_speed_max);
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		flight_subtask_cnt[n]++;
	}
	else if(flight_subtask_cnt[n]==3)
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
			
		distance_control_with_speed_limit(_distance_ctrl_speed_max);//�������
		speed_setup=distance_ctrl.output;//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		//ʵʱ�жϾ�������Ƿ�����
		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
			road_miss_flag=0;
			road_miss_cnt=0;
		}	
	}
	else if(flight_subtask_cnt[n]==4)
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		gray_turn_control_200hz(&turn_ctrl_pwm);//���ڻҶȶԹܵ�ת�����
		
		speed_setup=_self_guided_tracking_speed;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*turn_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*turn_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);	

		if(road_miss_flag==1)//�Ҷȴ��������ߣ����׶�ѭ����� 
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
		}
	}
	else if(flight_subtask_cnt[n]==5)//��ǰ���ٶȿ��Ƶ�0��Ϊת�������׼��
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);

		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
			
		if(ABS(speed_error[0])<speed_zero_check&&ABS(speed_error[1])<speed_zero_check)//�ٶȿ������
		{
			flight_subtask_cnt[n]++;	
		}
	}
	else if(flight_subtask_cnt[n]==6)
	{
		if(flight_subtask_yaw_angle_ctrl(-180+_move_diagonal_angle2))//�жϺ�����Ƿ�ִ����� 
		{
			flight_subtask_cnt[n]++;//�̼߳������Լ�
			flight_subtask_reset_num(7);//��λ
		}
		steer_control(&turn_ctrl_pwm);
		speed_setup=RC_Data.rc_rpyt[RC_PITCH];//�ٶ�������Դ�ڸ����˸���	
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);			
	}
	else if(flight_subtask_cnt[n]==7)
	{		
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		//������������
		distance_ctrl.expect=smartcar_imu.state_estimation.distance+_move_diagonal_distance2;
		//�������
		distance_control_with_speed_limit(_distance_ctrl_speed_max);
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		flight_subtask_cnt[n]++;
	}
	else if(flight_subtask_cnt[n]==8)
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
			
		distance_control_with_speed_limit(_distance_ctrl_speed_max);//�������
		speed_setup=distance_ctrl.output;//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		//ʵʱ�жϾ�������Ƿ�����
		if(ABS(distance_ctrl.error)<distance_precision_cm)
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
			road_miss_flag=0;
			road_miss_cnt=0;
		}	
	}
	else if(flight_subtask_cnt[n]==9)
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		gray_turn_control_200hz(&turn_ctrl_pwm);//���ڻҶȶԹܵ�ת�����
		
		speed_setup=_self_guided_tracking_speed;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*turn_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*turn_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);	

		if(road_miss_flag==1)//�Ҷȴ��������ߣ����׶�ѭ����� 
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
		}
	}
	else if(flight_subtask_cnt[n]==10)//��ǰ���ٶȿ��Ƶ�0��Ϊת�������׼��
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);

		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
			
		if(ABS(speed_error[0])<speed_zero_check&&ABS(speed_error[1])<speed_zero_check)//�ٶȿ������
		{
			flight_subtask_cnt[n]++;	
		}
	}
	else if(flight_subtask_cnt[n]==11)//�ظ�4Ȧ 
	{
			flight_global_cnt[n]++;
			if(flight_global_cnt[n]<times)	 flight_subtask_cnt[n]=1;//�ظ�ִ��4��
			else flight_subtask_cnt[n]++;
	}
	else
	{
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);		
	}
}




void auto_drive_smartcar_duty4(uint16_t times)
{
	static uint8_t n=10;	
	if(flight_subtask_cnt[n]==0)
	{
		//1���������
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
		flight_subtask_cnt[n]=1;		
	}
	else if(flight_subtask_cnt[n]==1)
	{
		if(flight_subtask_yaw_angle_ctrl(-_move_diagonal_angle1))//�жϺ�����Ƿ�ִ����� 
		{
			flight_subtask_cnt[n]++;//�̼߳������Լ�
			flight_subtask_reset_num(7);//��λ
		}
		steer_control(&turn_ctrl_pwm);
		speed_setup=RC_Data.rc_rpyt[RC_PITCH];//�ٶ�������Դ�ڸ����˸���	
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);			
	}
	else if(flight_subtask_cnt[n]==2)
	{		
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		//������������
		distance_ctrl.expect=smartcar_imu.state_estimation.distance+_move_diagonal_distance1;
		//�������
		distance_control_with_speed_limit(_distance_ctrl_speed_max);
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		flight_subtask_cnt[n]++;
	}
	else if(flight_subtask_cnt[n]==3)
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
			
		distance_control_with_speed_limit(_distance_ctrl_speed_max);//�������
		speed_setup=distance_ctrl.output;//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		//ʵʱ�жϾ�������Ƿ�����
		if(ABS(distance_ctrl.error)<distance_precision_cm||road_restore_flag==1)
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
			road_miss_flag=0;
			road_miss_cnt=0;
		}	
	}
	else if(flight_subtask_cnt[n]==4)
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		gray_turn_control_200hz(&turn_ctrl_pwm);//���ڻҶȶԹܵ�ת�����
		
		speed_setup=_self_guided_tracking_speed;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*turn_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*turn_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);	

		if(road_miss_flag==1)//�Ҷȴ��������ߣ����׶�ѭ����� 
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
		}
	}
	else if(flight_subtask_cnt[n]==5)//��ǰ���ٶȿ��Ƶ�0��Ϊת�������׼��
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);

		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
			
		if(ABS(speed_error[0])<speed_zero_check&&ABS(speed_error[1])<speed_zero_check)//�ٶȿ������
		{
			flight_subtask_cnt[n]++;	
		}
	}
	else if(flight_subtask_cnt[n]==6)
	{
		if(flight_subtask_yaw_angle_ctrl(-180+_move_diagonal_angle2))//�жϺ�����Ƿ�ִ����� 
		{
			flight_subtask_cnt[n]++;//�̼߳������Լ�
			flight_subtask_reset_num(7);//��λ
		}
		steer_control(&turn_ctrl_pwm);
		speed_setup=RC_Data.rc_rpyt[RC_PITCH];//�ٶ�������Դ�ڸ����˸���	
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);			
	}
	else if(flight_subtask_cnt[n]==7)
	{		
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		//������������
		distance_ctrl.expect=smartcar_imu.state_estimation.distance+_move_diagonal_distance2;
		//�������
		distance_control_with_speed_limit(_distance_ctrl_speed_max);
		speed_setup=distance_ctrl.output;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		flight_subtask_cnt[n]++;
	}
	else if(flight_subtask_cnt[n]==8)
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
			
		distance_control_with_speed_limit(_distance_ctrl_speed_max);//�������
		speed_setup=distance_ctrl.output;//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		speed_control_100hz(speed_ctrl_mode);
		
		//ʵʱ�жϾ�������Ƿ�����
		if(ABS(distance_ctrl.error)<distance_precision_cm||road_restore_flag==1)
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
			road_miss_flag=0;
			road_miss_cnt=0;
		}	
	}
	else if(flight_subtask_cnt[n]==9)
	{
		speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
		gray_turn_control_200hz(&turn_ctrl_pwm);//���ڻҶȶԹܵ�ת�����
		
		speed_setup=_self_guided_tracking_speed;
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*turn_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*turn_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);	

		if(road_miss_flag==1)//�Ҷȴ��������ߣ����׶�ѭ����� 
		{
			flight_subtask_cnt[n]++;
			//�����ٶ�
			speed_expect[0]=0;//��������ٶ�����
			speed_expect[1]=0;//�ұ������ٶ�����
			
			//ִ����Ϻ�,������ǰƫ����
			steer_angle_expect=smartcar_imu.rpy_deg[_YAW];
						
			bling_set(&light_red  ,2000,500,0.5,0,0);//��ɫ
			beep.reset = 1;
			beep.times = 2;
		}
	}
	else if(flight_subtask_cnt[n]==10)//��ǰ���ٶȿ��Ƶ�0��Ϊת�������׼��
	{
		//1���������
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);

		//2���ٶȿ���-�����˺���
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);
			
		if(ABS(speed_error[0])<speed_zero_check&&ABS(speed_error[1])<speed_zero_check)//�ٶȿ������
		{
			flight_subtask_cnt[n]++;	
		}
	}
	else if(flight_subtask_cnt[n]==11)//�ظ�timesȦ 
	{
			flight_global_cnt[n]++;
			if(flight_global_cnt[n]<times)	 flight_subtask_cnt[n]=1;//�ظ�ִ��4��
			else flight_subtask_cnt[n]++;
	}
	else
	{
		trackless_output.yaw_ctrl_mode=ROTATE;//ƫ������ģʽ
		trackless_output.yaw_outer_control_output  =RC_Data.rc_rpyt[RC_ROLL];//ƫ��������Դ�ں���˸���		
		steer_control(&turn_ctrl_pwm);
		speed_setup=0;//�ٶ�������0
		//�����ٶ�
		speed_expect[0]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
		speed_expect[1]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
		//�ٶȿ���
		speed_control_100hz(speed_ctrl_mode);		
	}
}


//�յ�Э����������ϵͳ

void beep_notify(void)
{
	beep.period=200;//200*5ms
	beep.light_on_percent=0.5f;			
	beep.reset=1;
	beep.times=1;		
}

void firetruck_nav_ctrl(float fixed_threshold_cm,uint16_t feed_times)
{
	speed_ctrl_mode=1;//�ٶȿ��Ʒ�ʽΪ���ֵ�������
	position_control(fixed_threshold_cm,feed_times);
	turn_ctrl_pwm=steer_gyro_output;
	speed_setup=distance_ctrl.output;
	//�����ٶ�
	speed_expect[0]=speed_setup-turn_ctrl_pwm*steer_gyro_scale;//��������ٶ�����
	speed_expect[1]=speed_setup+turn_ctrl_pwm*steer_gyro_scale;//�ұ������ٶ�����
	//�ٶȿ���
	speed_control_100hz(speed_ctrl_mode);
}


/************************************************
	D     E     F
		G2  G3
	A     B     C
U	G	G1
************************************************/
const int16_t nav_point[30][3]=
{
	{0,10,0},  //A'
	{40,50,0}, //O
	{80,90,0}, //C'
	{80,100,1},//C
	{77,115,0},
	{68,128,0},//G
	{55,137,0},
	{40,140,0},//E
	{25,137,0},
	{12,128,0},//H
	{3,115,0},
	{0,100,1}, //B
	{0,90,0},  //B'
	{40,50,0}, //O
	{80,10,0}, //D'
	{80,0,1},  //D
	{77,-15,0},
	{68,-28,0},//I
	{55,-37,0},
	{40,-40,0},//F
	{25,-37,0},
	{12,-28,0},//J
	{3,-15,0},
	{0,0,1}    //A
};


void auto_nav_point(uint16_t times)
{
	static uint8_t n=7;
	static uint16_t _times=0;
	if(flight_subtask_cnt[n]==0)
	{
		firetruck_nav_ctrl(5.0f,5);//�����ĵ�������
		//������ֵ��ֵ��������ر���
		ngs_nav_ctrl.update_flag=1;
		ngs_nav_ctrl.x=0;//����ƫ��x
		ngs_nav_ctrl.y=0;//����ƫ��y
		ngs_nav_ctrl.ctrl_finish_flag=0;
		flight_subtask_cnt[n]++;
	}
	else if(flight_subtask_cnt[n]==1)
	{
		firetruck_nav_ctrl(5.0f,1);//�����ĵ�������
		if(ngs_nav_ctrl.ctrl_finish_flag==1)//����������
		{
			if(nav_point[flight_global_cnt[n]][2]==1)	beep_notify();//�ж��Ƿ���Ҫ��������ʾ	
			if(flight_global_cnt[n]>23)	flight_subtask_cnt[n]++;
			else
			{
				//��������±�־λ������ֵ��ֵ��������ر���
				ngs_nav_ctrl.update_flag=1;
				ngs_nav_ctrl.x=nav_point[flight_global_cnt[n]][0];//����ƫ��x
				ngs_nav_ctrl.y=nav_point[flight_global_cnt[n]][1];//����ƫ��y
				ngs_nav_ctrl.ctrl_finish_flag=0;			
				flight_global_cnt[n]++;			
			}
		}
	}
	else
	{
		_times++;
		if(_times<times)
		{
			flight_subtask_cnt[n]=0;
			flight_global_cnt[n]=0;
		}	
		firetruck_nav_ctrl(5.0f,5);//�����ĵ�������
	}
}
static void auto_drive_2026_gray_set_speed(float base_speed_cmps,
	float steer_blend);
#define TASK2_TRACKING_SPEED_CMPS 40.0f
static uint8_t auto_drive_2026_lap_stop_gray(uint8_t n,
	float tracking_speed_cmps,uint8_t use_launch_ramp,
	uint8_t use_stop_ramp);

void auto_drive_2026_task2(void)
{
	/* Task 2 is scored against a 20 s lap limit and carries no ball-control
	 * requirement. Start at the configured tracking speed immediately. */
	(void)auto_drive_2026_lap_stop_gray(11,
		TASK2_TRACKING_SPEED_CMPS,0U,0U);
}

static void auto_drive_2026_hold_still(void)
{
	BallBalance_SetVehicleBraking(false);
	speed_ctrl_mode=1;
	turn_ctrl_pwm=0;
	speed_setup=0;
	speed_expect[0]=0;
	speed_expect[1]=0;
	speed_control_100hz(speed_ctrl_mode);
}

void auto_drive_2026_task3(void)
{
	auto_drive_2026_hold_still();
	auto_vision_2026_task3();
}

#define TASK4_B_YAW_CHANGE_DEG         7.0f
#define TASK4_B_GRAY_THRESHOLD         1.0f
#define TASK4_B_DETECT_CONFIRM_COUNT  6
#define TASK4_TRACKING_SPEED_CMPS      35.0f
#define TASK4_B_SENSOR_ENABLE_MS       4800U
#define TASK4_B_FORCE_DECEL_MS         5250U
#define TASK4_MIN_START_SPEED_CMPS     8.0f
#define TASK4_STOP_COMPLETE_SPEED_CMPS 3.0f
#define TASK4_POST_BREAKAWAY_INTEGRAL  150.0f
#define TASK4_BRAKING_TARGET_MM        5
/*
 * The 1500 ms ramp still produced a measured 0 -> 35 cm/s launch and drove
 * the ball as far as -6.4 cm in repeated task45 runs.  Keep accelerating
 * after the wheels begin moving so the servo does not stay at -40 degrees.
 */
#define TASK4_START_RAMP_MS           3000U
#define TASK_LAP_START_RAMP_MS        3000U
#define TASK_STOP_DECEL_MS            3000U
#define TASK4_STOP_DECEL_MS           3300U
#define TASK_STOP_COMPLETE_SPEED_CMPS 0.5f
#define TASK_LAP_MIN_START_SPEED_CMPS 5.0f
#define TASK_LAUNCH_DETECT_SPEED_CMPS 1.0f
#define TASK7_LAP_SUBTASK_INDEX        15U
#define TASK7_LAP_MIN_START_SPEED_CMPS 8.0f
#define TASK7_LAUNCH_DETECT_SPEED_CMPS 0.5f
#define TASK7_POST_BREAKAWAY_INTEGRAL  300.0f

static float auto_drive_2026_task4_smoothstep(uint32_t elapsed_ms,
	uint32_t duration_ms)
{
	float t;

	if(elapsed_ms>=duration_ms) return 1.0f;
	t=(float)elapsed_ms/(float)duration_ms;
	return t*t*t*(t*(t*6.0f-15.0f)+10.0f);
}

static float auto_drive_2026_abs_float(float value)
{
	return (value>=0.0f)?value:-value;
}

static float auto_drive_2026_measured_speed_cmps(void)
{
	return auto_drive_2026_abs_float(0.5f*(
		smartcar_imu.left_motor_speed_cmps+
		smartcar_imu.right_motor_speed_cmps));
}

static uint8_t auto_drive_2026_both_wheels_started(float detect_speed_cmps)
{
	return smartcar_imu.left_motor_speed_cmps>=
		detect_speed_cmps&&
		smartcar_imu.right_motor_speed_cmps>=
		detect_speed_cmps;
}

static float auto_drive_2026_decel_start_speed_cmps(float command_speed_cmps)
{
	float measured_speed_cmps;
	float start_speed_cmps;

	measured_speed_cmps=auto_drive_2026_measured_speed_cmps();
	start_speed_cmps=auto_drive_2026_abs_float(command_speed_cmps);
	if(measured_speed_cmps>start_speed_cmps)
	{
		start_speed_cmps=measured_speed_cmps;
	}
	return start_speed_cmps;
}

static void auto_drive_2026_set_drive_speed(float base_speed_cmps,
	float speed_difference)
{
	base_speed_cmps=auto_drive_2026_abs_float(base_speed_cmps);
	speed_setup=base_speed_cmps;
	/* Match oldcar: gray PD directly forms the left/right wheel targets. */
	speed_expect[0]=base_speed_cmps+speed_difference;
	speed_expect[1]=base_speed_cmps-speed_difference;
	if(trackless_output.unlock_flag==LOCK)
	{
		/* The motor output is locked at zero. Keep the speed PI state clear so
		 * the stationary launch command cannot accumulate a release kick. */
		speed_control_reset();
		speed_setup=base_speed_cmps;
		speed_expect[0]=base_speed_cmps+speed_difference;
		speed_expect[1]=base_speed_cmps-speed_difference;
		return;
	}
	speed_control_100hz(speed_ctrl_mode);
}

static void auto_drive_2026_gray_set_speed(float base_speed_cmps,
	float steer_blend)
{
	gray_turn_control_200hz(&turn_ctrl_pwm);
	auto_drive_2026_set_drive_speed(base_speed_cmps,
		turn_ctrl_pwm*turn_scale*steer_blend);
}

static void auto_drive_2026_task4_set_speed(float base_speed_cmps,
	float steer_blend)
{
	auto_drive_2026_gray_set_speed(base_speed_cmps,steer_blend);
}

static float auto_drive_2026_yaw_change_deg(float current_deg,
	float start_deg)
{
	float delta=current_deg-start_deg;
	while(delta>180.0f) delta-=360.0f;
	while(delta<-180.0f) delta+=360.0f;
	return delta<0.0f?-delta:delta;
}

void auto_drive_2026_task4(void)
{
	static uint8_t n=12;
	static uint16_t b_detect_cnt=0;
	static uint8_t b_arrival_confirmed=0;
	static uint8_t launch_motion_started=0;
	static uint8_t straight_timer_started=0;
	static uint8_t timer_gate_active=0;
	static float start_yaw_deg=0;
	static uint32_t ramp_start_ms=0;
	static uint32_t straight_start_ms=0;
	static uint32_t decel_start_ms=0;
	static float decel_start_speed_cmps=0;
	uint32_t now_ms=get_systick_ms();
	float blend;
	float base_speed_cmps;
	float straight_elapsed_ms;
	float speed_ratio;
	float decel_speed_cmps;
	uint8_t b_sensor_detected;
	uint8_t force_decel;

	speed_ctrl_mode=1;
	BallBalance_SetVehicleBraking(false);

	if(flight_subtask_cnt[n]==0)
	{
		start_yaw_deg=smartcar_imu.rpy_deg[_YAW];
		ramp_start_ms=now_ms;
		straight_start_ms=0;
		decel_start_ms=0;
		decel_start_speed_cmps=0;
		b_detect_cnt=0;
		b_arrival_confirmed=0;
		launch_motion_started=0;
		straight_timer_started=0;
		timer_gate_active=0;
		Timer_Gate_Set(0U);
		turn_ctrl_pwm=0;
		speed_control_reset();
		auto_drive_2026_set_drive_speed(0.0f,0.0f);
		flight_subtask_cnt[n]=1;
	}

	if(flight_subtask_cnt[n]==1)
	{
		if(trackless_output.unlock_flag==UNLOCK)
		{
			Timer_Gate_Set(1U);
			timer_gate_active=1;
			if(!straight_timer_started)
			{
				straight_timer_started=1;
				straight_start_ms=now_ms;
			}
		}
		else if(timer_gate_active)
		{
			Timer_Gate_Set(0U);
			timer_gate_active=0;
		}
		if(!launch_motion_started&&
		   auto_drive_2026_both_wheels_started(
		       TASK_LAUNCH_DETECT_SPEED_CMPS))
		{
			launch_motion_started=1;
			ramp_start_ms=now_ms;
			/* Drop the static-friction PI reserve as soon as both wheels roll.
			 * Keeping the full reserve produced a 13~16 cm/s first jump while
			 * the task-4 launch command was still near 8 cm/s. */
			if(speed_integral[0]>TASK4_POST_BREAKAWAY_INTEGRAL)
				speed_integral[0]=TASK4_POST_BREAKAWAY_INTEGRAL;
			if(speed_integral[1]>TASK4_POST_BREAKAWAY_INTEGRAL)
				speed_integral[1]=TASK4_POST_BREAKAWAY_INTEGRAL;
		}
		straight_elapsed_ms=straight_timer_started?
			(float)(uint32_t)(now_ms-straight_start_ms):0.0f;

		b_sensor_detected=straight_timer_started&&
			straight_elapsed_ms>=(float)TASK4_B_SENSOR_ENABLE_MS&&
			(gray_status[0]>TASK4_B_GRAY_THRESHOLD||
			 auto_drive_2026_yaw_change_deg(smartcar_imu.rpy_deg[_YAW],
			    start_yaw_deg)>=TASK4_B_YAW_CHANGE_DEG);
		force_decel=straight_timer_started&&
			straight_elapsed_ms>=(float)TASK4_B_FORCE_DECEL_MS;
		if(b_sensor_detected)
		{
			if(b_detect_cnt<TASK4_B_DETECT_CONFIRM_COUNT) b_detect_cnt++;
			if(b_detect_cnt>=TASK4_B_DETECT_CONFIRM_COUNT)
				b_arrival_confirmed=1;
		}
		else
		{
			b_detect_cnt=0;
		}

		if(!launch_motion_started)
		{
			base_speed_cmps=TASK4_TRACKING_SPEED_CMPS;
			if(base_speed_cmps>TASK4_MIN_START_SPEED_CMPS)
				base_speed_cmps=TASK4_MIN_START_SPEED_CMPS;
		}
		else
		{
			blend=auto_drive_2026_task4_smoothstep(
				(uint32_t)(now_ms-ramp_start_ms),TASK4_START_RAMP_MS);
			base_speed_cmps=TASK4_TRACKING_SPEED_CMPS;
			if(base_speed_cmps>TASK4_MIN_START_SPEED_CMPS)
			{
				base_speed_cmps=TASK4_MIN_START_SPEED_CMPS+
					(base_speed_cmps-TASK4_MIN_START_SPEED_CMPS)*blend;
			}
		}
		/* Follow gray continuously; time/yaw are used only to identify B. */
		speed_ratio=TASK4_TRACKING_SPEED_CMPS;
		speed_ratio=speed_ratio>0.01f?base_speed_cmps/speed_ratio:0.0f;
		auto_drive_2026_gray_set_speed(base_speed_cmps,speed_ratio);
		auto_vision_2026_task4(0);
		if(!force_decel&&!b_arrival_confirmed)
			return;

		if(b_arrival_confirmed)
		{
			Timer_Gate_Set(0U);
			timer_gate_active=0;
		}
		flight_subtask_cnt[n]=2;
		decel_start_ms=now_ms;
		decel_start_speed_cmps=
			auto_drive_2026_decel_start_speed_cmps(speed_setup);
		if(decel_start_speed_cmps<=TASK4_STOP_COMPLETE_SPEED_CMPS)
			flight_subtask_cnt[n]=3;
		bling_set(&light_red,2000,500,0.5,0,0);
		beep.reset=1;
		beep.times=2;
	}

	if(flight_subtask_cnt[n]==2)
	{
		BallBalance_SetVehicleBraking(true);
		if(!b_arrival_confirmed&&timer_gate_active)
		{
			b_sensor_detected=gray_status[0]>TASK4_B_GRAY_THRESHOLD||
				auto_drive_2026_yaw_change_deg(
					smartcar_imu.rpy_deg[_YAW],start_yaw_deg)>=
					TASK4_B_YAW_CHANGE_DEG;
			if(b_sensor_detected)
			{
				if(b_detect_cnt<TASK4_B_DETECT_CONFIRM_COUNT)
					b_detect_cnt++;
				if(b_detect_cnt>=TASK4_B_DETECT_CONFIRM_COUNT)
				{
					b_arrival_confirmed=1;
					Timer_Gate_Set(0U);
					timer_gate_active=0;
				}
			}
			else
			{
				b_detect_cnt=0;
			}
		}
		blend=auto_drive_2026_task4_smoothstep(
			(uint32_t)(now_ms-decel_start_ms),TASK4_STOP_DECEL_MS);
		decel_speed_cmps=decel_start_speed_cmps*(1.0f-blend);
		if(decel_speed_cmps>TASK4_STOP_COMPLETE_SPEED_CMPS)
		{
			speed_ratio=decel_speed_cmps/decel_start_speed_cmps;
			speed_ratio=constrain_float(speed_ratio,0.0f,1.0f);
			auto_drive_2026_task4_set_speed(
				decel_speed_cmps,speed_ratio);
			/* Compensate the repeatable rearward offset while braking toward B.
			 * Restore the physical center target as soon as B is confirmed. */
			g_auto_vision_2026_status.target_mm=b_arrival_confirmed?
				0:TASK4_BRAKING_TARGET_MM;
			auto_vision_2026_task4(0);
			return;
		}
		flight_subtask_cnt[n]=3;
		if(timer_gate_active)
		{
			Timer_Gate_Set(0U);
			timer_gate_active=0;
		}
		turn_ctrl_pwm=0;
	}

	BallBalance_SetVehicleBraking(false);
	g_auto_vision_2026_status.target_mm=0;
	speed_setup=0;
	speed_expect[0]=0;
	speed_expect[1]=0;
	speed_control_100hz(speed_ctrl_mode);
	auto_vision_2026_task4(1);
}

/* Full-lap gray tracker used by tasks 2/5/6.  The oldcar implementation
 * keeps the gray PD active through both straights and arcs; there are no
 * yaw/gray mode transitions that can inject a differential-speed step. */
static uint8_t auto_drive_2026_lap_stop_gray(uint8_t n,
	float tracking_speed_cmps,uint8_t use_launch_ramp,
	uint8_t use_stop_ramp)
{
	static uint8_t launch_motion_started[SUBTASK_NUM]={0};
	static uint8_t timer_gate_active[SUBTASK_NUM]={0};
	static uint32_t ramp_start_ms[SUBTASK_NUM]={0};
	static uint32_t decel_start_ms[SUBTASK_NUM]={0};
	static float decel_start_speed[SUBTASK_NUM]={0};
	uint32_t now_ms=get_systick_ms();
	float base_speed;
	float launch_min_speed;
	float launch_detect_speed;
	float steer_blend;
	float ramp_blend;
	float decel_blend;
	float decel_speed;

	speed_ctrl_mode=1;
	BallBalance_SetVehicleBraking(false);
	if(flight_subtask_cnt[n]==0)
	{
		gray_stop_detection_reset();
		road_miss_flag=0;
		road_miss_cnt=0;
		turn_ctrl_pwm=0.0f;
		launch_motion_started[n]=0;
		timer_gate_active[n]=0;
		Timer_Gate_Set(0U);
		ramp_start_ms[n]=now_ms;
		decel_start_ms[n]=0;
		decel_start_speed[n]=0.0f;
		/* Do not carry the previous lap's PI integral into the next launch;
		 * that stale output would add an uncontrolled kick to either mode. */
		speed_control_reset();
		auto_drive_2026_set_drive_speed(0.0f,0.0f);
		flight_subtask_cnt[n]=1;
	}

	if(flight_subtask_cnt[n]==1&&gray_stop_flag==0)
	{
		if(trackless_output.unlock_flag==UNLOCK)
		{
			Timer_Gate_Set(1U);
			timer_gate_active[n]=1;
		}
		else if(timer_gate_active[n])
		{
			Timer_Gate_Set(0U);
			timer_gate_active[n]=0;
		}
		base_speed=auto_drive_2026_abs_float(tracking_speed_cmps);
		launch_min_speed=(n==TASK7_LAP_SUBTASK_INDEX)?
			TASK7_LAP_MIN_START_SPEED_CMPS:
			TASK_LAP_MIN_START_SPEED_CMPS;
		launch_detect_speed=(n==TASK7_LAP_SUBTASK_INDEX)?
			TASK7_LAUNCH_DETECT_SPEED_CMPS:
			TASK_LAUNCH_DETECT_SPEED_CMPS;
		if(!use_launch_ramp)
		{
			launch_motion_started[n]=1;
			ramp_blend=1.0f;
		}
		else if(!launch_motion_started[n]&&
		        auto_drive_2026_both_wheels_started(
		            launch_detect_speed))
		{
			launch_motion_started[n]=1;
			ramp_start_ms[n]=now_ms;
			if(n==TASK7_LAP_SUBTASK_INDEX)
			{
				/* One encoder count is about 0.9 cm/s in the latest log.
				 * Release excess PI torque on that first count so a brief
				 * movement cannot stop and be followed by a larger launch. */
				if(speed_integral[0]>TASK7_POST_BREAKAWAY_INTEGRAL)
					speed_integral[0]=TASK7_POST_BREAKAWAY_INTEGRAL;
				if(speed_integral[1]>TASK7_POST_BREAKAWAY_INTEGRAL)
					speed_integral[1]=TASK7_POST_BREAKAWAY_INTEGRAL;
			}
		}
		if(use_launch_ramp&&!launch_motion_started[n])
		{
			if(base_speed>launch_min_speed)
				base_speed=launch_min_speed;
			ramp_blend=base_speed>0.01f?
				base_speed/launch_min_speed:0.0f;
		}
		else if(use_launch_ramp)
		{
			ramp_blend=auto_drive_2026_task4_smoothstep(
				(uint32_t)(now_ms-ramp_start_ms[n]),TASK_LAP_START_RAMP_MS);
			if(base_speed>launch_min_speed)
				base_speed=launch_min_speed+
					(base_speed-launch_min_speed)*ramp_blend;
		}
		/* Scale only the gray correction during launch; after ramp it is 1. */
		steer_blend=use_launch_ramp?
			(launch_motion_started[n]?ramp_blend:0.0f):1.0f;
		auto_drive_2026_gray_set_speed(base_speed,steer_blend);
		return 0;
	}

	if(flight_subtask_cnt[n]==1)
	{
		Timer_Gate_Set(0U);
		timer_gate_active[n]=0;
		if(use_stop_ramp)
		{
			flight_subtask_cnt[n]=2;
			decel_start_ms[n]=now_ms;
			decel_start_speed[n]=
				auto_drive_2026_decel_start_speed_cmps(speed_setup);
		}
		else
		{
			/* Task 2 stops its scoring timer at A. Remove the three-second
			 * controlled deceleration used by the moving-ball tasks. */
			flight_subtask_cnt[n]=3;
		}
		bling_set(&light_red,2000,500,0.5f,0,0);
		beep.reset=1;
		beep.times=2;
	}
	if(flight_subtask_cnt[n]==2)
	{
		BallBalance_SetVehicleBraking(true);
		decel_blend=auto_drive_2026_task4_smoothstep(
			(uint32_t)(now_ms-decel_start_ms[n]),TASK_STOP_DECEL_MS);
		decel_speed=decel_start_speed[n]*(1.0f-decel_blend);
		if(decel_speed>TASK_STOP_COMPLETE_SPEED_CMPS)
		{
			steer_blend=decel_start_speed[n]>0.01f?
				decel_speed/decel_start_speed[n]:0.0f;
			auto_drive_2026_gray_set_speed(decel_speed,steer_blend);
			return 0;
		}
		flight_subtask_cnt[n]=3;
	}
	BallBalance_SetVehicleBraking(false);
	auto_drive_2026_set_drive_speed(0.0f,0.0f);
	return 1;
}

void auto_drive_2026_task5(void)
{
	/* A repeated run of the same SDK mode can reset the lap counter without
	 * changing sdk_work_mode.  Clear the completed visual latch in that case,
	 * otherwise telemetry reports route_mode=0 before the next launch. */
	if(g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_5&&
	   g_auto_vision_2026_status.task_complete&&
	   flight_subtask_cnt[13]==0)
	{
		auto_vision_2026_stop();
	}
	uint8_t route_complete=auto_drive_2026_lap_stop_gray(13,
		_self_guided_tracking_speed,1U,1U);
	auto_vision_2026_task5(route_complete);
}

void auto_drive_2026_task6(void)
{
	uint8_t route_complete;

	if(g_auto_vision_2026_status.task_id!=AUTO_VISION_2026_TASK_6||
	   g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK6_CAPTURE||
	   g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK67_SELECT)
	{
		auto_drive_2026_hold_still();
		auto_vision_2026_task6(0);
		return;
	}

	route_complete=auto_drive_2026_lap_stop_gray(14,
		_self_guided_tracking_speed,1U,1U);
	auto_vision_2026_task6(route_complete);
}

void auto_drive_2026_task7(void)
{
	uint8_t route_complete;

	if(g_auto_vision_2026_status.task_id!=AUTO_VISION_2026_TASK_7||
	   g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK6_CAPTURE||
	   g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK67_SELECT)
	{
		auto_drive_2026_hold_still();
		auto_vision_2026_task7(0);
		return;
	}

	route_complete=auto_drive_2026_lap_stop_gray(15,
		_self_guided_tracking_speed,1U,1U);
	auto_vision_2026_task7(route_complete);
}

void auto_drive_2026_vision_debug(void)
{
	auto_drive_2026_hold_still();
	auto_vision_2026_debug_center();
}

/* MaixCAM Pro visual scheduling. */
#include "ftServo.h"
#include <string.h>

#define AUTO_VISION_2026_TASK2_DEADLINE_MS  (20000U)
#define AUTO_VISION_2026_TASK3_DEADLINE_MS  (5000U)
#define AUTO_VISION_2026_TASK4_DEADLINE_MS  (8000U)
#define AUTO_VISION_2026_TASK56_DEADLINE_MS (30000U)
#define AUTO_VISION_2026_TASK67_SELECT_WINDOW_MS (1000U)
#define AUTO_VISION_2026_TASK6_DEFAULT_TARGET_MM (-74)
#define AUTO_VISION_2026_TASK7_DEFAULT_TARGET_MM (72)
#define AUTO_VISION_2026_TASK7_DEFAULT_HOLD_BIAS_DEG (-4.0f)
#define AUTO_VISION_2026_TASK3_POSITIVE_TOLERANCE_MM 10
#define AUTO_VISION_2026_TASK3_NEGATIVE_TOLERANCE_MM 10

/* 2026视觉任务全局配置参数 */
AutoVision2026Config g_auto_vision_2026_config={
	.position_tolerance_mm=7,          /* 任务3按赛题±1cm要求留1mm余量 */
	.velocity_tolerance_mm_s=22,        /* 速度容差 (mm/s) */
	.positive_target_mm=55,             /* 正方向机械标定补偿，实际目标向右移5mm */
	.negative_target_mm=-55,            /* 负方向机械标定补偿，实测 -4.1~-4.8 cm 向 -5 cm 靠近 */
	.ball_center_limit_mm=120,         /* 任务6/7记忆范围，覆盖水管有效区间并留边 */
	.settle_time_ms=100U,              /* 位置控制收紧后，稳定100ms即切换目标 */
	.vision_timeout_ms=100U,            /* 视觉数据超时时间 (ms) */
	/* 大于1.05时启用任务3正向穿越模式：最大球速巡航并跳过防静止 */
	.task3_positive_position_gain_scale=1.35f
};
AutoVision2026Status g_auto_vision_2026_status;

static uint8_t g_auto_vision_2026_stable_timer_active;
static uint8_t g_auto_vision_2026_route_complete;
static uint32_t g_auto_vision_2026_stable_start_ms;
static uint32_t g_auto_vision_2026_route_stop_start_ms;
static uint32_t g_auto_vision_2026_task67_select_start_ms;

static int32_t auto_vision_2026_abs_i16(int16_t value)
{
	int32_t widened=value;
	return widened<0?-widened:widened;
}

static int16_t auto_vision_2026_clamp_i16(int16_t value,int16_t lower,int16_t upper)
{
	if(value<lower) return lower;
	if(value>upper) return upper;
	return value;
}

static uint8_t auto_vision_2026_observation_current(uint32_t now_ms)
{
	if(!g_ball_balance_status.vision_valid) return 0;
	return (uint32_t)(now_ms-g_ball_balance_status.received_at_ms)<=
		g_auto_vision_2026_config.vision_timeout_ms;
}

static uint32_t auto_vision_2026_deadline_ms(uint8_t task_id)
{
	switch(task_id)
	{
		case AUTO_VISION_2026_TASK_2:return AUTO_VISION_2026_TASK2_DEADLINE_MS;
		case AUTO_VISION_2026_TASK_3:return AUTO_VISION_2026_TASK3_DEADLINE_MS;
		case AUTO_VISION_2026_TASK_4:return AUTO_VISION_2026_TASK4_DEADLINE_MS;
		case AUTO_VISION_2026_TASK_5:
		case AUTO_VISION_2026_TASK_6:
		case AUTO_VISION_2026_TASK_7:return AUTO_VISION_2026_TASK56_DEADLINE_MS;
		default:return 0;
	}
}

static uint8_t auto_vision_2026_route_for_task(uint8_t task_id)
{
	switch(task_id)
	{
		case AUTO_VISION_2026_TASK_2:return AUTO_VISION_2026_ROUTE_ONE_LAP_STOP_A;
		case AUTO_VISION_2026_TASK_4:return AUTO_VISION_2026_ROUTE_TO_B;
		case AUTO_VISION_2026_TASK_5:
		case AUTO_VISION_2026_TASK_6:
		case AUTO_VISION_2026_TASK_7:return AUTO_VISION_2026_ROUTE_ONE_LAP_PASS_A;
		default:return AUTO_VISION_2026_ROUTE_STOP;
	}
}

static void auto_vision_2026_reset_stable_timer(void)
{
	g_auto_vision_2026_stable_timer_active=0;
	g_auto_vision_2026_stable_start_ms=0;
}

static void auto_vision_2026_set_phase_target(uint8_t phase,int16_t target_mm)
{
	g_auto_vision_2026_status.phase=phase;
	g_auto_vision_2026_status.target_mm=target_mm;
	auto_vision_2026_reset_stable_timer();
}

static uint8_t auto_vision_2026_update_stability(uint8_t current,
	uint32_t now_ms,uint32_t required_stable_ms,
	int16_t position_tolerance_mm)
{
	int16_t error;
	uint8_t stable;

	if(!current)
	{
		auto_vision_2026_reset_stable_timer();
		return 0;
	}

	error=(int16_t)(g_auto_vision_2026_status.target_mm-
		g_ball_balance_status.raw_position_mm);
	stable=auto_vision_2026_abs_i16(error)<=
		position_tolerance_mm&&
		auto_vision_2026_abs_i16(g_ball_balance_status.raw_velocity_mm_s)<=
		g_auto_vision_2026_config.velocity_tolerance_mm_s;
	if(!stable)
	{
		auto_vision_2026_reset_stable_timer();
		return 0;
	}
	if(!g_auto_vision_2026_stable_timer_active)
	{
		g_auto_vision_2026_stable_timer_active=1;
		g_auto_vision_2026_stable_start_ms=now_ms;
		return 0;
	}
	return (uint32_t)(now_ms-g_auto_vision_2026_stable_start_ms)>=
		required_stable_ms;
}

static void auto_vision_2026_signal(uint8_t beep_times)
{
	bling_set(&light_red,2000,500,0.5,0,0);
	beep.reset=1;
	beep.times=beep_times;
}

static void auto_vision_2026_advance_task3(void)
{
	switch(g_auto_vision_2026_status.phase)
	{
		case AUTO_VISION_2026_PHASE_TASK3_TO_CENTER:
			auto_vision_2026_set_phase_target(
				AUTO_VISION_2026_PHASE_TASK3_WAIT_CENTER,0);
		break;
		case AUTO_VISION_2026_PHASE_TASK3_TO_POSITIVE:
			auto_vision_2026_set_phase_target(
				AUTO_VISION_2026_PHASE_TASK3_TO_NEGATIVE,
				g_auto_vision_2026_config.negative_target_mm);
			auto_vision_2026_signal(1);
		break;
		case AUTO_VISION_2026_PHASE_TASK3_TO_NEGATIVE:
			auto_vision_2026_set_phase_target(
				AUTO_VISION_2026_PHASE_TASK3_HOLD_NEGATIVE,
				g_auto_vision_2026_config.negative_target_mm);
			g_auto_vision_2026_status.task_complete=1;
			Timer_Gate_Set(0U);
			auto_vision_2026_signal(2);
		break;
		default:break;
	}
}

void auto_vision_2026_init(void)
{
	auto_vision_2026_stop();
}

uint8_t auto_vision_2026_start(uint8_t task_id)
{
	uint32_t now_ms=get_systick_ms();
	uint8_t current;

	if(task_id<AUTO_VISION_2026_TASK_2||
	   task_id>AUTO_VISION_2026_TASK_DEBUG_CENTER)
		return 0;

	memset(&g_auto_vision_2026_status,0,sizeof(g_auto_vision_2026_status));
	g_auto_vision_2026_status.task_id=task_id;
	g_auto_vision_2026_status.task_start_ms=now_ms;
	g_auto_vision_2026_route_complete=0;
	g_auto_vision_2026_route_stop_start_ms=0U;
	g_auto_vision_2026_task67_select_start_ms=0U;
	BallBalance_SetVehicleBraking(false);
	BallBalance_SetTargetHoldBiasDeg(0.0f);
	auto_vision_2026_reset_stable_timer();
	current=auto_vision_2026_observation_current(now_ms);
	if(task_id==AUTO_VISION_2026_TASK_3)
	{
		BallBalance_SetControlProfile(BALL_BALANCE_PROFILE_TASK3);
	}
	else if(task_id==AUTO_VISION_2026_TASK_4)
	{
		BallBalance_SetControlProfile(BALL_BALANCE_PROFILE_TASK4);
	}
	else if(task_id==AUTO_VISION_2026_TASK_5)
	{
		BallBalance_SetControlProfile(BALL_BALANCE_PROFILE_TASK5);
	}
	else if(task_id==AUTO_VISION_2026_TASK_6)
	{
		BallBalance_SetControlProfile(BALL_BALANCE_PROFILE_TASK6);
	}
	else if(task_id==AUTO_VISION_2026_TASK_7)
	{
		BallBalance_SetControlProfile(BALL_BALANCE_PROFILE_TASK7);
	}
	else
	{
		BallBalance_SetControlProfile(BALL_BALANCE_PROFILE_STATIONARY);
	}

	switch(task_id)
	{
		case AUTO_VISION_2026_TASK_2:
		case AUTO_VISION_2026_TASK_4:
		case AUTO_VISION_2026_TASK_5:
		case AUTO_VISION_2026_TASK_DEBUG_CENTER:
			auto_vision_2026_set_phase_target(
				AUTO_VISION_2026_PHASE_HOLD_CENTER,0);
		break;
		case AUTO_VISION_2026_TASK_3:
			auto_vision_2026_set_phase_target(
				AUTO_VISION_2026_PHASE_TASK3_WAIT_CENTER,0);
		break;
		case AUTO_VISION_2026_TASK_6:
		case AUTO_VISION_2026_TASK_7:
			auto_vision_2026_set_phase_target(
				AUTO_VISION_2026_PHASE_TASK6_CAPTURE,
				current?auto_vision_2026_clamp_i16(
					g_ball_balance_status.raw_position_mm,
					(int16_t)-g_auto_vision_2026_config.ball_center_limit_mm,
					g_auto_vision_2026_config.ball_center_limit_mm):0);
		break;
		default:return 0;
	}

	if(g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK3_WAIT_CENTER||
	   g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK6_CAPTURE||
	   g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK67_SELECT)
	{
		BallBalance_SetEnabled(false);
		Servo_Y_SetAngle(g_ball_balance_config.servo_neutral_angle_deg,
		                 g_ball_balance_config.servo_speed);
	}
	Servo_Y_Enable(true);
	return 1;
}

void auto_vision_2026_update(uint8_t route_complete)
{
	uint32_t now_ms=get_systick_ms();
	uint32_t deadline;
	uint8_t current;

	if(g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_IDLE)
	{
		BallBalance_SetVehicleFeedforwardEnabled(false);
		BallBalance_SetEnabled(false);
		return;
	}

	current=auto_vision_2026_observation_current(now_ms);
	if(g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_3&&
	   (g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK3_TO_POSITIVE||
	    g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK3_TO_NEGATIVE))
	{
		/* Task 3 is timed from the first key press through arrival at -5 cm.
		 * Keep PA15 asserted for the whole interval instead of relying on one
		 * edge write at the key event. */
		Timer_Gate_Set(1U);
	}
	if(g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK3_WAIT_CENTER)
	{
		if(current&&_button.state[ME_3D].press==SHORT_PRESS)
		{
			_button.state[ME_3D].press=NO_PRESS;
			auto_vision_2026_set_phase_target(
				AUTO_VISION_2026_PHASE_TASK3_TO_POSITIVE,
				g_auto_vision_2026_config.positive_target_mm);
			g_auto_vision_2026_status.task_start_ms=now_ms;
			Timer_Gate_Set(1U);
			auto_vision_2026_signal(1);
		}
	}

	if(g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK6_CAPTURE)
	{
		Servo_Y_SetAngle(g_ball_balance_config.servo_neutral_angle_deg,
		                 g_ball_balance_config.servo_speed);
		if(current)
		{
			g_auto_vision_2026_status.target_mm=auto_vision_2026_clamp_i16(
				g_ball_balance_status.raw_position_mm,
				(int16_t)-g_auto_vision_2026_config.ball_center_limit_mm,
				g_auto_vision_2026_config.ball_center_limit_mm);
			if(_button.state[ME_3D].press==SHORT_PRESS)
			{
				_button.state[ME_3D].press=NO_PRESS;
				g_auto_vision_2026_task67_select_start_ms=now_ms;
				auto_vision_2026_set_phase_target(
					AUTO_VISION_2026_PHASE_TASK67_SELECT,
					g_auto_vision_2026_status.target_mm);
				auto_vision_2026_signal(1);
			}
		}
	}
	if(g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK67_SELECT)
	{
		/* After the first press, keep the beam neutral while the operator chooses
		 * between the captured position and the task's standard target. */
		Servo_Y_SetAngle(g_ball_balance_config.servo_neutral_angle_deg,
		                 g_ball_balance_config.servo_speed);
		if(current&&_button.state[ME_3D].press==SHORT_PRESS)
		{
			_button.state[ME_3D].press=NO_PRESS;
			BallBalance_SetTargetHoldBiasDeg(
				g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_7?
				AUTO_VISION_2026_TASK7_DEFAULT_HOLD_BIAS_DEG:0.0f);
			auto_vision_2026_set_phase_target(
				AUTO_VISION_2026_PHASE_TASK6_HOLD,
				g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_6?
				AUTO_VISION_2026_TASK6_DEFAULT_TARGET_MM:
				AUTO_VISION_2026_TASK7_DEFAULT_TARGET_MM);
			g_auto_vision_2026_status.task_start_ms=now_ms;
			auto_vision_2026_signal(1);
		}
		else if(current&&g_auto_vision_2026_task67_select_start_ms!=0U&&
				(uint32_t)(now_ms-g_auto_vision_2026_task67_select_start_ms)>=
				AUTO_VISION_2026_TASK67_SELECT_WINDOW_MS)
		{
			/* No second press: release the vehicle with the memorized target. */
			BallBalance_SetTargetHoldBiasDeg(0.0f);
			auto_vision_2026_set_phase_target(
				AUTO_VISION_2026_PHASE_TASK6_HOLD,
				g_auto_vision_2026_status.target_mm);
			g_auto_vision_2026_status.task_start_ms=now_ms;
			auto_vision_2026_signal(1);
		}
	}
	if(g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK3_HOLD_NEGATIVE&&
	   current&&_button.state[ME_3D].press==SHORT_PRESS)
	{
		_button.state[ME_3D].press=NO_PRESS;
		g_auto_vision_2026_status.task_complete=0;
		auto_vision_2026_set_phase_target(
			AUTO_VISION_2026_PHASE_TASK3_TO_CENTER,0);
		g_auto_vision_2026_status.task_start_ms=now_ms;
		auto_vision_2026_signal(1);
	}

	if(g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_3)
	{
		if(g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK3_TO_POSITIVE)
		{
			/* The +5 cm point is a turnaround point, not a stop point.  Waiting
			 * for low speed here added a second settling cycle and made task 3
			 * exceed five seconds.  A measured sample inside the allowed position
			 * window is enough; -5 cm still uses full position/speed settling. */
			if(current&&g_ball_balance_status.measured&&
			   g_ball_balance_status.raw_position_mm>=
				(g_auto_vision_2026_config.positive_target_mm-
				 AUTO_VISION_2026_TASK3_POSITIVE_TOLERANCE_MM))
			{
				auto_vision_2026_advance_task3();
			}
		}
		else if((g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK3_TO_CENTER||
				 g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK3_TO_NEGATIVE)&&
				auto_vision_2026_update_stability(current,now_ms,
					g_auto_vision_2026_config.settle_time_ms,
					g_auto_vision_2026_status.phase==
						AUTO_VISION_2026_PHASE_TASK3_TO_NEGATIVE?
						AUTO_VISION_2026_TASK3_NEGATIVE_TOLERANCE_MM:
						g_auto_vision_2026_config.position_tolerance_mm))
		{
			auto_vision_2026_advance_task3();
		}
	}

	if(route_complete) g_auto_vision_2026_route_complete=1;
	if(route_complete && g_auto_vision_2026_route_stop_start_ms==0U)
	{
		g_auto_vision_2026_route_stop_start_ms=now_ms;
	}
	deadline=auto_vision_2026_deadline_ms(g_auto_vision_2026_status.task_id);
	if(deadline>0U&&
		(uint32_t)(now_ms-g_auto_vision_2026_status.task_start_ms)>deadline)
	{
		g_auto_vision_2026_status.deadline_exceeded=1;
	}

	g_auto_vision_2026_status.route_mode=
		auto_vision_2026_route_for_task(g_auto_vision_2026_status.task_id);
	if(g_auto_vision_2026_route_complete||
		g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK6_CAPTURE||
		g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK67_SELECT)
	{
		g_auto_vision_2026_status.route_mode=AUTO_VISION_2026_ROUTE_STOP;
	}
	g_auto_vision_2026_status.position_error_mm=current?
		(int16_t)(g_auto_vision_2026_status.target_mm-
			g_ball_balance_status.raw_position_mm):0;
	g_auto_vision_2026_status.balance_enabled=
		current&&
		g_auto_vision_2026_status.phase!=AUTO_VISION_2026_PHASE_TASK6_CAPTURE&&
		g_auto_vision_2026_status.phase!=AUTO_VISION_2026_PHASE_TASK67_SELECT;
	{
		uint8_t moving_task=(g_auto_vision_2026_status.task_id==
			AUTO_VISION_2026_TASK_4||
			g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_5||
			g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_6||
			g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_7);
		uint8_t vehicle_still_stopping=0U;

		if(g_auto_vision_2026_route_complete)
		{
			/* The gray-line routine can finish before encoder speed reaches zero.
			 * Keep the feedforward active during that inertial tail so the ball
			 * controller sees the final deceleration instead of losing authority. */
			vehicle_still_stopping=
				(g_auto_vision_2026_route_stop_start_ms!=0U&&
				 (uint32_t)(now_ms-
				  g_auto_vision_2026_route_stop_start_ms)<
				 AUTO_VISION_2026_STOP_BALANCE_HOLD_MS)||
				(g_ball_balance_status.vehicle_measured_speed_cmps>
				 AUTO_VISION_2026_STOP_BALANCE_SPEED_CMPS)||
				(g_ball_balance_status.vehicle_measured_speed_cmps<
				 -AUTO_VISION_2026_STOP_BALANCE_SPEED_CMPS);
			BallBalance_SetVehicleBraking(
				moving_task&&vehicle_still_stopping);
			if(moving_task&&vehicle_still_stopping)
			{
				/* Do not return the servo to neutral for a single delayed
				 * camera frame while the stopped ball is still settling. */
				g_auto_vision_2026_status.balance_enabled=1U;
			}
		}
		BallBalance_SetVehicleFeedforwardEnabled(
			g_auto_vision_2026_status.balance_enabled&&moving_task&&
			(!g_auto_vision_2026_route_complete||vehicle_still_stopping));
	}
	g_auto_vision_2026_status.request_safe_level=
		!g_auto_vision_2026_status.balance_enabled;
	if(g_auto_vision_2026_status.task_id!=AUTO_VISION_2026_TASK_3)
	{
		g_auto_vision_2026_status.task_complete=g_auto_vision_2026_route_complete;
	}

	if(g_auto_vision_2026_status.task_id==AUTO_VISION_2026_TASK_3&&
	   g_auto_vision_2026_status.phase==AUTO_VISION_2026_PHASE_TASK3_TO_POSITIVE)
	{
		BallBalance_SetPositionGainScale(
			 g_auto_vision_2026_config.task3_positive_position_gain_scale);
	}
	else
	{
		BallBalance_SetPositionGainScale(1.0f);
	}
	BallBalance_SetTargetMm((float)g_auto_vision_2026_status.target_mm);
	BallBalance_SetEnabled(g_auto_vision_2026_status.balance_enabled!=0);
}

void auto_vision_2026_stop(void)
{
	memset(&g_auto_vision_2026_status,0,sizeof(g_auto_vision_2026_status));
	g_auto_vision_2026_status.phase=AUTO_VISION_2026_PHASE_IDLE;
	g_auto_vision_2026_status.route_mode=AUTO_VISION_2026_ROUTE_STOP;
	g_auto_vision_2026_status.request_safe_level=1;
	g_auto_vision_2026_route_complete=0;
	g_auto_vision_2026_route_stop_start_ms=0U;
	g_auto_vision_2026_task67_select_start_ms=0U;
	BallBalance_SetVehicleBraking(false);
	auto_vision_2026_reset_stable_timer();
	BallBalance_SetPositionGainScale(1.0f);
	BallBalance_SetTargetHoldBiasDeg(0.0f);
	BallBalance_SetTargetMm(0.0f);
	BallBalance_SetVehicleFeedforwardEnabled(false);
	BallBalance_SetEnabled(false);
	Servo_Y_SetAngle(g_ball_balance_config.servo_neutral_angle_deg,
	                 g_ball_balance_config.servo_speed);
	Servo_Y_Enable(true);
}

static void auto_vision_2026_run(uint8_t task_id,uint8_t route_complete)
{
	/* Re-entering the same SDK mode after a completed lap must create a new
	 * visual run.  The route driver resets its counter without changing the
	 * mode, so task_complete alone must not permanently latch route_mode=0. */
	if(g_auto_vision_2026_status.task_id!=task_id||
	   (task_id!=AUTO_VISION_2026_TASK_3&&
	    g_auto_vision_2026_status.task_complete&&!route_complete))
	{
		if(!auto_vision_2026_start(task_id)) return;
	}
	auto_vision_2026_update(route_complete);
}

void auto_vision_2026_task2(uint8_t route_complete)
{
	auto_vision_2026_run(AUTO_VISION_2026_TASK_2,route_complete);
}

void auto_vision_2026_task3(void)
{
	auto_vision_2026_run(AUTO_VISION_2026_TASK_3,0);
}

void auto_vision_2026_task4(uint8_t route_complete)
{
	auto_vision_2026_run(AUTO_VISION_2026_TASK_4,route_complete);
}

void auto_vision_2026_task5(uint8_t route_complete)
{
	auto_vision_2026_run(AUTO_VISION_2026_TASK_5,route_complete);
}

void auto_vision_2026_task6(uint8_t route_complete)
{
	auto_vision_2026_run(AUTO_VISION_2026_TASK_6,route_complete);
}

void auto_vision_2026_task7(uint8_t route_complete)
{
	auto_vision_2026_run(AUTO_VISION_2026_TASK_7,route_complete);
}

void auto_vision_2026_debug_center(void)
{
	auto_vision_2026_run(AUTO_VISION_2026_TASK_DEBUG_CENTER,0);
}
