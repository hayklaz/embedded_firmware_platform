/* 
 * File:   Main.c
 * Author: Hayk Lazaryan
 *
 * Created on March 12, 2026, 1:51am
 */

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>

// Standard libraries
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "BOARD.h"
#include "Uart.h"
#include "Protocol2.h"
#include "MessageIDs.h"
#include "ADCFilter.h"
#include "NonVolatileMemory.h"

/*******************************************************************************
 * Defines
 ******************************************************************************/

//Define number of channels and number of filters per channel
#define NUM_CHANNELS        4u
#define NUM_FILTERS         2u

//Each filter is 32 shorts = 64 bytes
#define FILTER_BYTES        64u

//Number of filtered samples kept for peak-to-peak LED mode
#define P2P_WINDOW          64u

//Scaling used by filter weights
#define FILTER_SCALE_SHIFT  15

//Switch registers
#define SW1_STATE()         PORTDbits.RD8
#define SW2_STATE()         PORTDbits.RD9
#define SW3_STATE()         PORTDbits.RD10
#define SW4_STATE()         PORTDbits.RD11

//For 100Hz app update clock
#define APP_UPDATE_TICKS    1562u

//For 1kHz sinewave update clock
#define SYNTH_UPDATE_TICKS  4999u

//Waveform settings
#define SYNTH_SAMPLE_RATE_HZ   1000u
#define SYNTH_TABLE_SIZE       256u
#define SYNTH_AMPLITUDE        450
#define SYNTH_DC_OFFSET        512

/*******************************************************************************
 * Variables and Data
 ******************************************************************************/

//Variable that stores filters for each channel and takes care of 2 filters per channel
static short storedFilters[NUM_CHANNELS][NUM_FILTERS][FILTERLENGTH];

//Circular buffers for each channel
static short sampleHistory[NUM_CHANNELS][FILTERLENGTH];
static uint8_t writeIndex = 0;

//Filtered-value history for LED peak-to-peak mode
static short p2pHistory[P2P_WINDOW];
static uint8_t p2pIndex = 0;
static uint8_t p2pCount = 0;

//Current switch-selected settings
static uint8_t currentChannel = 0;
static uint8_t currentFilter = 0;
static uint8_t currentP2PMode = 0;

//Last reported selection so we only notify when channel/filter changes
static uint8_t lastReportedChannel = 0xFF;
static uint8_t lastReportedFilter = 0xFF;

//Waveform state
static volatile unsigned int synthFrequency = 25u;
static volatile uint8_t synthEnabled = 0;
static volatile uint16_t synthPhase = 16384u;
static volatile uint16_t synthPhaseStep = 0;
static volatile short synthCurrentSample = SYNTH_DC_OFFSET;

//Sine value table
static const int16_t sineTable[SYNTH_TABLE_SIZE] = {
      0,   804,  1608,  2410,  3212,  4011,  4808,  5602,
   6393,  7179,  7962,  8739,  9512, 10278, 11039, 11793,
  12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
  18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
  23170, 23731, 24279, 24811, 25330, 25832, 26319, 26790,
  27245, 27683, 28106, 28510, 28898, 29268, 29621, 29955,
  30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
  32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
  32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285,
  32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
  30273, 29955, 29621, 29268, 28898, 28510, 28106, 27683,
  27245, 26790, 26319, 25832, 25330, 24811, 24279, 23731,
  23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868,
  18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
  12539, 11793, 11039, 10278,  9512,  8739,  7962,  7179,
   6393,  5602,  4808,  4011,  3212,  2410,  1608,   804,
      0,  -804, -1608, -2410, -3212, -4011, -4808, -5602,
  -6393, -7179, -7962, -8739, -9512,-10278,-11039,-11793,
 -12539,-13279,-14010,-14732,-15446,-16151,-16846,-17530,
 -18204,-18868,-19519,-20159,-20787,-21403,-22005,-22594,
 -23170,-23731,-24279,-24811,-25330,-25832,-26319,-26790,
 -27245,-27683,-28106,-28510,-28898,-29268,-29621,-29955,
 -30273,-30571,-30852,-31113,-31356,-31580,-31785,-31971,
 -32137,-32285,-32412,-32521,-32609,-32678,-32728,-32757,
 -32767,-32757,-32728,-32678,-32609,-32521,-32412,-32285,
 -32137,-31971,-31785,-31580,-31356,-31113,-30852,-30571,
 -30273,-29955,-29621,-29268,-28898,-28510,-28106,-27683,
 -27245,-26790,-26319,-25832,-25330,-24811,-24279,-23731,
 -23170,-22594,-22005,-21403,-20787,-20159,-19519,-18868,
 -18204,-17530,-16846,-16151,-15446,-14732,-14010,-13279,
 -12539,-11793,-11039,-10278, -9512, -8739, -7962, -7179,
  -6393, -5602, -4808, -4011, -3212, -2410, -1608,  -804
};

