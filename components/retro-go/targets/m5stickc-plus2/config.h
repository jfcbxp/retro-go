#define RG_TARGET_NAME             "M5STICKC PLUS2"

#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_FLASH_PARTITION  "vfs"

#define RG_AUDIO_USE_INT_DAC        0
#define RG_AUDIO_USE_EXT_DAC        0

#define RG_SCREEN_DRIVER            0
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             SPI_MASTER_FREQ_40M
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_WIDTH             240
#define RG_SCREEN_HEIGHT            135
#define RG_SCREEN_BUFFER_LENGTH    2040
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_ROTATION          3
#define RG_SCREEN_RGB_BGR           1
#define RG_SCREEN_OFFSET_X           40
#define RG_SCREEN_OFFSET_Y           52
#define RG_SCREEN_SLEEP_OUT_DELAY_MS 130
#define RG_SCREEN_BACKLIGHT_PWM_FREQ 256
#define RG_SCREEN_BACKLIGHT_PWM_CHANNEL 7
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}
#define RG_SCREEN_INIT() \
    ILI9341_CMD(0xB7, 0x35); \
    ILI9341_CMD(0xBB, 0x28); \
    ILI9341_CMD(0xC0, 0x0C); \
    ILI9341_CMD(0xC2, 0x01, 0xFF); \
    ILI9341_CMD(0xC3, 0x10); \
    ILI9341_CMD(0xC4, 0x20); \
    ILI9341_CMD(0xD0, 0xA4, 0xA1); \
    ILI9341_CMD(0xB0, 0x00, 0xC0); \
    ILI9341_CMD(0xE0, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17); \
    ILI9341_CMD(0xE1, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E); \
    ILI9341_CMD(0x21);

#define RG_GAMEPAD_GPIO_MAP {\
    {RG_KEY_UP,   .num = GPIO_NUM_39, .pullup = 1, .level = 0},\
    {RG_KEY_A,    .num = GPIO_NUM_37, .pullup = 1, .level = 0},\
    {RG_KEY_MENU, .num = GPIO_NUM_35, .pullup = 1, .level = 0},\
}

#define RG_GPIO_LCD_MISO            GPIO_NUM_NC
#define RG_GPIO_LCD_MOSI            GPIO_NUM_15
#define RG_GPIO_LCD_CLK             GPIO_NUM_13
#define RG_GPIO_LCD_CS              GPIO_NUM_5
#define RG_GPIO_LCD_DC              GPIO_NUM_14
#define RG_GPIO_LCD_RST             GPIO_NUM_12
#define RG_GPIO_LCD_BCKL            GPIO_NUM_27
#define RG_GPIO_POWER_HOLD          GPIO_NUM_4
