// ========================================
// Serial Debug Configuration
// ========================================

#define Serial_Debug_Port Serial
#define Serial_Debug_Port_Baud 115200

// =============================
// Settings for SPI bus capture
// =============================
/*
* Determining the buffer size for reading - here the maximum number of "raw" samples is determined
* One buffer entry (dma_elem_t;) contains four 8-bit samples (sample2, unused2, sample1, unused1) or two 16-bit values val2 and val1 or one 32-bit val
* Two samples are working (sample2 and sample1), and two are empty (unused2 and unused1)
* The order of samples sample2, unused2, sample1, unused1 is not chosen here by chance, this is how it is read from the stream and written to the DMA buffer
* In our case, the number of samples is the number of BITS (one sample = one bit -> 0 or 1) that must be stored in the buffer when reading the data stream from FLASH memory
* 
* In total, 1720 bytes of data are usually read from the SPI data stream between the HT6027 MCU and the EN25S80 FLASH, which is 1720 *8 = 13760 bits (end samples)
* It turns out that to read the SPI bus from Nartis-D101, you need 1720 values * 8 (samples per final byte) * 2 (2 bytes require 4 "useful" records) = 27520 bytes for the entire buffer
* Since the size of one buffer is 4000 bytes, 7 buffers are needed for guaranteed reading, which is 28000 bytes
*/

#define CAPTURE_SIZE      28000 //Total number of samples

//We read one SPI channel -> DI EN25S80
#define CAPTURE_CHANNELS  1     

//To read FLASH from Nartis-D101, you need 1720 values * 8 (samples per byte)
//Rounded up, this is 1700 * 8 = 13,600 samples
#define CAPTURE_SAMPLES   13600

// ==============================
// Pin Assignments
// ==============================

#define CH0_PIN   23  //DI (getting data from Flash) -> connecting to the 5 pin EN25S80
#define CLK_IN    18  //CLK -> connecting to the 6 pin EN25S80
