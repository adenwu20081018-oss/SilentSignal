#include "OLED.h"
#include "oledfont.h"
#include <math.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include "esp_rom_sys.h"

static void oled_pin_init(void);
static void oled_tx_bitbang(uint8_t ctrl, const uint8_t *data, size_t len);
static void oled_set_page_col(uint8_t page, uint8_t col);
static void Write_IIC_Data_Buffer(const uint8_t *buf, unsigned len);

uint8_t OLED_GRAM[128][8];

static inline void oled_i2c_delay(void)
{
    esp_rom_delay_us(OLED_SOFT_I2C_DELAY_US);
}

static esp_err_t oled_tx(uint8_t ctrl, const uint8_t *data, size_t len)
{
    if (len > 128) {
        return ESP_ERR_INVALID_SIZE;
    }
    oled_tx_bitbang(ctrl, data, len);
    return ESP_OK;
}

static void oled_gpio_init(void)
{
    oled_pin_init();
}
//OLED?????
//?????????.
//[0]0 1 2 3 ... 127
//[1]0 1 2 3 ... 127
//[2]0 1 2 3 ... 127
//[3]0 1 2 3 ... 127
//[4]0 1 2 3 ... 127
//[5]0 1 2 3 ... 127
//[6]0 1 2 3 ... 127
//[7]0 1 2 3 ... 127
/**********************************************
//IIC Start
**********************************************/
/**********************************************
//IIC Start
**********************************************/
void IIC_Start(void)
{
    OLED_SDIN_Set();
    OLED_SCLK_Set();
    oled_i2c_delay();
    OLED_SDIN_Clr();
    oled_i2c_delay();
    OLED_SCLK_Clr();
    oled_i2c_delay();
}

/**********************************************
//IIC Stop
**********************************************/
void IIC_Stop()
{
    OLED_SDIN_Clr();
    oled_i2c_delay();
    OLED_SCLK_Set();
    oled_i2c_delay();
    OLED_SDIN_Set();
    oled_i2c_delay();
    esp_rom_delay_us(OLED_SOFT_I2C_STOP_GAP_US);
}

void IIC_Wait_Ack(void)
{
    /* �ͷ� SDA���ӻ��ڵ� 9 �� SCL ��������Ӧ�� */
    OLED_SDIN_Set();
    oled_i2c_delay();
    OLED_SCLK_Set();
    oled_i2c_delay();
    OLED_SCLK_Clr();
    oled_i2c_delay();
}

/**********************************************
// IIC Write byte
**********************************************/

void Write_IIC_Byte(uint8_t IIC_Byte)
{
    uint8_t i;
    uint8_t m, da;
    da = IIC_Byte;
    OLED_SCLK_Clr();
    oled_i2c_delay();

    for (i = 0; i < 8; i++) {
        m = da & 0x80;
        if (m == 0x80) {
            OLED_SDIN_Set();
        } else {
            OLED_SDIN_Clr();
        }
        oled_i2c_delay();
        OLED_SCLK_Set();
        oled_i2c_delay();
        OLED_SCLK_Clr();
        oled_i2c_delay();
        da <<= 1;
    }
}

