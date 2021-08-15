/* mbed VLSI VS1053b library
 * Copyright (c) 2010 Christian Schmiljun
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
/* This code based on:
 *  mbeduino_MP3_Shield_MP3Player
 *  http://mbed.org/users/xshige/programs/mbeduino_MP3_Shield_MP3Player/lgcx63
 *  2010-10-16
 */
 
#ifndef _VS1053_H
#define _VS1053_H
 
// ----------------------------------------------------------------------------
// Extended settings
// ----------------------------------------------------------------------------
//   Enable debug output (Output -> printf ...)
//   --------------------------------------------------------------------------
//   #define DEBUG
//   #define DEBUGOUT (x,y...)                printf(x, ##y);
//   Patches, Addons
//   --------------------------------------------------------------------------
//   #define VS1053_PATCH_1_4_FLAC
//   #define VS1053_PATCH_1_5
//   #define VS1053_PATCH_1_5_FLAC
//   #define VS1053_SPECANA
//   #define VS1053B_PCM_RECORDER_0_9
// ----------------------------------------------------------------------------
 
#include "mbed.h"
#include "string"
#include "string.h"
#include "FATFileSystem.h"
 
#if defined(VS1053_PATCH_1_4_FLAC) && defined(VS1053_PATCH_1_5) && defined(VS1053_PATCH_1_5_FLAC) && defined(VS1053_SPECANA) && defined(VS1053B_PCM_RECORDER_0_9)
#error "VS1053: Exclusive use of patch and app versions."
#endif
#if defined(VS1053_PATCH_1_4_FLAC) || defined(VS1053_PATCH_1_5) || defined(VS1053_PATCH_1_5_FLAC) || defined(VS1053_SPECANA) || defined(VS1053B_PCM_RECORDER_0_9)
#define VS_PATCH
#endif

 
#define DEFAULT_BALANCE_DIFERENCE_LEFT_RIGHT          0.0f
#define DEFAULT_VOLUME                              -40.0f
#define DEFAULT_BASS_AMPLITUDE                        5        //   0 -    15 dB
#define DEFAULT_BASS_FREQUENCY                      100        //  20 -   150 Hz
#define DEFAULT_TREBLE_AMPLITUDE                      0        //  -8 -     7 dB
#define DEFAULT_TREBLE_FREQUENCY                  15000        //1000 - 15000 Hz
 
 
 
// SCI register address assignment
#define SCI_MODE                                    0x00
#define SCI_STATUS                                  0x01
#define SCI_BASS                                    0x02
#define SCI_CLOCKF                                  0x03
#define SCI_DECODE_TIME                             0x04
#define SCI_AUDATA                                  0x05
#define SCI_WRAM                                    0x06
#define SCI_WRAMADDR                                0x07
#define SCI_HDAT0                                   0x08
#define SCI_HDAT1                                   0x09
#define SCI_AIADDR                                  0x0A
#define SCI_VOL                                     0x0B
#define SCI_AICTRL0                                 0x0C
#define SCI_AICTRL1                                 0x0D
#define SCI_AICTRL2                                 0x0E
#define SCI_AICTRL3                                 0x0F
 
 
//SCI_MODE register bits as of p.38 of the datasheet
#define SM_DIFF                                     0x0001
#define SM_LAYER12                                  0x0002
#define SM_RESET                                    0x0004
#define SM_CANCEL                                   0x0008
#define SM_EARSPEAKER_LO                            0x0010
#define SM_TESTS                                    0x0020
#define SM_STREAM                                   0x0040
#define SM_EARSPEAKER_HI                            0x0080
#define SM_DACT                                     0x0100
#define SM_SDIORD                                   0x0200
#define SM_SDISHARE                                 0x0400
#define SM_SDINEW                                   0x0800
#define SM_ADPCM                                    0x1000
#define SM_B13                                      0x2000
#define SM_LINE1                                    0x4000
#define SM_CLK_RANGE                                0x8000
 
