#ifndef __OLED_H
#define __OLED_H

#include <stdint.h>
#include <driver/gpio.h>

/* ???? I2C ?????SCL=IO19??SDA=IO20???????????????? #define ????? */
#ifndef OLED_GPIO_SCL
#define OLED_GPIO_SCL GPIO_NUM_1
#endif
#ifndef OLED_GPIO_SDA
#define OLED_GPIO_SDA GPIO_NUM_2
#endif

/* SSD1306 д��ַ��0x78=0x3C<<1������ģ��Ϊ 0x3D ���Ϊ 0x7A */
#ifndef OLED_I2C_ADDR_8BIT_W
#define OLED_I2C_ADDR_8BIT_W 0x78
#endif

/* ���� I2C ÿ��������ʱ��΢�룩���������쳣ʱ�ɸ�Ϊ 6~10 */
#ifndef OLED_SOFT_I2C_DELAY_US
#define OLED_SOFT_I2C_DELAY_US 6
#endif
/* Stop 后总线空闲(µs)。整页突发后总 Stop 次数大减，可适当减小；不稳再加大 */
#ifndef OLED_SOFT_I2C_STOP_GAP_US
#define OLED_SOFT_I2C_STOP_GAP_US 4
#endif

/* 列起点：标准 SSD1306 12864 一般为 0；SH1106 或部分屏为 2（右侧被切、错位可试 0 或 2） */
#ifndef OLED_COLUMN_OFFSET
#define OLED_COLUMN_OFFSET 0
#endif

/* SSD1306：COM 引脚/扫描方向；多字节寄存器须在同一次 I2C 命令事务内连续发送（见 OLED.c Write_IIC_Command_Burst） */
#ifndef OLED_COM_SCAN_CMD
#define OLED_COM_SCAN_CMD 0xC8
#endif
#ifndef OLED_SSD1306_COM_PINS
#define OLED_SSD1306_COM_PINS 0x12
#endif

#define OLED_MODE   0
#define DisplaySIZE 16
#define XLevelL     0x00
#define XLevelH     0x10
#define Max_Column  128
#define Max_Row     64
#define Brightness  0xFF
#define X_WIDTH     128
#define Y_WIDTH     64

#define OLED_SCLK_Clr() gpio_set_level(OLED_GPIO_SCL, 0)
#define OLED_SCLK_Set() gpio_set_level(OLED_GPIO_SCL, 1)
#define OLED_SDIN_Clr() gpio_set_level(OLED_GPIO_SDA, 0)
#define OLED_SDIN_Set() gpio_set_level(OLED_GPIO_SDA, 1)

#define OLED_CMD  0
#define OLED_DATA 1

void OLED_WR_Byte(unsigned dat, unsigned cmd);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Init(void);
void OLED_Clear(void);
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t);
void OLED_Fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *p, uint8_t Char_Size);
/* 16 号字整行刷新：page 为起始页 0/2/4/6，一次写完上下两页 */
void OLED_ShowLine16(uint8_t page, const char *text);
/* 底部两页电平条，width 为 0~128 */
void OLED_ShowLevel(uint8_t width);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_Refresh_Gram(void);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void draw_circle(uint8_t x0, uint8_t y0, uint8_t r);
void draw_fill_circle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t dot);
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no);
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[]);
void fill_picture(uint8_t fill_Data);
void Picture(void);
void IIC_Start(void);
void IIC_Stop(void);
void Write_IIC_Command(uint8_t IIC_Command);
void Write_IIC_Data(uint8_t IIC_Data);
void Write_IIC_Byte(uint8_t IIC_Byte);
void IIC_Wait_Ack(void);

#endif
