#main.py -- put your code here!
import cpufreq
import pyb
import sensor,image, time,math
from pyb import LED,Timer,UART

sensor.reset()                      # Reset and initialize the sensor.
sensor.set_pixformat(sensor.RGB565) # Set pixel format to RGB565 (or GRAYSCALE)
sensor.set_framesize(sensor.QQQVGA)  # Set frame size to QVGA (320x240)
sensor.skip_frames(time = 2000)     #延时跳过一些帧，等待感光元件变稳定
sensor.set_auto_gain(False)          #黑线不易识别时，将此处写False
sensor.set_auto_whitebal(False)
clock = time.clock()                # Create a clock object to track the FPS.
#sensor.set_auto_exposure(True, exposure_us=5000) # 设置自动曝光sensor.get_exposure_us()


uart=UART(3,256000)
THRESHOLD = (0,100) # Grayscale threshold for dark things... (5, 70, -23, 15, -57, 0)(18, 100, 31, -24, -21, 70)
IMAGE_WIDTH=sensor.snapshot().width()
IMAGE_HEIGHT=sensor.snapshot().height()
IMAGE_DIS_MAX=(int)(math.sqrt(IMAGE_WIDTH*IMAGE_WIDTH+IMAGE_HEIGHT*IMAGE_HEIGHT)/2)

class target_check(object):
    x=0          #int16_t
    y=0          #int16_t
    pixel=0      #uint16_t
    flag=0       #uint8_t
    state=0      #uint8_t
    angle=0      #int16_t
    distance=0   #uint16_t
    apriltag_id=0#uint16_t
    img_width=0  #uint16_t
    img_height=0 #uint16_t
    reserved1=0  #uint8_t
    reserved2=0  #uint8_t
    reserved3=0  #uint8_t
    reserved4=0  #uint8_t
    fps=0        #uint8_t
    range_sensor1=0
    range_sensor2=0
    range_sensor3=0
    range_sensor4=0
    camera_id=0
    reserved1_int32=0
    reserved2_int32=0
    reserved3_int32=0
    reserved4_int32=0

class rgb(object):
    def __init__(self):
        self.red=LED(1)
        self.green=LED(2)
        self.blue=LED(3)



class uart_buf_prase(object):
    uart_buf = []
    _data_len = 0
    _data_cnt = 0
    state = 0

class mode_ctrl(object):
    work_mode = 0x01 #工作模式.默认是点检测，可以通过串口设置成其他模式
    check_show = 1   #开显示，在线调试时可以打开，离线使用请关闭，可提高计算速度

ctr=mode_ctrl()


rgb=rgb()
R=uart_buf_prase()
target=target_check()
target.camera_id=0x01
target.reserved1_int32=0
target.reserved2_int32=0
target.reserved3_int32=0
target.reserved4_int32=0

HEADER=[0xFF,0xFC]
MODE=[0xF1,0xF2,0xF3]
#__________________________________________________________________
def package_blobs_data(mode):
    #数据打包封装
    data=bytearray([HEADER[0],HEADER[1],0xC0+mode,0x00,
                   target.x>>8,target.x,        #将整形数据拆分成两个8位
                   target.y>>8,target.y,        #将整形数据拆分成两个8位
                   target.pixel>>8,target.pixel,#将整形数据拆分成两个8位
                   target.flag,                 #数据有效标志位
                   target.state,                #数据有效标志位
                   target.angle>>8,target.angle,#将整形数据拆分成两个8位
                   target.distance>>8,target.distance,#将整形数据拆分成两个8位
                   target.apriltag_id>>8,target.apriltag_id,#将整形数据拆分成两个8位
                   target.img_width>>8,target.img_width,    #将整形数据拆分成两个8位
                   target.img_height>>8,target.img_height,  #将整形数据拆分成两个8位
                   target.fps,      #数据有效标志位
                   target.reserved1,#数据有效标志位
                   target.reserved2,#数据有效标志位
                   target.reserved3,#数据有效标志位
                   target.reserved4,#数据有效标志位
                   target.range_sensor1>>8,target.range_sensor1,
                   target.range_sensor2>>8,target.range_sensor2,
                   target.range_sensor3>>8,target.range_sensor3,
                   target.range_sensor4>>8,target.range_sensor4,
                   target.camera_id,
                   target.reserved1_int32>>24&0xff,target.reserved1_int32>>16&0xff,
                   target.reserved1_int32>>8&0xff,target.reserved1_int32&0xff,
                   target.reserved2_int32>>24&0xff,target.reserved2_int32>>16&0xff,
                   target.reserved2_int32>>8&0xff,target.reserved2_int32&0xff,
                   target.reserved3_int32>>24&0xff,target.reserved3_int32>>16&0xff,
                   target.reserved3_int32>>8&0xff,target.reserved3_int32&0xff,
                   target.reserved4_int32>>24&0xff,target.reserved4_int32>>16&0xff,
                   target.reserved4_int32>>8&0xff,target.reserved4_int32&0xff,
                   0x00])
    #数据包的长度
    data_len=len(data)
    data[3]=data_len-5#有效数据的长度
    #和校验
    sum=0
    for i in range(0,data_len-1):
        sum=sum+data[i]
    data[data_len-1]=sum
    #返回打包好的数据
    return data