/*******************************************************************************
 * Helpers
 ******************************************************************************/

//Initializes Timer2 for app timing
static void AppTimerInit(void)
{
    T2CON = 0;
    TMR2 = 0;
    PR2 = APP_UPDATE_TICKS;
    T2CONbits.TCKPS = 0b111;   // 1:256
    IFS0bits.T2IF = 0;
    T2CONbits.ON = 1;
}

//Checks whether Timer2 period elapsed
static int AppTimerReady(void)
{
    if (IFS0bits.T2IF) {
        IFS0bits.T2IF = 0;
        return SUCCESS;
    }
    return ERROR;
}

//Initializes Timer3 for sinusoidal sampling
static void SynthTimerInit(void)
{
    T3CON = 0;
    TMR3 = 0;
    PR3 = SYNTH_UPDATE_TICKS;
    T3CONbits.TCKPS = 0b001;   // 1:8
    IFS0bits.T3IF = 0;
    T3CONbits.ON = 1;
}

//Checks whether Timer3 period elapsed
static int SynthTimerReady(void)
{
    if (IFS0bits.T3IF) {
        IFS0bits.T3IF = 0;
        return SUCCESS;
    }
    return ERROR;
}

//Sets switch pins as digital inputs
static void SwitchesInit(void)
{
    AD1PCFG = 0xFFFF;
    TRISDSET = (1u << 8) | (1u << 9) | (1u << 10) | (1u << 11);
}

//Reads SW1 and SW2 and uses them as a 2-bit channel select
static uint8_t ReadSelectedChannel(void)
{
    uint8_t ch = 0u;

    if (SW1_STATE()) {
        ch |= 0x01u;
    }
    if (SW2_STATE()) {
        ch |= 0x02u;
    }

    return ch;
}

//Reads SW3 to choose between the 2 stored filters
static uint8_t ReadSelectedFilter(void)
{
    return SW3_STATE() ? 1u : 0u;
}

//Reads SW4 to choose LED display mode
static uint8_t ReadDisplayModeP2P(void)
{
    return SW4_STATE() ? 1u : 0u;
}

//Packs channel and filter selection into one byte for protocol reporting
static uint8_t BuildChannelFilterByte(uint8_t channel, uint8_t filterSel)
{
    return (uint8_t)(((channel & 0x0Fu) << 4) | (filterSel & 0x0Fu));
}

//Sends current channel/filter selection
static void SendChannelFilterReport(uint8_t channel, uint8_t filterSel)
{
    uint8_t payload = BuildChannelFilterByte(channel, filterSel);
    Protocol_SendPacket(1u, (unsigned char)ID_CHANNEL_FILTER, &payload);
}

//Converts 32 short filter weights into a page (64 bytes) for EEPROM storage
static void ShortsToPage(const short weights[], unsigned char page[])
{
    int i;
    uint16_t value;

    for (i = 0; i < FILTERLENGTH; i++) {
        value = (uint16_t)weights[i];
        page[2 * i]     = (unsigned char)((value >> 8) & 0xFFu);
        page[2 * i + 1] = (unsigned char)(value & 0xFFu);
    }
}

//Converts 64 EEPROM bytes back into 32 short filter weights
static void PageToShorts(const unsigned char page[], short weights[])
{
    int i;
    uint16_t value;

    for (i = 0; i < FILTERLENGTH; i++) {
        value = ((uint16_t)page[2 * i] << 8) | (uint16_t)page[2 * i + 1];
        weights[i] = (short)value;
    }
}