/**********************************************
// IIC Write Command
**********************************************/
static void oled_pin_init(void)
{
    gpio_config_t scl = {
        .pin_bit_mask = 1ULL << OLED_GPIO_SCL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config_t sda = {
        .pin_bit_mask = 1ULL << OLED_GPIO_SDA,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_hold_dis(OLED_GPIO_SCL);
    gpio_hold_dis(OLED_GPIO_SDA);
    gpio_reset_pin(OLED_GPIO_SCL);
    gpio_reset_pin(OLED_GPIO_SDA);
    gpio_config(&scl);
    gpio_config(&sda);
    gpio_sleep_sel_dis(OLED_GPIO_SCL);
    gpio_sleep_sel_dis(OLED_GPIO_SDA);
    gpio_set_drive_capability(OLED_GPIO_SCL, GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(OLED_GPIO_SDA, GPIO_DRIVE_CAP_3);
    gpio_set_level(OLED_GPIO_SCL, 1);
    gpio_set_level(OLED_GPIO_SDA, 1);
    printf("OLED SCL=%d SDA=%d\r\n", (int)OLED_GPIO_SCL, (int)OLED_GPIO_SDA);
}

static portMUX_TYPE s_i2c_lock = portMUX_INITIALIZER_UNLOCKED;
static uint8_t s_panel[8][128];

static void oled_tx_bitbang(uint8_t ctrl, const uint8_t *data, size_t len)
{
    size_t i;

    /* 位翻转期间不能被节拍中断，否则 SSD1306 会把半个字节当成命令，屏幕闪一下 */
    portENTER_CRITICAL(&s_i2c_lock);
    IIC_Start();
    Write_IIC_Byte(OLED_I2C_ADDR_8BIT_W);
    IIC_Wait_Ack();
    Write_IIC_Byte(ctrl);
    IIC_Wait_Ack();
    for (i = 0; i < len; i++) {
        Write_IIC_Byte(data[i]);
        IIC_Wait_Ack();
    }
    IIC_Stop();
    portEXIT_CRITICAL(&s_i2c_lock);
}

static void oled_blit_page(uint8_t page, const uint8_t *src)
{
    unsigned i = 0;

    page &= 7;
    while (i < 128) {
        unsigned start;
        unsigned n;
        while (i < 128 && src[i] == s_panel[page][i]) {
            i++;
        }
        if (i >= 128) {
            break;
        }
        start = i;
        while (i < 128 && src[i] != s_panel[page][i]) {
            i++;
        }
        n = i - start;
        oled_set_page_col(page, (uint8_t)(OLED_COLUMN_OFFSET + start));
        Write_IIC_Data_Buffer(&src[start], n);
        memcpy(&s_panel[page][start], &src[start], n);
    }
}

void Write_IIC_Command(uint8_t IIC_Command)
{
    oled_tx(0x00, &IIC_Command, 1);
}

/* SSD1306：多字节命令必须在同一 I2C 事务内紧跟 0x00 连续发出，否则 0x21/0x22/0x20 等会失效 */
static void Write_IIC_Command_Burst(const uint8_t *cmds, unsigned len)
{
    if (cmds == NULL || len == 0) {
        return;
    }
    oled_tx(0x00, cmds, len);
}

/* Page + 列起点，一次发出 3 字节命令（SSD1306 Page 模式） */
static void oled_set_page_col(uint8_t page, uint8_t col)
{
    uint8_t buf[3];
    if (col > 127) {
        col = 127;
    }
    page &= 7;
    buf[0] = (uint8_t)(0xB0 | page);
    buf[1] = (uint8_t)(0x10 | (col >> 4));
    buf[2] = (uint8_t)(0x00 | (col & 0x0F));
    Write_IIC_Command_Burst(buf, 3);
}

/**********************************************
// IIC Write Data
**********************************************/
void Write_IIC_Data(uint8_t IIC_Data)
{
    oled_tx(0x40, &IIC_Data, 1);
}

/* 连续写多字节显存（一次 I2C 事务），用于 Horizontal 模式整块写字模 */
static void Write_IIC_Data_Buffer(const uint8_t *buf, unsigned len)
{
    if (len == 0 || buf == NULL) {
        return;
    }
    oled_tx(0x40, buf, len);
}

static void oled_ssd1306_page_mode(void)
{
    const uint8_t pm[] = { 0x20, 0x02 };
    Write_IIC_Command_Burst(pm, 2);
}

/* 16 点高 ASCII：Horizontal 模式一次写满上下 8 行（多字节命令必须 burst） */
static void oled_char16_horizontal(uint8_t x, uint8_t page, const uint8_t *glyph16)
{
    uint16_t xs = (uint16_t)x + (uint16_t)OLED_COLUMN_OFFSET;
    uint8_t cs = (xs > 127) ? 127 : (uint8_t)xs;
    uint8_t ce = cs + 7;
    if (ce > 127) {
        ce = 127;
    }
    page &= 7;
    uint8_t p1 = (page < 7) ? (page + 1) : 7;

    {
        const uint8_t m0[] = { 0x20, 0x00 };
        uint8_t m1[3] = { 0x21, cs, ce };
        uint8_t m2[3] = { 0x22, page, p1 };
        Write_IIC_Command_Burst(m0, 2);
        Write_IIC_Command_Burst(m1, 3);
        Write_IIC_Command_Burst(m2, 3);
    }

    Write_IIC_Data_Buffer(glyph16, 16);

    oled_ssd1306_page_mode();
}

void OLED_WR_Byte(unsigned dat, unsigned cmd)
{
    if (cmd)
    {
        Write_IIC_Data(dat);
    }
    else
    {
        Write_IIC_Command(dat);
    }
}


/********************************************
// fill_Picture
********************************************/
void fill_picture(uint8_t fill_Data)
{
    uint8_t m;
    uint8_t line[128];

    memset(line, fill_Data, sizeof(line));
    for (m = 0; m < 8; m++) {
        oled_set_page_col(m, OLED_COLUMN_OFFSET);
        Write_IIC_Data_Buffer(line, sizeof(line));
    }
}

/* y 为页号 0~7（8 像素高为一页），非像素坐标 */
void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    uint16_t col = (uint16_t)x + (uint16_t)OLED_COLUMN_OFFSET;
    if (col > 127) {
        col = 127;
    }
    oled_set_page_col(y & 7, (uint8_t)col);
}

//????
//x:0~127
//y:0~63
//t:1 ??? 0,???
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t)   //???????????????????LCD??????OLED_Refresh_Gram();
{
    uint8_t pos, bx, temp=0;

    if (x>127||y>63)
        return;//??????????.

    pos = 7-y/8;            //????  ???????????????       ?????
    bx = y%8;
    temp = 1<<(7-bx);       //???????? 1 ??????y?????

    if (t)
        OLED_GRAM[x][pos] |= temp;
    else
        OLED_GRAM[x][pos] &= ~temp;
}

//????????LCD
void OLED_Refresh_Gram(void)
{
    uint8_t i, n;
    uint8_t line[128];

    for (i = 0; i < 8; i++) {
        for (n = 0; n < 128; n++) {
            line[n] = OLED_GRAM[n][i];
        }
        oled_set_page_col(i, OLED_COLUMN_OFFSET);
        Write_IIC_Data_Buffer(line, sizeof(line));
    }
}

//????
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) //???????????????????LCD??????OLED_Refresh_Gram();
{
    uint16_t t;
    int xerr=0, yerr=0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x=x2-x1; //????????????
    delta_y=y2-y1;
    uRow=x1;
    uCol=y1;

    if (delta_x>0)
        incx=1; //???????????
    else if (delta_x==0)
        incx=0;//?????
    else
    {
        incx=-1;
        delta_x=-delta_x;
    }

    if (delta_y>0)
        incy=1;
    else if (delta_y==0)
        incy=0;//????
    else
    {
        incy=-1;
        delta_y=-delta_y;
    }

    if (delta_x>delta_y)
        distance=delta_x; //????????????????
    else
        distance=delta_y;

    for (t=0; t<=distance+1; t++) //???????
    {
        OLED_DrawPoint(uRow, uCol, 1); //????
        xerr+=delta_x ;
        yerr+=delta_y ;

        if (xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }

        if (yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
}

//???
void draw_circle(uint8_t x0, uint8_t y0, uint8_t r) //???(x0,y0),??r
{
    uint8_t x;

    for (x=x0-r; x<=x0+r; x++)
    {
        uint8_t y_old;
        y_old=sqrt(pow(r, 2)-pow(x-x0, 2));
        OLED_DrawPoint(x, y_old+y0, 1);  //???
        OLED_DrawPoint(x, y0-y_old, 1); //???
    }
}

//??????????(x0,y0),??r
void draw_fill_circle(uint8_t x0, uint8_t y0, uint8_t r, uint8_t dot)
{
    uint8_t x = 0, y = 0, R = 0;

    for (x = x0-r; x <= x0+r; x++)
    {
        for (y = y0-r; y <= y0+r ; y++)
        {
            R = sqrt(pow(r, 2)-pow(x-x0, 2))+y0; //?????  x,y????

            if ((y >= y0 && y <= R) || (y < y0 && y >= 2*y0-R)|| dot == 0)   //???????? ???????
            {
                OLED_DrawPoint(y, x, dot);
            }
        }
    }
}


//????OLED???
void OLED_Display_On(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); //SET DCDC????
    OLED_WR_Byte(0X14, OLED_CMD); //DCDC ON
    OLED_WR_Byte(0XAF, OLED_CMD); //DISPLAY ON
}

//???OLED???
void OLED_Display_Off(void)
{
    OLED_WR_Byte(0X8D, OLED_CMD); //SET DCDC????
    OLED_WR_Byte(0X10, OLED_CMD); //DCDC OFF
    OLED_WR_Byte(0XAE, OLED_CMD); //DISPLAY OFF
}

//????????,??????,?????????????!??????????!!!
void OLED_Clear(void)
{
    uint8_t i;
    static const uint8_t zeros[128] = { 0 };

    for (i = 0; i < 8; i++) {
        oled_set_page_col(i, OLED_COLUMN_OFFSET);
        Write_IIC_Data_Buffer(zeros, sizeof(zeros));
    }
    memset(s_panel, 0, sizeof(s_panel));
}

void OLED_On(void)
{
    uint8_t i;
    uint8_t line[128];

    memset(line, 0xFF, sizeof(line));
    for (i = 0; i < 8; i++) {
        oled_set_page_col(i, OLED_COLUMN_OFFSET);
        Write_IIC_Data_Buffer(line, sizeof(line));
    }
}

//??????????????????,???????????
//x:0~127
//y:0~63
//mode:0,???????;1,???????
//size:??????? 16/12
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t Char_Size)
{
    unsigned char c = 0, i = 0;
    c = (unsigned char)(chr - ' ');
    if (c > 94) {
        c = 0;
    }

    if (x>Max_Column-1)
    {
        x=0;
        y=y+2;
    }

    if (Char_Size == 16) {
        uint8_t page = y & 7;
        if (page >= 7) {
            OLED_Set_Pos(x, page);
            for (i = 0; i < 8; i++) {
                OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
            }
        } else {
            oled_char16_horizontal(x, page, &F8X16[c * 16]);
        }
    } else
    {
        OLED_Set_Pos(x, y);
        Write_IIC_Data_Buffer(F6x8[c], 6);
    }
}