#__________________________________________________________________



#串口数据解析
def Receive_Anl(data_buf,num):
    #和校验
    sum = 0
    i = 0
    while i<(num-1):
        sum = sum + data_buf[i]
        i = i + 1
    sum = sum%256 #求余
    if sum != data_buf[num-1]:
        return
    #和校验通过
    if data_buf[2]==0xA0:
        #设置模块工作模式
        ctr.work_mode = data_buf[4]
        print(ctr.work_mode)
        print("Set work mode success!")

#__________________________________________________________________
def uart_data_prase(buf):
    if R.state==0 and buf==0xFF:#帧头1
        R.state=1
        R.uart_buf.append(buf)
    elif R.state==1 and buf==0xFE:#帧头2
        R.state=2
        R.uart_buf.append(buf)
    elif R.state==2 and buf<0xFF:#功能字
        R.state=3
        R.uart_buf.append(buf)
    elif R.state==3 and buf<50:#数据长度小于50
        R.state=4
        R._data_len=buf  #有效数据长度
        R._data_cnt=buf+5#总数据长度
        R.uart_buf.append(buf)
    elif R.state==4 and R._data_len>0:#存储对应长度数据
        R._data_len=R._data_len-1
        R.uart_buf.append(buf)
        if R._data_len==0:
            R.state=5
    elif R.state==5:
        R.uart_buf.append(buf)
        R.state=0
        Receive_Anl(R.uart_buf,R.uart_buf[3]+5)
#        print(R.uart_buf)
        R.uart_buf=[]#清空缓冲区，准备下次接收数据
    else:
        R.state=0
        R.uart_buf=[]#清空缓冲区，准备下次接收数据

#__________________________________________________________________



def uart_data_read():
    buf_len=uart.any()
    for i in range(0,buf_len):
        uart_data_prase(uart.readchar())



def time_callback(info):
    rgb.red.toggle()

timer=Timer(2,freq=4)
timer.callback(time_callback)


# 绘制水平线
def draw_hori_line(img, x0, x1, y, color):
    for x in range(x0, x1):
        img.set_pixel(x, y, color)
# 绘制竖直线
def draw_vec_line(img, x, y0, y1, color):
    for y in range(y0, y1):
        img.set_pixel(x, y, color)
# 绘制矩形
def draw_rect(img, x, y, w, h, color):
    draw_hori_line(img, x, x+w, y, color)
    draw_hori_line(img, x, x+w, y+h, color)
    draw_vec_line(img, x, y, y+h, color)
    draw_vec_line(img, x+w, y, y+h, color)





class singleline_check():
    rho_err = 0
    theta_err = 0
    state = 0


singleline = singleline_check()
THRESHOLD = (0,100) # Grayscale threshold for dark things
thresholds =(0, 30, -30, 30, -30, 30)
#找线
def found_line():
    target.img_width=IMAGE_WIDTH
    target.img_height=IMAGE_HEIGHT
    target.flag = 0
    #sensor.set_pixformat(sensor.GRAYSCALE)
    #img=sensor.snapshot().binary([THRESHOLD])
    img=sensor.snapshot()
    target.img_width =IMAGE_WIDTH
    target.img_height=IMAGE_HEIGHT
    pixels_max=0
    #singleline.state = img.get_regression([thresholds],x_stride=2,y_stride=2,pixels_threshold=10,robust = True)
    singleline.state = img.get_regression([thresholds],robust = True)
    if(singleline.state):
        singleline.rho_err = abs(singleline.state.rho())
        singleline.theta_err = singleline.state.theta()
        target.x=singleline.rho_err;
        target.angle=singleline.theta_err;
        target.flag = 1
    if target.flag==1:
        img.draw_line(singleline.state.line(), color = 127)



