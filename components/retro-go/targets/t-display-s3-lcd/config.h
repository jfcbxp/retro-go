// Target definition
#define RG_TARGET_NAME             "T-Display-S3-LCD"

// Storage
#define RG_STORAGE_ROOT             "/"
#define RG_STORAGE_FLASH_PARTITION  "vfs"

// Audio
#define RG_AUDIO_USE_INT_DAC        0
#define RG_AUDIO_USE_EXT_DAC        0

// Video: ST7789 on ESP32-S3 Intel 8080 8-bit parallel
#define RG_SCREEN_DRIVER            2
#define RG_SCREEN_WIDTH             320
#define RG_SCREEN_HEIGHT            170
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_PARTIAL_UPDATES   0
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}

// Input: two physical buttons found on the LilyGO board
#define RG_GAMEPAD_GPIO_MAP {\
    {RG_KEY_SELECT, .num = GPIO_NUM_0,  .pullup = 1, .level = 0},\
    {RG_KEY_START,  .num = GPIO_NUM_14, .pullup = 1, .level = 0},\
}
#define RG_GAMEPAD_VIRT_MAP {\
    {RG_KEY_MENU, .src = RG_KEY_SELECT | RG_KEY_START},\
}

// Parallel LCD pins
#define RG_GPIO_LCD_CS              GPIO_NUM_6
#define RG_GPIO_LCD_DC              GPIO_NUM_7
#define RG_GPIO_LCD_RST             GPIO_NUM_5
#define RG_GPIO_LCD_WR              GPIO_NUM_8
#define RG_GPIO_LCD_RD              GPIO_NUM_9
#define RG_GPIO_LCD_BCKL            GPIO_NUM_38
#define RG_GPIO_LCD_D0              GPIO_NUM_39
#define RG_GPIO_LCD_D1              GPIO_NUM_40
#define RG_GPIO_LCD_D2              GPIO_NUM_41
#define RG_GPIO_LCD_D3              GPIO_NUM_42
#define RG_GPIO_LCD_D4              GPIO_NUM_45
#define RG_GPIO_LCD_D5              GPIO_NUM_46
#define RG_GPIO_LCD_D6              GPIO_NUM_47
#define RG_GPIO_LCD_D7              GPIO_NUM_48