//m^n????
uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result=1;

    while (n--)result*=m;

    return result;
}

//???2??????
//x,y :???????
//len :?????????
//size:???????
//mode:?? 0,?????;1,??????
//num:???(0~4294967295);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size2)
{
    uint8_t t, temp;
    uint8_t enshow=0;

    for (t=0; t<len; t++)
    {
        temp=(num/oled_pow(10, len-t-1))%10;

        if (enshow==0&&t<(len-1))
        {
            if (temp==0)
            {
                OLED_ShowChar(x+(size2/2)*t, y, ' ', size2);
                continue;
            }
            else
                enshow=1;
        }

        OLED_ShowChar(x+(size2/2)*t, y, temp+'0', size2);
    }
}

void OLED_ShowLine16(uint8_t page, const char *text)
{
    uint8_t top[128];
    uint8_t bot[128];
    unsigned col;

    memset(top, 0, sizeof(top));
    memset(bot, 0, sizeof(bot));
    if (text == NULL) {
        text = "";
    }
    page &= 0xFE;
    if (page > 6) {
        page = 6;
    }

    for (col = 0; col < 16 && text[col] != '\0'; col++) {
        unsigned char c = (unsigned char)(text[col] - ' ');
        unsigned i;
        if (c > 94) {
            c = 0;
        }
        for (i = 0; i < 8; i++) {
            top[col * 8 + i] = F8X16[c * 16 + i];
            bot[col * 8 + i] = F8X16[c * 16 + 8 + i];
        }
    }

    oled_blit_page(page, top);
    oled_blit_page((uint8_t)(page + 1), bot);
}