//SCI_CLOCKF register bits as of p.42 of the datasheet
#define SC_ADD_NOMOD                                0x0000
#define SC_ADD_10x                                  0x0800
#define SC_ADD_15x                                  0x1000
#define SC_ADD_20x                                  0x1800
#define SC_MULT_XTALI                               0x0000
#define SC_MULT_XTALIx20                            0x2000
#define SC_MULT_XTALIx25                            0x4000
#define SC_MULT_XTALIx30                            0x6000
#define SC_MULT_XTALIx35                            0x8000
#define SC_MULT_XTALIx40                            0xA000
#define SC_MULT_XTALIx45                            0xC000
#define SC_MULT_XTALIx50                            0xE000
 
// Extra Parameter in X memory (refer to p.58 of the datasheet)
#define para_chipID_0                               0x1E00
#define para_chipID_1                               0x1E01
#define para_version                                0x1E02
#define para_config1                                0x1E03
#define para_playSpeed                              0x1E04
#define para_byteRate                               0x1E05
#define para_endFillByte                            0x1E06
//
#define para_positionMsec_0                         0x1E27
#define para_positionMsec_1                         0x1E28
#define para_resync                                 0x1E29
   
#define INTERRUPT_HANDLER_ENABLE                    _DREQ_INTERUPT_IN.rise(this, &VS1053::dataRequestHandler); timer.attach_us(this, &VS1053::dataRequestHandler, 1000)
#define INTERRUPT_HANDLER_DISABLE                   _DREQ_INTERUPT_IN.rise(NULL); timer.detach()  

 #define MAX_SONG_NUM 6

extern int new_song_number;
extern int volume_set;
extern bool pause;
extern bool mute;
extern char song_name[MAX_SONG_NUM][20];
extern BufferedSerial pc;
 

class VS1053  {
 
public:
    /** Create a vs1053b object.
     *
     * @param mosi 
     *   SPI Master Out, Slave In pin to vs1053b.
     * @param miso 
     *   SPI Master In, Slave Out pin to vs1053b.
     * @param sck  
     *   SPI Clock pin to vs1053b.
     * @param cs   
     *   Pin to vs1053b control chip select.
     * @param rst  
     *   Pin to vs1053b reset.
     * @param dreq 
     *   Pin to vs1053b data request.
     * @param dcs  
     *   Pin to vs1053b data chip select.
     * @param buffer  
     *   Array to cache audio data.
     * @param buffer_size  
     *   Length of the array.
     */
    VS1053(
        PinName mosi,
        PinName miso,
        PinName sck,
        PinName cs,
        PinName rst,
        PinName dreq,
        PinName dcs,        
        char*   buffer,
        int     buffer_size
    );           
 
    /** Reset the vs1053b. (hardware reset)
     *
     */
    void reset(void);    
 
    
    /** Set the volume.
     * 
     * @param volume 
     *   Volume -0.5dB, -1.0dB, .. -64.0dB. 
     */    
    void  setVolume(float volume = DEFAULT_VOLUME);    
    
    /** Get the volume.
     *
     * @return 
     *   Return the volume in dB.
     */
    float getVolume();
    
    void play_song(int);
    
    void cs_low(void);
    void cs_high(void);
    void dcs_low(void);
    void dcs_high(void);
    void sci_en(void);
    void sci_dis(void);
    void sdi_en(void);
    void sdi_dis(void);
 
    void spi_initialise(void);
 
    void sdi_initialise(void);
 
    void sci_write(unsigned char, unsigned short int);
    void sdi_write(unsigned char);
    unsigned short int sci_read(unsigned short int);
    void sine_test_activate(unsigned char);
 
    void sine_test_deactivate(void);
 
    void changeVolume(void);
        
    
    // TODO
    void power_down(void);
 
    void changeBass(void);
    
    DigitalOut                      _RST;
    DigitalIn                       _DREQ; 
    
protected:
    
    SPI                             _spi;
    DigitalOut                      _CS;
    DigitalOut                      _DCS;       
    InterruptIn                     _DREQ_INTERUPT_IN;
    
    char*                            _buffer;
    char*                           _bufferReadPointer;
    char*                           _bufferWritePointer;
    int                             BUFFER_SIZE;
    
    bool                            _isIdle;
 
    // variables to save 
    //   volume, values in db
    float                           _balance;    
    float                           _volume;
    //   bass enhancer settings  
    int                             _sb_amplitude;
    int                             _sb_freqlimit;
    int                             _st_amplitude;
    int                             _st_freqlimit; 
       
    
    Ticker timer;  
    Timer timer1;      
    
};
 
#endif
 