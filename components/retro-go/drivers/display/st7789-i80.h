#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <esp_heap_caps.h>

static esp_lcd_i80_bus_handle_t i80_bus;
static esp_lcd_panel_io_handle_t panel_io;
static esp_lcd_panel_handle_t panel;
static uint16_t *i80_buffers[1];
static QueueHandle_t i80_free_buffers;
static QueueHandle_t i80_submitted_buffers;
static int current_window_left;
static int current_window_top;
static int current_window_width;
static int current_window_height;

static bool IRAM_ATTR i80_color_done_cb(esp_lcd_panel_io_handle_t io,
                                        esp_lcd_panel_io_event_data_t *edata,
                                        void *context)
{
    (void)io;
    (void)edata;
    (void)context;
    uint16_t *buffer;
    if (xQueueReceiveFromISR(i80_submitted_buffers, &buffer, NULL) == pdTRUE)
        xQueueSendFromISR(i80_free_buffers, &buffer, NULL);
    return false;
}

static void lcd_set_backlight(float percent)
{
    float level = RG_MIN(RG_MAX(percent / 100.f, 0), 1.f);
    uint32_t duty = 0x1FFF * level;
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    if (err == ESP_OK)
        err = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (err != ESP_OK)
        RG_LOGE("failed setting backlight to %d%% (0x%02X)\n", (int)(100 * level), err);
}

static void lcd_set_window(int left, int top, int width, int height)
{
    static int window_left = -1, window_top = -1, window_right = -1, window_bottom = -1;
    int right = left + width - 1;
    int bottom = top + height - 1;

    if (window_left == left && window_top == top && window_right == right && window_bottom == bottom)
        return;

    window_left = left;
    window_top = top;
    window_right = right;
    window_bottom = bottom;
    current_window_left = left;
    current_window_top = top;
    current_window_width = width;
    current_window_height = height;
}

static inline uint16_t *lcd_get_buffer(size_t length)
{
    (void)length;
    uint16_t *buffer;
    xQueueReceive(i80_free_buffers, &buffer, portMAX_DELAY);
    return buffer;
}

static inline void lcd_send_buffer(uint16_t *buffer, size_t length)
{
    if (length > 0)
    {
        xQueueSend(i80_submitted_buffers, &buffer, portMAX_DELAY);
        int height = length / current_window_width;
        if (height < 1)
            height = 1;
        esp_err_t err = esp_lcd_panel_draw_bitmap(panel,
            current_window_left, current_window_top,
            current_window_left + current_window_width,
            current_window_top + height, buffer);
        if (err != ESP_OK)
        {
            uint16_t *unused;
            xQueueReceive(i80_submitted_buffers, &unused, portMAX_DELAY);
            xQueueSend(i80_free_buffers, &unused, portMAX_DELAY);
            RG_LOGE("lcd draw bitmap failed: %d\n", err);
        }
        else
        {
            uint16_t *returned_buffer;
            xQueueReceive(i80_free_buffers, &returned_buffer, portMAX_DELAY);
            RG_ASSERT(returned_buffer == buffer, "lcd dma buffer mismatch");
            xQueueSend(i80_free_buffers, &returned_buffer, portMAX_DELAY);
        }
    }
    else
    {
        xQueueSend(i80_free_buffers, &buffer, portMAX_DELAY);
    }
}

static void lcd_sync(void)
{
    esp_lcd_panel_io_tx_param(panel_io, 0x00, NULL, 0);
}

