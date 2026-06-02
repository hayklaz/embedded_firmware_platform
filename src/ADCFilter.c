/* 
 * File:   ADCFilter.c
 * Author: Hayk Lazaryan
 * 
 * Created on March 11, 2026, 12:45pm
 */

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>

// Standard libraries
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "ADCFilter.h"
#include "MessageIDs.h"
#include "BOARD.h"
#include "Uart.h"
#include "Protocol2.h"

/*******************************************************************************
 * Defines
 ******************************************************************************/

//Support 4 different ADC channels (A0-A3)
#define NUMCHANNELS 4

//Filter weights are to scale sinusoidal function
#define FILTER_SCALE_SHIFT 15

//Synthetic waveform timing
#define SYNTH_SAMPLE_RATE_HZ   1000u
#define SYNTH_TABLE_SIZE       256u
#define SYNTH_AMPLITUDE        450
#define SYNTH_DC_OFFSET        512

//Synthetic clock
#define ADC_SYNTH_UPDATE_TICKS    4999u
#define PACKET_T2_PRESCALE        8u
#define PACKET_T2_TCKPS_BITS      0b001
#define PACKET_RATE_MIN_HZ        50u
#define PACKET_RATE_MAX_HZ        1000u

/*******************************************************************************
 * Variables and Data
 ******************************************************************************/

//Circular buffer of input samples for each logical ADC channel
static volatile short adcValues[NUMCHANNELS][FILTERLENGTH];

//Weights for each logical ADC channel
static volatile short adcWeights[NUMCHANNELS][FILTERLENGTH];

//Points to the next slot that will be written in the circular buffer
static volatile uint8_t writeIndex = 0;

//Synthetic waveform state
static volatile unsigned int synthFrequency = 25u;
static volatile uint8_t synthEnabled = 0;
static volatile uint16_t synthPhase = 0;
static volatile uint16_t synthPhaseStep = 0;
static volatile short synthCurrentSample = SYNTH_DC_OFFSET;


//256-point sine table scaled to +/-32767 which is used to generate a 
//synthetic analog waveform in software
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

//Converts logical channel number into array index
static int PinToIndex(short pin)
{
    if ((pin < 0) || (pin >= NUMCHANNELS)) {
        return -1;
    }
    return pin;
}

// Points to the latest valid sample that has been written into
static uint8_t LatestIndexFromWrite(uint8_t nextWrite)
{
    if (nextWrite == 0u) {
        return (uint8_t)(FILTERLENGTH - 1u);
    }
    return (uint8_t)(nextWrite - 1u);
}

//Copies a channel's circular buffer into a local array
static void CopyChannelSamples(short dst[], short pin, uint8_t *capturedWriteIndex)
{
    uint8_t i;

    *capturedWriteIndex = writeIndex;
    for (i = 0; i < FILTERLENGTH; i++) {
        dst[i] = adcValues[pin][i];
    }
}

//Clears a channel's sample history
static void ClearChannelHistory(short pin)
{
    int idx = PinToIndex(pin);
    uint8_t i;

    if (idx < 0) {
        return;
    }

    for (i = 0; i < FILTERLENGTH; i++) {
        adcValues[idx][i] = 0;
    }
}

//Clears all channel histories and resets write index
static void ClearAllHistory(void)
{
    uint8_t ch, i;

    for (ch = 0; ch < NUMCHANNELS; ch++) {
        for (i = 0; i < FILTERLENGTH; i++) {
            adcValues[ch][i] = 0;
        }
    }
    writeIndex = 0;
}

//Turns 2 big-endian bytes into a signed short
static short BytesToShortBE(unsigned char msb, unsigned char lsb)
{
    uint16_t combined = ((uint16_t)msb << 8) | (uint16_t)lsb;
    return (short)combined;
}

//Turns 2 big-endian bytes into an unsigned short
static unsigned short BytesToUShortBE(unsigned char msb, unsigned char lsb)
{
    uint16_t combined = ((uint16_t)msb << 8) | (uint16_t)lsb;
    return (unsigned short)combined;
}

