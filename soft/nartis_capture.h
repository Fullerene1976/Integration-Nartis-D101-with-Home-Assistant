#include <Arduino.h>
#include "rom/lldesc.h"
#include "soc/i2s_struct.h"
#include "driver/periph_ctrl.h"
#include "esp32/rom/gpio.h"
#include "soc/io_mux_reg.h"
#include "soc/gpio_periph.h"

#include "nartis_config.h"

typedef struct {

  float total = 0;
  float t1 = 0;
  float t2 = 0;

  float voltage = 0;
  float current = 0;
  float power = 0;
  float freq = 0;
} MeterData;

typedef enum
{
    I2S_PARALLEL_BITS_8 = 8,
    I2S_PARALLEL_BITS_16 = 16,
    I2S_PARALLEL_BITS_32 = 32,
} i2s_parallel_cfg_bits_t;

typedef struct
{
    void *memory;
    size_t size;
} i2s_parallel_buffer_desc_t;

typedef struct
{
    //One input for SPI data and one for clock
    int8_t gpio_bus[1];    
    int8_t gpio_clk_in;
    
    int clkspeed_hz;
    i2s_parallel_cfg_bits_t bits;
    i2s_parallel_buffer_desc_t *buf;
    
} i2s_parallel_config_t;

typedef struct
{
    volatile lldesc_t *dmadesc;
    int desccount;
} i2s_parallel_state_t;

typedef union
{
    struct
    {
        uint8_t sample2;
        uint8_t unused2;
        uint8_t sample1;
        uint8_t unused1;
    };
    struct
    {
        uint16_t val2;
        uint16_t val1;
    };
    uint32_t val;
} dma_elem_t;

typedef struct
{
    //Pointer
    lldesc_t *dma_desc;
    
    //Data desks consisting of dma_elem_t
    //Pointer to pointer
    dma_elem_t **dma_buf;
    
    //Data collection completed
    bool dma_done;
    
    //Total number of DMA desks for data
    size_t dma_desc_count;
    
    //Number of DMA desks used
    size_t dma_desc_cur;    
    
    //Buffer size in bytes
    size_t dma_buf_width;
    
    //Total number of samples that can be recorded
    size_t dma_sample_count;
    
    //The number of 16-bit values in the buffer
    size_t dma_val_per_desc;
    
    //The number of 8-bit samples in the buffer
    size_t dma_sample_per_desc;
    
    //The interrupt handler
    intr_handle_t i2s_intr_handle;

} logic_analyzer_state_t;

void IRAM_ATTR i2s_wrapper(void *arg);

class NartisD101
{
public:
    void begin(void);
    MeterData get_meter_data(void);
    void i2s_isr(void *arg);

private:

    uint8_t channels_to_read = CAPTURE_CHANNELS;
    uint32_t readCount = CAPTURE_SAMPLES;

    i2s_parallel_buffer_desc_t bufdesc;

    i2s_parallel_state_t *i2s_state[2] = {NULL, NULL};
    logic_analyzer_state_t *s_state;

    //An array of data collected from samples
    byte* capture_data(void);

    void gpio_setup_in(int gpio, int sig, int inv);    
    
    esp_err_t dma_desc_init(int raw_byte_size);
    void dma_desc_deinit();    
    void start_dma_capture(void);    

    void i2s_conf_reset();
    void i2s_parallel_setup(const i2s_parallel_config_t *cfg);

};