static void lcd_init(void)
{
    gpio_config_t rd_config = {
        .pin_bit_mask = 1ULL << RG_GPIO_LCD_RD,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&rd_config);
    gpio_set_level(RG_GPIO_LCD_RD, 1);

    i80_free_buffers = xQueueCreate(1, sizeof(uint16_t *));
    i80_submitted_buffers = xQueueCreate(1, sizeof(uint16_t *));
    for (size_t i = 0; i < 1; i++)
    {
        i80_buffers[i] = (uint16_t *)heap_caps_aligned_alloc(64, LCD_BUFFER_LENGTH * sizeof(uint16_t),
                                                               MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        RG_ASSERT(i80_buffers[i] != NULL, "lcd framebuffer allocation failed");
        uint16_t *buffer = i80_buffers[i];
        xQueueSend(i80_free_buffers, &buffer, portMAX_DELAY);
    }

#ifdef RG_GPIO_LCD_BCKL
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    RG_ASSERT(ledc_timer_config(&timer_config) == ESP_OK, "backlight timer init failed");

    ledc_channel_config_t channel_config = {
        .gpio_num = RG_GPIO_LCD_BCKL,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    RG_ASSERT(ledc_channel_config(&channel_config) == ESP_OK, "backlight channel init failed");

    gpio_config_t backlight_config = {
        .pin_bit_mask = 1ULL << RG_GPIO_LCD_BCKL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&backlight_config);
    gpio_set_level(RG_GPIO_LCD_BCKL, 1);
#endif

    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = RG_GPIO_LCD_DC,
        .wr_gpio_num = RG_GPIO_LCD_WR,
        .data_gpio_nums = {
            RG_GPIO_LCD_D0, RG_GPIO_LCD_D1, RG_GPIO_LCD_D2, RG_GPIO_LCD_D3,
            RG_GPIO_LCD_D4, RG_GPIO_LCD_D5, RG_GPIO_LCD_D6, RG_GPIO_LCD_D7,
        },
        .bus_width = 8,
        .max_transfer_bytes = LCD_BUFFER_LENGTH * 2,
        .psram_trans_align = 64,
        .sram_trans_align = 64,
        .clk_src = LCD_CLK_SRC_PLL160M,
    };
    RG_ASSERT(esp_lcd_new_i80_bus(&bus_config, &i80_bus) == ESP_OK, "i80 bus init failed");

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = RG_GPIO_LCD_CS,
        .pclk_hz = 16 * 1000 * 1000,
        .trans_queue_depth = 20,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {.swap_color_bytes = 0},
        .on_color_trans_done = i80_color_done_cb,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    RG_ASSERT(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &panel_io) == ESP_OK, "i80 panel io init failed");

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = RG_GPIO_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    RG_ASSERT(esp_lcd_new_panel_st7789(panel_io, &panel_config, &panel) == ESP_OK, "st7789 init failed");

    esp_lcd_panel_reset(panel);
    esp_lcd_panel_init(panel);

    esp_lcd_panel_io_tx_param(panel_io, 0x11, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(120));
    esp_lcd_panel_io_tx_param(panel_io, 0x3A, (uint8_t[]) {0x05}, 1);
    esp_lcd_panel_io_tx_param(panel_io, 0xB2, (uint8_t[]) {0x0B, 0x0B, 0x00, 0x33, 0x33}, 5);
    esp_lcd_panel_io_tx_param(panel_io, 0xB7, (uint8_t[]) {0x75}, 1);
    esp_lcd_panel_io_tx_param(panel_io, 0xBB, (uint8_t[]) {0x28}, 1);
    esp_lcd_panel_io_tx_param(panel_io, 0xC0, (uint8_t[]) {0x2C}, 1);
    esp_lcd_panel_io_tx_param(panel_io, 0xC2, (uint8_t[]) {0x01}, 1);
    esp_lcd_panel_io_tx_param(panel_io, 0xC3, (uint8_t[]) {0x1F}, 1);
    esp_lcd_panel_io_tx_param(panel_io, 0xC6, (uint8_t[]) {0x13}, 1);
    esp_lcd_panel_io_tx_param(panel_io, 0xD0, (uint8_t[]) {0xA7}, 1);
    esp_lcd_panel_io_tx_param(panel_io, 0xD0, (uint8_t[]) {0xA4, 0xA1}, 2);
    esp_lcd_panel_io_tx_param(panel_io, 0xD6, (uint8_t[]) {0xA1}, 1);
    esp_lcd_panel_io_tx_param(panel_io, 0xE0,
        (uint8_t[]) {0xF0, 0x05, 0x0A, 0x06, 0x06, 0x03, 0x2B, 0x32, 0x43, 0x36, 0x11, 0x10, 0x2B, 0x32}, 14);
    esp_lcd_panel_io_tx_param(panel_io, 0xE1,
        (uint8_t[]) {0xF0, 0x08, 0x0C, 0x0B, 0x09, 0x24, 0x2B, 0x22, 0x43, 0x38, 0x15, 0x16, 0x2F, 0x37}, 14);

    esp_lcd_panel_invert_color(panel, true);
    esp_lcd_panel_swap_xy(panel, true);
    esp_lcd_panel_mirror(panel, false, true);
    esp_lcd_panel_set_gap(panel, 0, 35);
    esp_lcd_panel_disp_on_off(panel, true);
}

static void lcd_deinit(void)
{
    lcd_set_backlight(0);
    if (panel)
        esp_lcd_panel_del(panel);
    if (panel_io)
        esp_lcd_panel_io_del(panel_io);
    if (i80_bus)
        esp_lcd_del_i80_bus(i80_bus);
}