void OLED_ShowLevel(uint8_t width)
{
    uint8_t top[128];
    uint8_t bot[128];
    unsigned i;

    if (width > 128) {
        width = 128;
    }
    for (i = 0; i < 128; i++) {
        if (i < width) {
            top[i] = 0xFF;
            bot[i] = 0xFF;
        } else {
            top[i] = 0x01;
            bot[i] = 0x80;
        }
    }
    oled_blit_page(6, top);
    oled_blit_page(7, bot);
}

//????????????
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t Char_Size)
{
    uint8_t j=0;

    while (chr[j] != '\0') {
        OLED_ShowChar(x, y, (uint8_t)chr[j], Char_Size);
        x += 8;

        if (x>120)
        {
            x=0;
            y+=2;
        }

        j++;
    }
}

//???????
void OLED_ShowCHinese(uint8_t x, uint8_t y, uint8_t no)
{
    uint16_t xs = (uint16_t)x + (uint16_t)OLED_COLUMN_OFFSET;
    uint8_t cs = (xs > 127) ? 127 : (uint8_t)xs;
    uint8_t ce = cs + 15;
    if (ce > 127) {
        ce = 127;
    }
    uint8_t page = y & 7;
    uint8_t p1 = (page < 7) ? (page + 1) : 7;
    uint8_t t;

    if (page >= 7) {
        OLED_Set_Pos(x, page);
        for (t = 0; t < 16; t++) {
            OLED_WR_Byte(Hzk[2 * no][t], OLED_DATA);
        }
        return;
    }

    {
        const uint8_t m0[] = { 0x20, 0x00 };
        uint8_t m1[3] = { 0x21, cs, ce };
        uint8_t m2[3] = { 0x22, page, p1 };
        Write_IIC_Command_Burst(m0, 2);
        Write_IIC_Command_Burst(m1, 3);
        Write_IIC_Command_Burst(m2, 3);
    }

    IIC_Start();
    Write_IIC_Byte(OLED_I2C_ADDR_8BIT_W);
    IIC_Wait_Ack();
    Write_IIC_Byte(0x40);
    IIC_Wait_Ack();
    for (t = 0; t < 16; t++) {
        Write_IIC_Byte(Hzk[2 * no][t]);
        IIC_Wait_Ack();
    }
    for (t = 0; t < 16; t++) {
        Write_IIC_Byte(Hzk[2 * no + 1][t]);
        IIC_Wait_Ack();
    }
    IIC_Stop();

    oled_ssd1306_page_mode();
}