//Converts the 64-byte payload from Python into 32 signed filter weights
//then loads those weights into the selected channel
static int LoadWeightsFromPayload(short channel, unsigned char payload[])
{
    short weights[FILTERLENGTH];
    int i;

    if (payload == 0) {
        return ERROR;
    }

    for (i = 0; i < FILTERLENGTH; i++) {
        weights[i] = BytesToShortBE(payload[2 * i], payload[2 * i + 1]);
    }

    return ADCFilter_SetWeights(channel, weights);
}

//Computes phase increment for the synthetic sine generator
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

//Chooses how fast packets should be sent
//Slows down packet transmission for low frequencies, and vice versa
static unsigned int ComputePacketRateHz(unsigned int freq)
{
    uint32_t rate = (uint32_t)freq * 20u;

    if (rate < PACKET_RATE_MIN_HZ) {
        rate = PACKET_RATE_MIN_HZ;
    }
    if (rate > PACKET_RATE_MAX_HZ) {
        rate = PACKET_RATE_MAX_HZ;
    }

    return (unsigned int)rate;
}

//Converts desired packet rate into Timer2 PR value
static uint16_t ComputePacketTimerPR(unsigned int packetRateHz)
{
    uint32_t pr;

    if (packetRateHz < 1u) {
        packetRateHz = 1u;
    }

    pr = (uint32_t)(PBCLK / (PACKET_T2_PRESCALE * packetRateHz));
    if (pr == 0u) {
        pr = 1u;
    }
    pr -= 1u;

    if (pr > 0xFFFFu) {
        pr = 0xFFFFu;
    }

    return (uint16_t)pr;
}

//Updates Timer2 so packet send rate follows the current synthetic frequency
static void UpdatePacketTimerFromFrequency(unsigned int freq)
{
    unsigned int packetRateHz = ComputePacketRateHz(freq);

    T2CONbits.ON = 0;
    TMR2 = 0;
    PR2 = ComputePacketTimerPR(packetRateHz);
    IFS0bits.T2IF = 0;
    T2CONbits.ON = 1;
}

//Initializes software sine generator
static void SynthGenerator_Init(void)
{
    synthFrequency = 25u;
    synthEnabled = 0;
    synthPhase = 16384u;
    synthPhaseStep = ComputeSynthPhaseStep(synthFrequency);
    synthCurrentSample = SYNTH_DC_OFFSET;
}

//Changes synthetic waveform frequency
static void SynthGenerator_SetFrequency(unsigned int newFreq)
{
    if (newFreq < 1u) {
        newFreq = 1u;
    }

    synthFrequency = newFreq;
    synthPhaseStep = ComputeSynthPhaseStep(synthFrequency);
    synthPhase = 16384u;
}

//Turns synthetic waveform generation on
static void SynthGenerator_On(void)
{
    synthEnabled = 1;
    synthPhase = 16384u;
}

//Turns waveform generation off
static void SynthGenerator_Off(void)
{
    synthEnabled = 0;
    synthPhase = 16384u;
    synthCurrentSample = SYNTH_DC_OFFSET;
}

//Generates the next ADC sample
static short SynthGenerator_GetNextSample(void)
{
    uint8_t tableIndex;
    int32_t scaled;

    if (!synthEnabled) {
        synthCurrentSample = SYNTH_DC_OFFSET;
        return synthCurrentSample;
    }

    synthPhase += synthPhaseStep;
    tableIndex = (uint8_t)(synthPhase >> 8);

    scaled = ((int32_t)sineTable[tableIndex] * SYNTH_AMPLITUDE) / 32767;
    scaled += SYNTH_DC_OFFSET;

    if (scaled < 0) {
        scaled = 0;
    } else if (scaled > 1023) {
        scaled = 1023;
    }

    synthCurrentSample = (short)scaled;
    return synthCurrentSample;
}