track_roi=[(0,25,5,10),
           (5,25,5,10),
           (10,25,5,10),
           (15,25,5,10),
           (20,25,5,10),
           (25,25,5,10),
           (30,25,5,10),
           (35,25,5,10),
           (40,25,5,10),
           (45,25,5,10),
           (50,25,5,10),
           (55,25,5,10),
           (60,25,5,10),
           (65,25,5,10),
           (70,25,5,10),
           (75,25,5,10)]

target_roi=[(70,0,10,12),
           (70,12,10,12),
           (70,24,10,12),
           (70,36,10,12),
           (70,48,10,12)]


thresholds =(0, 30, -30, 30, -30, 30)
class linedata(object):
    bit0=0
    bit1=0
    bit2=0
    bit3=0
    bit4=0
    bit5=0
    bit6=0
    bit7=0
    bit8=0
    bit9=0
    bit10=0
    bit11=0
    bit12=0
    bit13=0
    bit14=0
    bit15=0

lines=linedata()
hor_bits=['0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0']
ver_bits=['0','0','0','0','0',]
def findtrack():
    target.img_width=IMAGE_WIDTH
    target.img_height=IMAGE_HEIGHT
    target.flag = 0
    target.x=0
    target.y=0
    img=sensor.snapshot()
    target.img_width =IMAGE_WIDTH
    target.img_height=IMAGE_HEIGHT

    for i in range(0,16):
        hor_bits[i]=0
        blobs=img.find_blobs([thresholds],roi=track_roi[i],merge=True,margin=10)
        for b in blobs:
            #img.draw_rectangle(b[0:4])
            #img.draw_circle(b.cx(),b.cy(),2,(255,100,100))
            hor_bits[i]=1


    for i in range(0,5):
        ver_bits[i]=0
        blobs=img.find_blobs([thresholds],roi=target_roi[i],merge=True,margin=10)
        for b in blobs:
            ver_bits[i]=1


    target.flag = 1
    for k in range(0,16):
        if  hor_bits[k]:
            target.x=target.x|(0x01<<(15-k))
            img.draw_circle(int(track_roi[k][0]+track_roi[k][2]*0.5),int(track_roi[k][1]+track_roi[k][3]*0.5),1,(255,0,0))



    for k in range(0,5):
        if  ver_bits[k]:
            target.y=target.y|(0x01<<(4-k))
            img.draw_circle(int(target_roi[k][0]+target_roi[k][2]*0.5),int(target_roi[k][1]+target_roi[k][3]*0.5),3,(0,255,0))

    for rec in track_roi:
        img.draw_rectangle(rec, color=(0,0,255))#绘制出roi区域
    for rec in target_roi:
        img.draw_rectangle(rec, color=(0,255,255))#绘制出roi区域

    print(target.x,target.y)
ctr.work_mode=0x04
last_ticks=0
ticks=0
ticks_delta=0;
while True:
    clock.tick()
    if ctr.work_mode==0x03:
        found_line()
    elif ctr.work_mode==0x04:
        findtrack()
    uart.write(package_blobs_data(ctr.work_mode))
    uart_data_read()
#__________________________________________________________________
    #计算fps
    last_ticks=ticks
    ticks=time.ticks_ms()#ticks=time.ticks_ms()
                      #新版本OPENMV固件使用time.ticks_ms()
                      #旧版本OPENMV固件使用time.ticks()
    ticks_delta=ticks-last_ticks
    if ticks_delta<1:
        ticks_delta=1
    target.fps=(int)(1000/ticks_delta)
    #target.fps = (int)(clock.fps())
#__________________________________________________________________
    #print(target.fps,ticks-last_ticks,ctr.work_mode,IMAGE_WIDTH,IMAGE_HEIGHT)