/***********????????????????BMP??128??64?????????(x,y),x?????0??127??y???????0??7*****************/
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[])
{
    uint16_t j=0;
    uint8_t x, y;

    if (y1%8==0)
        y=y1/8;
    else
        y=y1/8+1;

    for (y=y0; y<y1; y++)
    {
        OLED_Set_Pos(x0, y);

        for (x=x0; x<x1; x++)
        {
            OLED_WR_Byte(BMP[j++], OLED_DATA);
        }
    }
}

void OLED_Fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot)
{
    uint16_t x, y;
    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            OLED_DrawPoint((uint8_t)x, (uint8_t)y, dot);
        }
    }
    OLED_Refresh_Gram();
}

void Picture(void)
{
}

/* SSD1306 128x64：多字节命令成组 burst（I2C 协议要求），单字节仍用 Write_IIC_Command */
void OLED_Init(void)
{
    static const uint8_t k_ae[] = { 0xAE };
    static const uint8_t k_mode_page[] = { 0x20, 0x02 };
    static const uint8_t k_d5[] = { 0xD5, 0x80 };
    static const uint8_t k_a8[] = { 0xA8, 0x3F };
    static const uint8_t k_d3[] = { 0xD3, 0x00 };
    static const uint8_t k_40[] = { 0x40 };
    static const uint8_t k_8d[] = { 0x8D, 0x14 };
    uint8_t k_a1_c8[2] = { 0xA1, (uint8_t)OLED_COM_SCAN_CMD };
    uint8_t k_da[] = { 0xDA, (uint8_t)OLED_SSD1306_COM_PINS };
    static const uint8_t k_81[] = { 0x81, 0xFF };
    static const uint8_t k_d9[] = { 0xD9, 0xF1 };
    static const uint8_t k_db[] = { 0xDB, 0x30 };
    static const uint8_t k_a6[] = { 0xA6 };
    static const uint8_t k_af[] = { 0xAF };

    oled_gpio_init();
    vTaskDelay(pdMS_TO_TICKS(100));

    Write_IIC_Command_Burst(k_ae, 1);
    Write_IIC_Command_Burst(k_mode_page, 2);
    Write_IIC_Command_Burst(k_d5, 2);
    Write_IIC_Command_Burst(k_a8, 2);
    Write_IIC_Command_Burst(k_d3, 2);
    Write_IIC_Command_Burst(k_40, 1);
    Write_IIC_Command_Burst(k_8d, 2);
    Write_IIC_Command_Burst(k_a1_c8, 2);
    Write_IIC_Command_Burst(k_da, 2);
    Write_IIC_Command_Burst(k_81, 2);
    Write_IIC_Command_Burst(k_d9, 2);
    Write_IIC_Command_Burst(k_db, 2);
    Write_IIC_Command_Burst(k_a6, 1);
    Write_IIC_Command_Burst(k_af, 1);
}


