#include "nartis_capture.h"

void NartisD101::start_dma_capture(void)
{
    s_state->dma_done = false;
    s_state->dma_desc_cur = 0;

    ESP_ERROR_CHECK(esp_intr_disable(s_state->i2s_intr_handle));
    i2s_conf_reset();

    //The beginning of the DMA desks
    I2S0.in_link.addr = (uint32_t)&s_state->dma_desc[0];
    I2S0.in_link.start = 1;
    I2S0.int_clr.val = I2S0.int_raw.val;

    I2S0.int_ena.val = 0;
    I2S0.int_ena.in_done = 1;

    I2S0.rx_eof_num = readCount / 2; // why /2 ???
    I2S0.int_ena.in_suc_eof = 1;
    I2S0.int_ena.rx_take_data = 1;
    
    //
    lldesc_t *pd = &s_state->dma_desc[s_state->dma_desc_count - 1];
    pd->eof = 1;
    pd->qe.stqe_next = 0x0;

    //DMA desk cleaning
    for (int i = 0; i < s_state->dma_desc_count; i++)
        memset(s_state->dma_buf[i], 0, s_state->dma_desc[i].size);

    //Enable the Interrupt
    ESP_ERROR_CHECK(esp_intr_enable(s_state->i2s_intr_handle));

}

void NartisD101::i2s_isr(void *arg)
{

    //DMA desk full
    if (I2S0.int_raw.in_done) {
        
        //Serial_Debug_Port.printf("Interrupt state when DMA filled out %d = 0x%02X\n", s_state->dma_desc_cur, I2S0.int_raw.val);
        s_state->dma_desc_cur++;
    }

    //Capture is complete
    if (I2S0.int_raw.in_suc_eof || I2S0.int_raw.rx_take_data || s_state->dma_desc_cur == s_state->dma_desc_count)
    {
        //Uncommenting this lines will cause WatchDog
        // Serial_Debug_Port.printf("DMA INT take_data? %d\n", I2S0.int_raw.rx_take_data);
        // Serial_Debug_Port.printf("DMA INT in_dscr_empty? %d\n", I2S0.int_raw.in_dscr_empty);
        // Serial_Debug_Port.printf("DMA INT in_done? %d\n", I2S0.int_raw.in_done);
        // Serial_Debug_Port.printf("DMA INT in_suc_eof? %d\n", I2S0.int_raw.in_suc_eof);
        // Serial_Debug_Port.printf("DMA INT rx_rempty? %d\n", I2S0.int_raw.rx_rempty);
        // Serial_Debug_Port.printf("dma_desc_cur %d\n", s_state->dma_desc_cur);

        // s_state->dma_desc_cur=0;
        
        esp_intr_disable(s_state->i2s_intr_handle);
        i2s_conf_reset();
        I2S0.conf.rx_start = 0;
        s_state->dma_done = true;
    }

    // vTaskDelay(1);
    I2S0.int_clr.val = I2S0.int_raw.val;
}

void NartisD101::dma_desc_deinit()
{
    ESP_ERROR_CHECK(esp_intr_disable(s_state->i2s_intr_handle));
    
    if (s_state->dma_buf) for (int i = 0; i < s_state->dma_desc_count; ++i) free(s_state->dma_buf[i]);

    free(s_state->dma_buf);
    free(s_state->dma_desc);
}