//Returns EEPROM page number for a given channel/filter pair
static uint8_t FilterPageIndex(uint8_t channel, uint8_t filterSel)
{
    return (uint8_t)(channel * NUM_FILTERS + filterSel);
}

//Saves the selected filter into EEPROM
static void SaveFilterToNVM(uint8_t channel, uint8_t filterSel)
{
    unsigned char page[FILTER_BYTES];
    uint8_t pageIndex = FilterPageIndex(channel, filterSel);

    //Pack weights into EEPROM page format
    ShortsToPage(storedFilters[channel][filterSel], page);

    //Write one full filter page to EEPROM
    NonVolatileMemory_WritePage(pageIndex, (char)FILTER_BYTES, page);
}

//Loads all 8 saved filters from EEPROM at startup
static void LoadAllFiltersFromNVM(void)
{
    unsigned char page[FILTER_BYTES];
    uint8_t ch, filt;

    for (ch = 0; ch < NUM_CHANNELS; ch++) {
        for (filt = 0; filt < NUM_FILTERS; filt++) {
            uint8_t pageIndex = FilterPageIndex(ch, filt);

            //Read one saved filter page from EEPROM
            NonVolatileMemory_ReadPage(pageIndex, (char)FILTER_BYTES, page);

            //Unpack EEPROM bytes back into filter weights
            PageToShorts(page, storedFilters[ch][filt]);
        }
    }
}

//Clears filtered-value history
static void ClearP2PHistory(void)
{
    memset(p2pHistory, 0, sizeof(p2pHistory));
    p2pIndex = 0;
    p2pCount = 0;
}

//Pushes one filtered sample into the peak-to-peak history buffer
static void PushFilteredSample(short sample)
{
    p2pHistory[p2pIndex] = sample;
    p2pIndex++;
    if (p2pIndex >= P2P_WINDOW) {
        p2pIndex = 0;
    }

    if (p2pCount < P2P_WINDOW) {
        p2pCount++;
    }
}

//Returns absolute value of a short safely
static short ComputeAbsoluteValue(short sample)
{
    if (sample < 0) {
        if (sample == (short)-32768) {
            return 32767;
        }
        return (short)(-sample);
    }
    return sample;
}

//Computes peak-to-peak value from the filtered-value history
static unsigned short ComputePeakToPeak(void)
{
    uint8_t i;
    short minVal, maxVal;

    if (p2pCount == 0u) {
        return 0u;
    }

    minVal = p2pHistory[0];
    maxVal = p2pHistory[0];

    for (i = 1; i < p2pCount; i++) {
        if (p2pHistory[i] < minVal) {
            minVal = p2pHistory[i];
        }
        if (p2pHistory[i] > maxVal) {
            maxVal = p2pHistory[i];
        }
    }

    return (unsigned short)(maxVal - minVal);
}

//Converts a measured value into an 8-LED bar graph pattern
static uint8_t ScaleToLEDBar(unsigned int value, unsigned int fullScale)
{
    uint8_t leds = 0;
    unsigned int litCount;
    unsigned int i;

    if (fullScale == 0u) {
        return 0u;
    }

    if (value >= fullScale) {
        return 0xFFu;
    }

    litCount = (value * 8u) / fullScale;
    if ((value > 0u) && (litCount == 0u)) {
        litCount = 1u;
    }

    for (i = 0; i < litCount; i++) {
        leds |= (uint8_t)(1u << i);
    }

    return leds;
}

//Updates the LED bar based on selected display mode
static void UpdateLEDBar(short filtered)
{
    unsigned int displayValue;
    uint8_t ledPattern;

    if (currentP2PMode) {
        //Peak-to-peak display mode
        displayValue = ComputePeakToPeak();
        ledPattern = ScaleToLEDBar(displayValue, 1000u);
    } else {
        //Absolute filtered value display mode
        displayValue = (unsigned int)ComputeAbsoluteValue(filtered);
        ledPattern = ScaleToLEDBar(displayValue, 1000u);
    }

    LEDS_SET(ledPattern);
}

