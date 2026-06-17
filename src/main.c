#include "ch32fun.h"
#include "i2c_slave.h"
#include <stdio.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Constants and Structures
// -----------------------------------------------------------------------------

#define NVM_FLASH_ADDRESS 0x08003FC0
#define MAX_ADC_CHANNELS 9  // 8 external channels (AIN0..7) + 1 internal reference (AIN9/channel 8)

// Union for NVM storage (exactly 64 bytes/16 words to match page programming size)
struct NVMConfig {
    uint16_t magic_header;    // 0x55AA
    uint8_t  device_address;  // 7-bit I2C address
    uint8_t  config_register; // FILTER[7:4] and CH_MASK[3:0]
    uint16_t alarm_limits[16]; // High/Low limits for 8 channels (High index 2*ch, Low index 2*ch + 1)
};

union NVMPage {
    struct NVMConfig config;
    uint32_t words[16];
};

// -----------------------------------------------------------------------------
// Global Variables
// -----------------------------------------------------------------------------

volatile uint8_t i2c_registers[256];
volatile uint16_t adc_buffer[MAX_ADC_CHANNELS];
static uint32_t filtered_adc[8] = {0};

volatile uint8_t current_alerts = 0;

volatile bool config_changed = false;
volatile bool save_settings_requested = false;
volatile bool reset_requested = false;

volatile uint32_t *nvm_flash_ptr = (volatile uint32_t *)NVM_FLASH_ADDRESS;

// -----------------------------------------------------------------------------
// Function Declarations
// -----------------------------------------------------------------------------

void load_nvm_settings(void);
void save_nvm_settings(void);
void initialize_gpios(void);
void initialize_adc_dma(void);
void configure_adc_sequence(uint8_t num_channels);
void initialize_filter_values(uint8_t num_channels);
void set_alert_pin_asserted(bool asserted);
uint8_t get_active_channel_count(void);
void onWrite(uint8_t reg, uint8_t length);

// -----------------------------------------------------------------------------
// Main Application
// -----------------------------------------------------------------------------

int main(void) {
    SystemInit();
    
    // Wait for power supply stabilization
    Delay_Ms(100);
    
    printf("\r\n\n=== CH32V003 I2C ADC Module ===\n");
    
    // 1. Load configuration from Flash or defaults
    load_nvm_settings();
    
    // 2. Initialize GPIOs
    initialize_gpios();
    
    // 3. Initialize ADC and DMA
    initialize_adc_dma();
    
    // 4. Configure active channels sequence
    uint8_t initial_ch_count = get_active_channel_count();
    configure_adc_sequence(initial_ch_count);
    initialize_filter_values(initial_ch_count);
    
    // 5. Initialize I2C Slave
    uint8_t slave_addr = i2c_registers[0x01];
    printf("Starting I2C Slave at address: 0x%02X\n", slave_addr);
    SetupI2CSlave(slave_addr, i2c_registers, sizeof(i2c_registers), onWrite, NULL, false);
    
    // Timebase variables for non-blocking 1ms loop
    uint32_t last_tick = SysTick->CNT;
    uint32_t tick_interval = FUNCONF_SYSTEM_CORE_CLOCK / 1000; // 24000 cycles at 24MHz
    
    printf("Initialization complete. Entering main loop...\n");
    
    while (1) {
        uint32_t current_tick = SysTick->CNT;
        
        // 1ms Non-blocking Execution Window
        if (current_tick - last_tick >= tick_interval) {
            // Check for counter fell behind (e.g., during Flash operations which take ~6ms)
            if (current_tick - last_tick > tick_interval * 10) {
                last_tick = current_tick;
            } else {
                last_tick += tick_interval;
            }
            
            uint8_t num_channels = get_active_channel_count();
            
            // A. Copy raw ADC values from DMA circular buffer atomically
            uint16_t local_raw[MAX_ADC_CHANNELS];
            for (int i = 0; i < num_channels + 1; i++) {
                local_raw[i] = adc_buffer[i];
            }
            
            // B. Calculate VDD from internal bandgap reference (scanned at local_raw[num_channels])
            uint32_t raw_bg = local_raw[num_channels];
            uint32_t vdd_mv = 3300; // Default fallback (3.3V)
            if (raw_bg > 0) {
                // VDD = 1200 mV * 1023 / raw_bg
                vdd_mv = (1200 * 1023) / raw_bg;
            }
            
            // Update VDD registers in Big-Endian format
            i2c_registers[0x02] = (vdd_mv >> 8) & 0xFF;
            i2c_registers[0x03] = vdd_mv & 0xFF;
            
            // C. Apply Exponential Moving Average (EMA) filter
            uint8_t alpha = (i2c_registers[0x00] >> 4) & 0x0F;
            for (int i = 0; i < num_channels; i++) {
                uint32_t raw = local_raw[i];
                if (alpha == 0) {
                    filtered_adc[i] = raw;
                } else {
                    filtered_adc[i] = ((alpha * raw) + ((16 - alpha) * filtered_adc[i])) >> 4;
                }
                
                // Write filtered raw value to RAW_CHx register (Big-Endian)
                uint16_t raw_val = filtered_adc[i] & 0x03FF;
                i2c_registers[0x04 + i * 4] = (raw_val >> 8) & 0xFF;
                i2c_registers[0x05 + i * 4] = raw_val & 0xFF;
            }
            
            // D. Convert to millivolts and check alarm thresholds
            for (int i = 0; i < num_channels; i++) {
                uint32_t mv = (filtered_adc[i] * vdd_mv) / 1023;
                
                // Update MV_CHx register (Big-Endian)
                i2c_registers[0x06 + i * 4] = (mv >> 8) & 0xFF;
                i2c_registers[0x07 + i * 4] = mv & 0xFF;
                
                // Read limits (Big-Endian)
                uint16_t high_limit = ((uint16_t)i2c_registers[0x24 + i * 4] << 8) | i2c_registers[0x25 + i * 4];
                uint16_t low_limit  = ((uint16_t)i2c_registers[0x26 + i * 4] << 8) | i2c_registers[0x27 + i * 4];
                
                // Sticky latching alarms
                if (mv > high_limit || mv < low_limit) {
                    current_alerts |= (1 << i);
                }
            }
            
            // Update ALERT_STATUS register and hardware pin
            i2c_registers[0x44] = current_alerts;
            set_alert_pin_asserted(current_alerts != 0);
        }
        
        // Idle/Deferred operations outside 1ms timing-critical window
        
        // Handle I2C address or scan count changes
        if (config_changed) {
            config_changed = false;
            uint8_t new_ch_count = get_active_channel_count();
            printf("Config updated: filter alpha = %d, scan channels = %d\n", 
                   (i2c_registers[0x00] >> 4) & 0x0F, new_ch_count);
            configure_adc_sequence(new_ch_count);
            initialize_filter_values(new_ch_count);
        }
        
        // Handle Flash Save settings request
        if (save_settings_requested) {
            save_settings_requested = false;
            save_nvm_settings();
        }
        
        // Handle Soft Reset request
        if (reset_requested) {
            reset_requested = false;
            printf("Software reset requested. Rebooting...\n");
            Delay_Ms(10); // Wait for UART flush
            NVIC_SystemReset();
        }
    }
}

