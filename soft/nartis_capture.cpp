#include "nartis_capture.h"

void parse_meter_page(uint32_t addr, uint8_t* p, MeterData &m)
{
    switch (addr) {

        // ===== 0x060000 — T, T1 and T2 Energy =====
        case 0x060000:
            m.total = (*(uint32_t*)(p + 0xB4)) / 1000.0;
            m.t1    = (*(uint32_t*)(p + 0xB8)) / 1000.0;
            m.t2    = (*(uint32_t*)(p + 0xBC)) / 1000.0;
            break;

        // ===== 0x060100 — Voltage, Current, Power =====
        case 0x060100:
            m.voltage = (*(uint16_t*)(p + 0x90)) / 10.0;
            m.current = (*(uint16_t*)(p + 0x98)) / 1000.0;
            m.power   = (*(uint16_t*)(p + 0xAC));
            break;

        // ===== 0x060200 — Frequency =====
        case 0x060200:
            m.freq = (*(uint16_t*)(p + 0x04)) / 100.0;
            break;
    }
}

void NartisD101::begin()
{
    i2s_parallel_config_t cfg;

    dma_desc_init(CAPTURE_SIZE);

    // Limitations for ESP32 pins
    // GPIO01 used for UART 0 RX, able to use it if you select different UART port (1,2) as OLS_Port
    // GPIO03 used for UART 0 TX
    // GPIO06 used for SCK, bootloop,
    // GPIO07 used for SDO, bootloop
    // GPIO8 used for SDI, bootloop
    // GPIO9 lead SW_CPU_RESET on WROOVER module
    // GPI10 lead SW_CPU_RESET on WROOVER module
    // GPIO11 used for CMD, bootloop
    // GPIO16 is UART2 RX
    // GPIO17 is UART2 TX
    // GPIO 20,24,28,29,30,31 results bootloop

    //One input for SPI bus
    //DI
    cfg.gpio_bus[0] = CH0_PIN;
    
    //CLK
    cfg.gpio_clk_in = CLK_IN;

    // cfg.bits = I2S_PARALLEL_BITS_8; //Not ready yet
    cfg.bits = I2S_PARALLEL_BITS_16;
    cfg.clkspeed_hz = 2 * 1000 * 1000; //Resulting pixel clock = 1MHz
    cfg.buf = &bufdesc;

    i2s_parallel_setup(&cfg);

    dma_elem_t cur;

    //Serial debug information
    //Serial_Debug_Port.println("---------------- Entire DMA zone parameters ---------------");
    //Serial_Debug_Port.printf("One element [dma_elem_t] size in DAM desk = %d\n", sizeof(cur));
 
    //for (int k = 0; k < s_state->dma_desc_count; k++) {
    //    Serial_Debug_Port.printf("DMA desk size #%d = %d bytes\n", k, s_state->dma_desc[k].size);
    //    Serial_Debug_Port.printf("Samples volume in one DMA desk #%d = %d\n", k, s_state->dma_desc[k].length);        
   // }

    //Serial_Debug_Port.println("--------------------------------------------------------");    
}

MeterData NartisD101::get_meter_data()
{   

  MeterData meter;
  
  channels_to_read = CAPTURE_CHANNELS;  
  readCount = CAPTURE_SAMPLES;
  
  uint8_t* flashflow = capture_data();

  //We get pages of data recorded in FLASH and extract metrics for MQTT
  //0x02 is the write command in FLASH, followed by the page address.
  for (int i = 0; i < readCount - 4; i++) {
    if (flashflow[i] == 0x02) {
      uint32_t addr = (flashflow[i+1] << 16) | (flashflow[i+2] << 8) | flashflow[i+3];
      parse_meter_page(addr, &flashflow[i+4], meter);
    }    
  }
  return meter;
}

uint8_t* NartisD101::capture_data()
{

  //To do. It needs to be converted to a dynamic array, where the size is equal to readCount
  static uint8_t flashflow[13600];
  
  int out_idx = 0;
  int bit_count = 0;
  uint8_t current_byte = 0;        
  uint32_t sample_index = 0;

  Serial_Debug_Port.printf("Capturing for %d samples started....\n", readCount);    

  start_dma_capture();
  yield();
  I2S0.conf.rx_start = 1;
  delay(100); // this delay is strictly need for error free capturing...

  while (!s_state->dma_done) {
    delay(100);
  }  

  yield();
  
  Serial_Debug_Port.println("Capturing samples complete");

  //Serial debug information
  //Serial_Debug_Port.println("============= Captuerd data =============");

  //We collect all the DMA desks into a single array of decrypted data from the MCU stream->FLASH
  for (int b = 0; b < s_state->dma_desc_count; b++) {

    dma_elem_t* buf = s_state->dma_buf[b];

    for (int i = 0; i < s_state->dma_sample_per_desc; i++) {

      // === sample1 ===
      uint8_t sample_bit = buf[i].sample1 & 1;

      //Global first sample1 skip
      if (sample_index != 0) {  
        current_byte = (current_byte << 1) | sample_bit;
        bit_count++;

        if (bit_count == 8) {
            flashflow[out_idx++] = current_byte;
            //Serial_Debug_Port.printf("%02X\n", current_byte);
            current_byte = 0;
            bit_count = 0;
        }
      }

      sample_index++;

      // === sample2 ===
      sample_bit = buf[i].sample2 & 1;

      current_byte = (current_byte << 1) | sample_bit;
      bit_count++;

      if (bit_count == 8) {
        flashflow[out_idx++] = current_byte;
        //Serial_Debug_Port.printf("%02X\n", current_byte);
        current_byte = 0;
        bit_count = 0;
      }

      sample_index++;
    }
  }  

  //Serial_Debug_Port.println("==============================================");
  return flashflow;
}

void NartisD101::gpio_setup_in(int gpio, int sig, int inv)
{
  if (gpio == -1) return; 
  
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[gpio], PIN_FUNC_GPIO);
  gpio_set_direction((gpio_num_t)gpio, (gpio_mode_t)GPIO_MODE_DEF_INPUT);
  gpio_matrix_in(gpio, sig, false);

  Serial_Debug_Port.printf("GPIO %d ready for input data\n", gpio);
}