esp_err_t NartisD101::dma_desc_init(int raw_byte_size)
{
    
    Serial_Debug_Port.println("--------------- DMA zone initialization ---------------");

    s_state = (logic_analyzer_state_t *)malloc(sizeof(logic_analyzer_state_t));
    
    //We check that the total size of the raw buffer is divided into four parts without remainder (since one buffer element consists of four bytes)
    assert(raw_byte_size % 4 == 0);

    Serial_Debug_Port.printf("Required size for DMA zone: %d байт\n", raw_byte_size);
   
    //One DMA desk for now
    size_t dma_desc_count = 1;
    
    //DMA zone size
    size_t buf_size = raw_byte_size;

    //DMA desk size
    s_state->dma_buf_width = buf_size = 4000;
    
    //Qtty 16-bit values in DMA desk
    s_state->dma_val_per_desc = s_state->dma_buf_width / 2;
    
    //КолиQtty 8-bit samples in DMA desk
    s_state->dma_sample_per_desc = s_state->dma_val_per_desc / 2; // 4bytes has only 2bytes sample on 16bit mode.
    
    //Qtty of DMA desks
    s_state->dma_desc_count = dma_desc_count = raw_byte_size / 4000;

    Serial_Debug_Port.printf("Calculated size for DMA zone: %d bytes\n", buf_size * dma_desc_count);
    Serial_Debug_Port.printf("Calculated size for one DMA desk: %d bytes\n", buf_size);
    Serial_Debug_Port.printf("Calculated number of DMA desks: %d\n", dma_desc_count);
    
    //Allocating memory    
    s_state->dma_buf = (dma_elem_t **)malloc(sizeof(dma_elem_t *) * dma_desc_count);
    if (s_state->dma_buf == NULL)
    {
        Serial_Debug_Port.println("Insuficient memory for DMA zone!");
        return ESP_ERR_NO_MEM;
    }
        
    s_state->dma_desc = (lldesc_t *)malloc(sizeof(lldesc_t) * dma_desc_count);    
    if (s_state->dma_desc == NULL)
    {
        Serial_Debug_Port.println("Insuficient memory for DMA desk descriptors!");
        return ESP_ERR_NO_MEM;
    }
    
    size_t dma_sample_count = 0;
    
    for (int i = 0; i < dma_desc_count; ++i)
    {
        //Serial_Debug_Port.printf("Allocating DMA desk #%d with size of %d bytes\n", i, buf_size);
        
        dma_elem_t *buf = (dma_elem_t *)malloc(buf_size);
        
        if (buf == NULL)
        {
            Serial_Debug_Port.printf("Insuficient memory for DMA desk #%d!\n", i);
            return ESP_ERR_NO_MEM;
        } else {
            //Serial_Debug_Port.printf("Pointer to DMA desk succesifully received #%d -> %p\n", i, buf);
        }
        
        //Pointer to beginning of DMA desk
        s_state->dma_buf[i] = buf;        

        //DMA desk parameters
        lldesc_t *pd = &s_state->dma_desc[i];
        
        dma_sample_count += buf_size / 2; // indeed /4 because each sample is 4 bytes        
        
        //Length is a qtty useful samples = 1/2 of full DMA desk size
        pd->length = buf_size / 2;        
        
        //DMA desk size in bytes
        pd->size = buf_size;
        
        pd->owner = 1;
        pd->sosf = 1;
        pd->buf = (uint8_t *)buf;
        pd->offset = 0;
        pd->empty = 0;
        pd->eof = 0;
        
        //Pointer to next DMA desk
        pd->qe.stqe_next = &s_state->dma_desc[(i + 1) % dma_desc_count];
        
        //The end of DMA zone
        if (i + 1 == dma_desc_count)
        {
            pd->eof = 1;
            pd->qe.stqe_next = 0x0;
            // pd->eof = 0;
            // pd->qe.stqe_next = &s_state->dma_desc[0];
        }
    }
    
    s_state->dma_done = false;
    s_state->dma_sample_count = dma_sample_count;
    
    Serial_Debug_Port.printf("Maximum sample threshold in the entire DMA zone: %d\n", dma_sample_count);
    Serial_Debug_Port.println("---------- DMA zone initialization complete ----------");

    return ESP_OK;
}

void NartisD101::i2s_conf_reset()
{
    // Toggle some reset bits in LC_CONF register
    I2S0.lc_conf.in_rst = 1;
    I2S0.lc_conf.in_rst = 0;
    I2S0.lc_conf.ahbm_rst = 1;
    I2S0.lc_conf.ahbm_rst = 0;
    I2S0.lc_conf.ahbm_fifo_rst = 1;
    I2S0.lc_conf.ahbm_fifo_rst = 0;

    // Toggle some reset bits in CONF register
    I2S0.conf.rx_reset = 1;
    I2S0.conf.rx_reset = 0;
    I2S0.conf.rx_fifo_reset = 1;
    I2S0.conf.rx_fifo_reset = 0;
    I2S0.conf.tx_reset = 1;
    I2S0.conf.tx_reset = 0;
    I2S0.conf.tx_fifo_reset = 1;
    I2S0.conf.tx_fifo_reset = 0;
    while (I2S0.state.rx_fifo_reset_back)
    {
    }
}