//Computes the phase step used by the synthetic sine generator
static uint16_t ComputeSynthPhaseStep(unsigned int freq)
{
    uint32_t step;

    if (freq < 1u) {
        freq = 1u;
    }

    step = ((uint32_t)freq * 65536u) / SYNTH_SAMPLE_RATE_HZ;
    if (step == 0u) {
        step = 1u;
    }
    if (step > 65535u) {
        step = 65535u;
    }

    return (uint16_t)step;
}

//Initializes synthetic sine generator state
static void SynthInit(void)
{
    synthFrequency = 25u;
    synthEnabled = 0;
    synthPhase = 16384u;
    synthPhaseStep = ComputeSynthPhaseStep(synthFrequency);
    synthCurrentSample = SYNTH_DC_OFFSET;
}

//Changes synthetic waveform frequency
static void SynthSetFrequency(unsigned int newFreq)
{
    if (newFreq < 1u) {
        newFreq = 1u;
    }

    synthFrequency = newFreq;
    synthPhaseStep = ComputeSynthPhaseStep(synthFrequency);
    synthPhase = 16384u;
}

//Turns waveform generation on
static void SynthOn(void)
{
    synthEnabled = 1;
    synthPhase = 16384u;
}

//Turns waveform generation off
static void SynthOff(void)
{
    synthEnabled = 0;
    synthPhase = 16384u;
    synthCurrentSample = SYNTH_DC_OFFSET;
}

//Generates the next synthetic sample
static short SynthGetNextSample(void)
{
    uint8_t tableIndex;
    int32_t scaled;

    if (!synthEnabled) {
        synthCurrentSample = SYNTH_DC_OFFSET;
        return synthCurrentSample;
    }

    //Advance waveform phase and get corresponding sine-table index
    synthPhase += synthPhaseStep;
    tableIndex = (uint8_t)(synthPhase >> 8);

    //Scale signed sine value into ADC-like range around DC offset
    scaled = ((int32_t)sineTable[tableIndex] * SYNTH_AMPLITUDE) / 32767;
    scaled += SYNTH_DC_OFFSET;

    //Clamp into 10-bit ADC-like range
    if (scaled < 0) {
        scaled = 0;
    } else if (scaled > 1023) {
        scaled = 1023;
    }

    synthCurrentSample = (short)scaled;
    return synthCurrentSample;
}

//Pushes one synthetic sample into all channel circular buffers
static void PushSyntheticSample(short sample)
{
    uint8_t ch;
    uint8_t idx = writeIndex;

    for (ch = 0; ch < NUM_CHANNELS; ch++) {
        sampleHistory[ch][idx] = sample;
    }

    idx++;
    if (idx >= FILTERLENGTH) {
        idx = 0;
    }
    writeIndex = idx;
}

//Returns most recent raw sample for the selected channel
static short GetRawReading(uint8_t channel)
{
    uint8_t latest;

    latest = (writeIndex == 0u) ? (FILTERLENGTH - 1u) : (writeIndex - 1u);
    return sampleHistory[channel][latest];
}

//Returns filtered reading for selected channel/filter configuration
static short GetFilteredReading(uint8_t channel, uint8_t filterSel)
{
    return ADCFilter_ApplyFilter(storedFilters[channel][filterSel],
                                 sampleHistory[channel],
                                 (short)writeIndex);
}

//Sends ADC reading packet
static void SendADCReadingPacket(short raw, short filtered)
{
    unsigned char payload[4];

    payload[0] = (unsigned char)(((uint16_t)raw >> 8) & 0xFFu);
    payload[1] = (unsigned char)((uint16_t)raw & 0xFFu);
    payload[2] = (unsigned char)(((uint16_t)filtered >> 8) & 0xFFu);
    payload[3] = (unsigned char)((uint16_t)filtered & 0xFFu);

    Protocol_SendPacket(4u, (unsigned char)ID_ADC_READING, payload);
}