// -----------------------------------------------------------------------------
// Functions Implementation
// -----------------------------------------------------------------------------

uint8_t get_active_channel_count(void) {
    uint8_t mask = i2c_registers[0x00] & 0x0F;
    if (mask < 1) return 1;
    if (mask > 8) return 8;
    return mask;
}

void set_alert_pin_asserted(bool asserted) {
    // Clear pin configuration for PD2 (bits 8 to 11 in CFGLR)
    GPIOD->CFGLR &= ~(0xF << 8);
    
    if (asserted) {
        // Asserted: Output Open-Drain, 10MHz (0x5)
        GPIOD->CFGLR |= (0x5 << 8);
        GPIOD->BCR = (1 << 2); // Set output low
    } else {
        // De-asserted (released to High-Z)
        uint8_t num_channels = get_active_channel_count();
        if (num_channels >= 4) {
            // Scanned as AIN3: configure as Analog Input (0x0)
            // (Already cleared to 0, no addition needed)
        } else {
            // Not scanned: configure as Input Floating (0x4)
            GPIOD->CFGLR |= (0x4 << 8);
        }
    }
}

void initialize_gpios(void) {
    // Enable GPIO clocks
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
    
    // PA1 and PA2 to Analog Input: clear bits 4..7 (PA1) and 8..11 (PA2) of GPIOA->CFGLR
    GPIOA->CFGLR &= ~((0xF << 4) | (0xF << 8));
    
    // PC4 to Analog Input: clear bits 16..19 of GPIOC->CFGLR
    GPIOC->CFGLR &= ~(0xF << 16);
    
    // PD3, PD4, PD5, PD6 to Analog Input: clear respective bits in GPIOD->CFGLR
    // PD3: bits 12..15
    // PD4: bits 16..19
    // PD5: bits 20..23
    // PD6: bits 24..27
    GPIOD->CFGLR &= ~((0xF << 12) | (0xF << 16) | (0xF << 20) | (0xF << 24));
    
    // PC1 (SDA) and PC2 (SCL): configure as Output 10MHz Alternate Function Open Drain (0xD)
    GPIOC->CFGLR &= ~((0xF << 4) | (0xF << 8));
    GPIOC->CFGLR |= ((0xD << 4) | (0xD << 8));
    
    // Initialize ALERT pin state (de-asserted)
    set_alert_pin_asserted(false);
}