void NartisD101::i2s_parallel_setup(const i2s_parallel_config_t *cfg)
{

    //Figure out which signal numbers to use for routing
    Serial_Debug_Port.printf("--------------- I2S bus initialization ---------------\n", 0);
    
    int sig_data_base, sig_clk;

    sig_data_base = I2S0I_DATA_IN0_IDX;
    sig_clk = I2S0I_WS_IN_IDX;
    //sig_clk = I2S0I_BCK_IN_IDX;

    // Route the signals
    gpio_setup_in(cfg->gpio_bus[0], sig_data_base, false); // D0
    gpio_setup_in(cfg->gpio_clk_in, sig_clk, false);

    gpio_matrix_in(0x38, I2S0I_V_SYNC_IDX, false);
    gpio_matrix_in(0x38, I2S0I_H_SYNC_IDX, false);
    gpio_matrix_in(0x38, I2S0I_H_ENABLE_IDX, false);

    // Enable and configure I2S peripheral
    periph_module_enable(PERIPH_I2S0_MODULE);

    // Initialize I2S dev
    //  Toggle some reset bits in LC_CONF register
    //  Toggle some reset bits in CONF register
    i2s_conf_reset();

    I2S0.conf.rx_slave_mod = 1; // Enable slave mode (sampling clock is external)
    I2S0.conf2.val = 0;         // Disable LCD mode
    I2S0.conf2.lcd_en = 1;      // Enable parallel mode
    I2S0.conf2.camera_en = 1;   // Use HSYNC/VSYNC/HREF to control sampling
    I2S0.clkm_conf.val = 0;     // Configure clock divider
    I2S0.clkm_conf.clka_en = 0; // select PLL_D2_CLK. Digital Multiplexer that select between APLL_CLK or PLL_D2_CLK.
    I2S0.clkm_conf.clkm_div_a = 1;
    I2S0.clkm_conf.clkm_div_b = 0;
    I2S0.clkm_conf.clkm_div_num = 4;
    I2S0.fifo_conf.dscr_en = 1; // FIFO will sink data to DMA

    // FIFO configuration
    I2S0.fifo_conf.rx_fifo_mod = 1; // SM_0A0B_0C0D = 1,
    I2S0.fifo_conf.rx_fifo_mod_force_en = 1;
    // dev->conf_chan.val = 0;
    I2S0.conf_chan.rx_chan_mod = 1;

    // Clear flags which are used in I2S serial mode
    I2S0.sample_rate_conf.rx_bits_mod = 0;
    I2S0.conf.rx_right_first = 1;
    I2S0.conf.rx_msb_right = 0;
    I2S0.conf.rx_msb_shift = 0;
    I2S0.conf.rx_mono = 1;
    I2S0.conf.rx_short_sync = 1;
    I2S0.timing.val = 0;

    I2S0.sample_rate_conf.val = 0;
    // Clear flags which are used in I2S serial mode
    I2S0.sample_rate_conf.rx_bits_mod = cfg->bits;
    // dev->sample_rate_conf.rx_bck_div_num = 16; //ToDo: Unsure about what this does...
    I2S0.sample_rate_conf.rx_bck_div_num = 1; // datasheet says this must be 2 or greater (but 1 seems to work)

    // this combination is 20MHz
    // dev->sample_rate_conf.tx_bck_div_num=1;
    // dev->clkm_conf.clkm_div_num=3; // datasheet says this must be 2 or greater (but lower values seem to work)

    // Allocate DMA descriptors
    i2s_state[0] = (i2s_parallel_state_t *)malloc(sizeof(i2s_parallel_state_t));
    i2s_parallel_state_t *st = i2s_state[0];

    s_state->dma_done = false;
    s_state->dma_desc_cur = 0;

    // esp_intr_disable(s_state->i2s_intr_handle);
    // i2s_conf_reset();

    Serial_Debug_Port.printf("---------- I2S bus initialization complete  ----------\n", s_state->dma_sample_count);

    I2S0.rx_eof_num = s_state->dma_sample_count;
    I2S0.in_link.addr = (uint32_t)&s_state->dma_desc[0];
    I2S0.in_link.start = 1;
    I2S0.int_clr.val = I2S0.int_raw.val;
    I2S0.int_ena.val = 0;
    I2S0.int_ena.in_done = 1;

    // Setup I2S DMA Interrupt
    esp_err_t err = esp_intr_alloc(ETS_I2S0_INTR_SOURCE,
                                   ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM,
                                   &i2s_wrapper, NULL, &s_state->i2s_intr_handle);


    //I2S0.conf.rx_start = 1;
}