//Pushes one new synthetic sample into every logical ADC channel
static void PushSyntheticSample(short sample)
{
    uint8_t ch;
    uint8_t idx = writeIndex;

    for (ch = 0; ch < NUMCHANNELS; ch++) {
        adcValues[ch][idx] = sample;
    }

    idx++;
    if (idx >= FILTERLENGTH) {
        idx = 0;
    }
    writeIndex = idx;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

//Initializes filter buffers, weights, and synthetic source
int ADCFilter_Init(void)
{
    int ch, i;

    memset((void *)adcValues, 0, sizeof(adcValues));
    memset((void *)adcWeights, 0, sizeof(adcWeights));
    writeIndex = 0;

    for (ch = 0; ch < NUMCHANNELS; ch++) {
        adcWeights[ch][0] = (1 << FILTER_SCALE_SHIFT);
        for (i = 1; i < FILTERLENGTH; i++) {
            adcWeights[ch][i] = 0;
        }
    }

    SynthGenerator_Init();

    return SUCCESS;
}

//Returns most recent raw sample from selected channel
short ADCFilter_RawReading(short pin)
{
    int idx = PinToIndex(pin);
    uint8_t nextWrite;
    uint8_t latest;

    if (idx < 0) {
        return 0;
    }

    nextWrite = writeIndex;
    latest = LatestIndexFromWrite(nextWrite);
    return adcValues[idx][latest];
}

//Returns filtered reading from selected channel
short ADCFilter_FilteredReading(short pin)
{
    int idx = PinToIndex(pin);
    short localValues[FILTERLENGTH];
    short localWeights[FILTERLENGTH];
    uint8_t capturedWrite;
    uint8_t i;

    if (idx < 0) {
        return 0;
    }

    CopyChannelSamples(localValues, (short)idx, &capturedWrite);

    for (i = 0; i < FILTERLENGTH; i++) {
        localWeights[i] = adcWeights[idx][i];
    }

    return ADCFilter_ApplyFilter(localWeights, localValues, (short)capturedWrite);
}

//Applies filter to one circular buffer
short ADCFilter_ApplyFilter(short filter[], short values[], short startIndex)
{
    int i;
    int32_t acc = 0;
    int sampleIndex;

    sampleIndex = (int)startIndex - 1;
    if (sampleIndex < 0) {
        sampleIndex = FILTERLENGTH - 1;
    }

    for (i = 0; i < FILTERLENGTH; i++) {
        acc += ((int32_t)filter[i]) * ((int32_t)values[sampleIndex]);

        sampleIndex--;
        if (sampleIndex < 0) {
            sampleIndex = FILTERLENGTH - 1;
        }
    }

    acc >>= FILTER_SCALE_SHIFT;

    if (acc > 32767) {
        acc = 32767;
    } else if (acc < -32768) {
        acc = -32768;
    }

    return (short)acc;
}

//Loads a full set of weights into the selected channel
int ADCFilter_SetWeights(short pin, short weights[])
{
    int idx = PinToIndex(pin);
    int i;

    if ((idx < 0) || (weights == 0)) {
        return ERROR;
    }

    for (i = 0; i < FILTERLENGTH; i++) {
        adcWeights[idx][i] = weights[i];
    }

    return SUCCESS;
}


#ifdef ADCFILTER_TESTHARNESS

//Initializes Timer2 for packet send timing and Timer3 for waveform timing
static void ADCFilter_TestHarnessTimersInit(void)
{
    //Timer2 used for adaptive packet send rate
    T2CON = 0;
    TMR2 = 0;
    T2CONbits.TCKPS = PACKET_T2_TCKPS_BITS;
    UpdatePacketTimerFromFrequency(synthFrequency);

    //Timer3 used for 1kHz sample update rate
    T3CON = 0;
    TMR3 = 0;
    PR3 = ADC_SYNTH_UPDATE_TICKS;
    T3CONbits.TCKPS = 0b001;
    IFS0bits.T3IF = 0;
    T3CONbits.ON = 1;
}

//Returns SUCCESS whenever it is time to send another packet
static int ADCFilter_TestHarnessPacketReady(void)
{
    if (IFS0bits.T2IF) {
        IFS0bits.T2IF = 0;
        return SUCCESS;
    }
    return ERROR;
}

//Returns SUCCESS whenever it is time to generate another synthetic ADC sample
static int ADCFilter_TestHarnessSynthReady(void)
{
    if (IFS0bits.T3IF) {
        IFS0bits.T3IF = 0;
        return SUCCESS;
    }
    return ERROR;
}

//Sends raw and filtered readings
static void SendADCReadingPacket(short channel)
{
    short raw;
    short filtered;
    unsigned char payload[4];

    raw = ADCFilter_RawReading(channel);
    filtered = ADCFilter_FilteredReading(channel);

    payload[0] = (unsigned char)(((uint16_t)raw >> 8) & 0xFFu);
    payload[1] = (unsigned char)((uint16_t)raw & 0xFFu);
    payload[2] = (unsigned char)(((uint16_t)filtered >> 8) & 0xFFu);
    payload[3] = (unsigned char)((uint16_t)filtered & 0xFFu);

    Protocol_SendPacket(4u, (unsigned char)ID_ADC_READING, payload);
}


int main(void)
{
    uint8_t pktType;
    uint8_t pktLen;
    unsigned char msg[MAXPAYLOADLENGTH];
    char dbg[128];
    unsigned char channel = 0u;
    unsigned char freqState = 0u;
    unsigned short curFrequency = 25u;

    //Initialize Protocol, Filters and Timers
    Protocol_Init(115200UL);
    ADCFilter_Init();
    ADCFilter_TestHarnessTimersInit();

    //Initialize sinewave generator's initial settings 
    SynthGenerator_SetFrequency(curFrequency);
    SynthGenerator_Off();
    ClearAllHistory();
    UpdatePacketTimerFromFrequency(curFrequency);

    sprintf(dbg, "ADCFilter synthetic sine harness fixed HP + adaptive GUI Vpp %s %s", __DATE__, __TIME__);
    Protocol_SendDebugMessage(dbg);

    while (1) {
        //Keep protocol input queue updated
        Protocol_QueuePacket();

        //Handle all incoming packets
        while (Protocol_GetInPacket(&pktType, &pktLen, msg) == SUCCESS) {
            switch (pktType) {

                //Changes currently selected ADC channel
                case ID_ADC_SELECT_CHANNEL:
                    if (pktLen == 1u) {
                        if (msg[0] < 4u) {
                            channel = msg[0];
                            Protocol_SendPacket(1u,
                                                (unsigned char)ID_ADC_SELECT_CHANNEL_RESP,
                                                &channel);
                        } else {
                            Protocol_SendDebugMessage("ADC channel out of range");
                        }
                    } else {
                        Protocol_SendDebugMessage("ADC select payload wrong length");
                    }
                    break;

                //Loads new filter weights into the selected channel
                case ID_ADC_FILTER_VALUES:
                    if (pktLen == (uint8_t)(FILTERLENGTH * 2u)) {
                        if (LoadWeightsFromPayload((short)channel, msg) == SUCCESS) {
                            Protocol_SendPacket(1u,
                                                (unsigned char)ID_ADC_FILTER_VALUES_RESP,
                                                &channel);
                            ClearChannelHistory((short)channel);
                        } else {
                            Protocol_SendDebugMessage("ADC filter load failed");
                        }
                    } else {
                        Protocol_SendDebugMessage("ADC filter payload wrong length");
                    }
                    break;

                //Changes waveform frequency and updates packet timing
                case ID_LAB3_SET_FREQUENCY:
                    if (pktLen == 2u) {
                        curFrequency = BytesToUShortBE(msg[0], msg[1]);
                        SynthGenerator_SetFrequency((unsigned int)curFrequency);
                        UpdatePacketTimerFromFrequency((unsigned int)curFrequency);
                        ClearAllHistory();
                    } else {
                        Protocol_SendDebugMessage("Frequency payload wrong length");
                    }
                    break;

                //Turns waveform on or off
                case ID_LAB3_FREQUENCY_ONOFF:
                    if (pktLen == 1u) {
                        freqState = msg[0];
                        if (freqState) {
                            SynthGenerator_On();
                        } else {
                            SynthGenerator_Off();
                        }
                        ClearAllHistory();
                    } else {
                        Protocol_SendDebugMessage("Frequency on/off payload wrong length");
                    }
                    break;

                default:
                    break;
            }
        }

        //Generate ADC samples at 1kHz
        if (ADCFilter_TestHarnessSynthReady() == SUCCESS) {
            PushSyntheticSample(SynthGenerator_GetNextSample());
        }

        //Send raw and filtered readings
        if (ADCFilter_TestHarnessPacketReady() == SUCCESS) {
            SendADCReadingPacket((short)channel);
        }
    }

    return 0;
}

#endif /* ADCFILTER_TESTHARNESS */