void initialize_adc_dma(void) {
    // Enable clocks
    RCC->APB2PCENR |= RCC_APB2Periph_ADC1;
    RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;
    
    // ADCCLK = 24 MHz => RCC_ADCPRE = 0: divide by 2
    RCC->CFGR0 &= ~(0x1F << 11);
    
    // Reset ADC1 to clear previous state
    RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;
    
    // Configure sampling times:
    // SAMPTR2: channels 0 to 7 to 241 cycles (value 7)
    ADC1->SAMPTR2 = 0xFFFFFF;
    // SAMPTR1: channel 8 (Vref) to 241 cycles (value 7)
    ADC1->SAMPTR1 = 0x07;
    
    // Turn on ADC
    ADC1->CTLR2 |= ADC_ADON;
    
    // Reset calibration
    ADC1->CTLR2 |= ADC_RSTCAL;
    while (ADC1->CTLR2 & ADC_RSTCAL);
    
    // Calibrate
    ADC1->CTLR2 |= ADC_CAL;
    while (ADC1->CTLR2 & ADC_CAL);
    
    // Configure DMA1 Channel 1 for ADC1 peripheral requests
    DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
    DMA1_Channel1->MADDR = (uint32_t)adc_buffer;
    DMA1_Channel1->CFGR =
        DMA_M2M_Disable |
        DMA_Priority_VeryHigh |
        DMA_MemoryDataSize_HalfWord |
        DMA_PeripheralDataSize_HalfWord |
        DMA_MemoryInc_Enable |
        DMA_Mode_Circular |
        DMA_DIR_PeripheralSRC;
        
    // Enable scanning and continuous mode with DMA
    ADC1->CTLR1 |= ADC_SCAN;
    ADC1->CTLR2 |= ADC_CONT | ADC_DMA | ADC_EXTSEL;
}

void configure_adc_sequence(uint8_t num_channels) {
    if (num_channels < 1) num_channels = 1;
    if (num_channels > 8) num_channels = 8;
    
    // Stop ADC and DMA
    ADC1->CTLR2 &= ~ADC_ADON;
    DMA1_Channel1->CFGR &= ~DMA_CFGR1_EN;
    
    Delay_Us(10); // Wait for current transfer to abort
    
    // Set sequence conversion count in RSQR1: (num_channels + 1 - 1) = num_channels
    ADC1->RSQR1 = (num_channels) << 20;
    
    uint32_t rsqr3 = 0;
    uint32_t rsqr2 = 0;
    
    // Map external channels sequentially (Rank 1..N correspond to Channels 0..N-1)
    for (int i = 0; i < num_channels; i++) {
        if (i < 6) {
            rsqr3 |= (i << (5 * i));
        } else {
            rsqr2 |= (i << (5 * (i - 6)));
        }
    }
    
    // Map internal 1.2V reference (Channel 8) as the last rank
    int internal_rank = num_channels;
    if (internal_rank < 6) {
        rsqr3 |= (8 << (5 * internal_rank));
    } else {
        rsqr2 |= (8 << (5 * (internal_rank - 6)));
    }
    
    ADC1->RSQR3 = rsqr3;
    ADC1->RSQR2 = rsqr2;
    
    // Set DMA buffer size
    DMA1_Channel1->CNTR = num_channels + 1;
    
    // Re-enable DMA
    DMA1_Channel1->CFGR |= DMA_CFGR1_EN;
    
    // Re-enable ADC and restart conversion
    ADC1->CTLR2 |= ADC_ADON;
    ADC1->CTLR2 |= ADC_SWSTART;
    
    // Update ALERT pin configuration based on new channel count
    set_alert_pin_asserted(current_alerts != 0);
}

void initialize_filter_values(uint8_t num_channels) {
    Delay_Ms(10); // Wait for first DMA scan cycle to finish
    for (int i = 0; i < num_channels; i++) {
        filtered_adc[i] = adc_buffer[i];
    }
}