//Reads switches and updates currently selected channel/filter/mode
static void PollSwitchesAndUpdateSelection(void)
{
    uint8_t newChannel = ReadSelectedChannel();
    uint8_t newFilter = ReadSelectedFilter();
    uint8_t newP2PMode = ReadDisplayModeP2P();

    currentP2PMode = newP2PMode;

    if ((newChannel != currentChannel) || (newFilter != currentFilter)) {
        currentChannel = newChannel;
        currentFilter = newFilter;
        ClearP2PHistory();

        if ((currentChannel != lastReportedChannel) || (currentFilter != lastReportedFilter)) {
            SendChannelFilterReport(currentChannel, currentFilter);
            lastReportedChannel = currentChannel;
            lastReportedFilter = currentFilter;
        }
    }
}

//Handles incoming packets
static void HandleIncomingPackets(void)
{
    uint8_t pktType;
    uint8_t pktLen;
    unsigned char msg[MAXPAYLOADLENGTH];

    while (Protocol_GetInPacket(&pktType, &pktLen, msg) == SUCCESS) {
        switch (pktType) {

            case ID_ADC_FILTER_VALUES:
                if (pktLen == FILTER_BYTES) {
                    //Unpack incoming filter coefficients into current slot
                    PageToShorts(msg, storedFilters[currentChannel][currentFilter]);

                    //Save updated filter to EEPROM
                    SaveFilterToNVM(currentChannel, currentFilter);

                    //Acknowledge filter write
                    Protocol_SendPacket(1u, (unsigned char)ID_ADC_FILTER_VALUES_RESP, &currentChannel);

                    ClearP2PHistory();
                } else {
                    Protocol_SendDebugMessage("ADC filter payload wrong length");
                }
                break;

            case ID_SET_FREQUENCY:
                if (pktLen == 2u) {
                    uint16_t newFreq = ((uint16_t)msg[0] << 8) | (uint16_t)msg[1];

                    //Update synthetic waveform frequency
                    SynthSetFrequency((unsigned int)newFreq);

                    ClearP2PHistory();
                } else {
                    Protocol_SendDebugMessage("Frequency payload wrong length");
                }
                break;

            case ID_FREQUENCY_ONOFF:
                if (pktLen == 1u) {
                    //Enable or disable synthetic waveform generation
                    if (msg[0]) {
                        SynthOn();
                    } else {
                        SynthOff();
                    }

                    ClearP2PHistory();
                } else {
                    Protocol_SendDebugMessage("Frequency on/off payload wrong length");
                }
                break;

            default:
                break;
        }
    }
}

//Tie everything together to make a functioning main.c
int main(void)
{
    char dbg[96];
    short rawReading;
    short filteredReading;

    //Initialize modules and peripherals
    BOARD_Init();
    Protocol_Init(BAUD);
    LEDS_INIT();
    SwitchesInit();
    AppTimerInit();
    SynthTimerInit();

    //Initialize EEPROM, filter module, and synthetic source
    NonVolatileMemory_Init();
    ADCFilter_Init();
    SynthInit();

    //Start with empty sample history, then load saved filters from EEPROM
    memset(sampleHistory, 0, sizeof(sampleHistory));
    LoadAllFiltersFromNVM();

    //Read current switch positions at startup
    currentChannel = ReadSelectedChannel();
    currentFilter = ReadSelectedFilter();
    currentP2PMode = ReadDisplayModeP2P();

    ClearP2PHistory();

    //Send startup debug string
    sprintf(dbg, "Main application %s %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(dbg);

    //Report initial selected channel/filter to Python script
    SendChannelFilterReport(currentChannel, currentFilter);
    lastReportedChannel = currentChannel;
    lastReportedFilter = currentFilter;

    while (1) {
        //Keep protocol queue updated and process incoming commands
        Protocol_QueuePacket();
        HandleIncomingPackets();

        //Generate new fake ADC samples at 1kHz
        if (SynthTimerReady() == SUCCESS) {
            PushSyntheticSample(SynthGetNextSample());
        }

        //Do main application update at 100Hz
        if (AppTimerReady() == SUCCESS) {
            PollSwitchesAndUpdateSelection();

            //Compute latest raw and filtered values for selected channel/filter
            rawReading = GetRawReading(currentChannel);
            filteredReading = GetFilteredReading(currentChannel, currentFilter);

            //Update p2p history, LEDs, and Python display packet
            PushFilteredSample(filteredReading);
            UpdateLEDBar(filteredReading);
            SendADCReadingPacket(rawReading, filteredReading);
        }
    }

    return 0;
}