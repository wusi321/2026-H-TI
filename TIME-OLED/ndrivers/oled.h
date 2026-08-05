#ifndef __OLED_H
#define __OLED_H

/* OLED 物理分辨率与亮度参数 */
#define XLevelL      0x00
#define XLevelH      0x10
#define XLevel       ((XLevelH&0x0F)*16+XLevelL)
#define Max_Column   128
#define Max_Row      64
#define Brightness   0xCF
#define X_WIDTH      128
#define Y_WIDTH      64

/* 底层核心通信（带全局中断临界区锁，防测速中断及串口噪声干扰） */
void OLED_WrDat(unsigned char IIC_Data);
void OLED_WrCmd(unsigned char IIC_Command);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void oled_init(void);

/* 基础图形与清屏函数 */
void OLED_Fill(unsigned char bmp_dat);
void OLED_CLS(void);
void LCD_clear_L(unsigned char x, unsigned char y); // 清除单行部分残影，带物理寻址对齐

/* 字符与字符串渲染（6x8点阵，带122边界自动换行保护） */
void LCD_P6x8Char(unsigned char x, unsigned char y, unsigned char ucData);
void LCD_P6x8Str(unsigned char x, unsigned char y, char ch[]);

/* 数值格式化转换（采用内置 snprintf 规避动态栈溢出与死循环） */
void write_6_8_number(unsigned char x, unsigned char y, float number);    // 3位小数
void write_6_8_number_f1(unsigned char x, unsigned char y, float number); // 1位小数

/* 智能车主控专用封装应用接口 */
void display_6_8_string(unsigned char x, unsigned char y, char ch[]);
void display_6_8_number(unsigned char x, unsigned char y, float number);    // 自动去尾随0
void display_6_8_number_f1(unsigned char x, unsigned char y, float number); // 推荐速度/PID刷新
void display_6_8_number_pro(unsigned char x, unsigned char y, float number);// 强制带+/-号对齐显示

#endif