void load_nvm_settings(void) {
    union NVMPage temp_buf;
    
    // Read 16 words (64 bytes) from Flash
    for (int i = 0; i < 16; i++) {
        temp_buf.words[i] = nvm_flash_ptr[i];
    }
    
    // Check if magic header matches
    if (temp_buf.config.magic_header == 0x55AA) {
        printf("NVM Config found in Flash. Loading settings...\n");
        i2c_registers[0x00] = temp_buf.config.config_register;
        i2c_registers[0x01] = temp_buf.config.device_address;
        
        for (int ch = 0; ch < 8; ch++) {
            uint16_t high = temp_buf.config.alarm_limits[ch * 2];
            uint16_t low  = temp_buf.config.alarm_limits[ch * 2 + 1];
            
            i2c_registers[0x24 + ch * 4] = (high >> 8) & 0xFF;
            i2c_registers[0x25 + ch * 4] = high & 0xFF;
            i2c_registers[0x26 + ch * 4] = (low >> 8) & 0xFF;
            i2c_registers[0x27 + ch * 4] = low & 0xFF;
        }
    } else {
        printf("No valid config in Flash. Using default settings...\n");
        i2c_registers[0x00] = 0x4F; // Default CONFIG: alpha = 4, scan 8 channels
        i2c_registers[0x01] = 0x24; // Default address: 0x24
        
        // Limits defaults: High = 0xFFFF, Low = 0x0000
        for (int ch = 0; ch < 8; ch++) {
            i2c_registers[0x24 + ch * 4] = 0xFF;
            i2c_registers[0x25 + ch * 4] = 0xFF;
            i2c_registers[0x26 + ch * 4] = 0x00;
            i2c_registers[0x27 + ch * 4] = 0x00;
        }
    }
}

void save_nvm_settings(void) {
    union NVMPage temp_buf;
    
    // Prepare settings structure
    temp_buf.config.magic_header = 0x55AA;
    temp_buf.config.device_address = i2c_registers[0x01];
    temp_buf.config.config_register = i2c_registers[0x00];
    
    for (int ch = 0; ch < 8; ch++) {
        uint16_t high = ((uint16_t)i2c_registers[0x24 + ch * 4] << 8) | i2c_registers[0x25 + ch * 4];
        uint16_t low  = ((uint16_t)i2c_registers[0x26 + ch * 4] << 8) | i2c_registers[0x27 + ch * 4];
        
        temp_buf.config.alarm_limits[ch * 2] = high;
        temp_buf.config.alarm_limits[ch * 2 + 1] = low;
    }
    
    // Pad remaining words with 0xFFFFFFFF
    for (int i = 10; i < 16; i++) {
        temp_buf.words[i] = 0xFFFFFFFF;
    }
    
    // 1. Dirty check: check if data is different from Flash content
    bool dirty = false;
    for (int i = 0; i < 16; i++) {
        if (nvm_flash_ptr[i] != temp_buf.words[i]) {
            dirty = true;
            break;
        }
    }
    
    if (!dirty) {
        printf("Settings are identical to Flash. Skipping write.\n");
        return;
    }
    
    printf("Settings changed. Writing to Flash page at 0x%08X...\n", NVM_FLASH_ADDRESS);
    
    // 2. Unlock Flash
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    FLASH->MODEKEYR = FLASH_KEY1;
    FLASH->MODEKEYR = FLASH_KEY2;
    
    // 3. Erase Page
    FLASH->CTLR = CR_PAGE_ER;
    FLASH->ADDR = NVM_FLASH_ADDRESS;
    FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
    while (FLASH->STATR & FLASH_STATR_BSY);
    
    // 4. Load Buffer and Program Page
    FLASH->CTLR = CR_PAGE_PG;
    FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
    FLASH->ADDR = NVM_FLASH_ADDRESS;
    
    for (int i = 0; i < 16; i++) {
        nvm_flash_ptr[i] = temp_buf.words[i];
        FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD;
        while (FLASH->STATR & FLASH_STATR_BSY);
    }
    
    FLASH->CTLR = CR_PAGE_PG | CR_STRT_Set;
    while (FLASH->STATR & FLASH_STATR_BSY);
    
    // 5. Lock Flash
    FLASH->CTLR |= FLASH_CTLR_LOCK;
    
    printf("Flash save operation complete.\n");
}

void onWrite(uint8_t reg, uint8_t length) {
    // Check if CONFIG (0x00) was written
    if (reg <= 0x00 && (reg + length) > 0x00) {
        config_changed = true;
    }
    
    // Check if ALERT_STATUS (0x44) was written
    if (reg <= 0x44 && (reg + length) > 0x44) {
        uint8_t written_val = i2c_registers[0x44];
        current_alerts &= ~written_val;
        i2c_registers[0x44] = current_alerts;
    }
    
    // Check if SAVE_SETTINGS (0xFE) was written
    if (reg <= 0xFE && (reg + length) > 0xFE) {
        if (i2c_registers[0xFE] == 0xAA) {
            save_settings_requested = true;
        }
        i2c_registers[0xFE] = 0x00;
    }
    
    // Check if RESET (0xFF) was written
    if (reg <= 0xFF && (reg + length) > 0xFF) {
        if (i2c_registers[0xFF] == 0x77) {
            reset_requested = true;
        }
        i2c_registers[0xFF] = 0x00;
    }
}

