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
 
#include "VS1053.h"
#include "mbed.h"
#include "FATFileSystem.h"

BufferedSerial pc1(CONSOLE_TX, CONSOLE_RX, 115200);
char buffer1[80];
BlockDevice *bd1 = BlockDevice::get_default_instance();
FATFileSystem fs1("fs");

/* ==================================================================
 * Constructor
 * =================================================================*/
VS1053::VS1053(
    PinName mosi, PinName miso, PinName sck, PinName cs, PinName rst,
    PinName dreq, PinName dcs, char* buffer, int buffer_size)
        :
        _spi(mosi, miso, sck),
        _CS(cs),
        _RST(rst),
        _DCS(dcs),
        _DREQ(dreq),        
        _DREQ_INTERUPT_IN(dreq) 
{           
        _volume = DEFAULT_VOLUME;
        _balance = DEFAULT_BALANCE_DIFERENCE_LEFT_RIGHT;
        _sb_amplitude = DEFAULT_BASS_AMPLITUDE;
        _sb_freqlimit = DEFAULT_BASS_FREQUENCY;
        _st_amplitude = DEFAULT_TREBLE_AMPLITUDE;
        _st_freqlimit = DEFAULT_TREBLE_FREQUENCY;   
        _buffer = buffer;
        BUFFER_SIZE = buffer_size;
        _DREQ_INTERUPT_IN.mode(PullDown);
        INTERRUPT_HANDLER_DISABLE;
}
 
 
/*===================================================================
 * Functions
 *==================================================================*/
 
void VS1053::cs_low(void) {
    _CS = 0;
}
void VS1053::cs_high(void) {
    _CS = 1;
}
void VS1053::dcs_low(void) {
    _DCS = 0;
 
}
void VS1053::dcs_high(void) {
    _DCS = 1;
}
void VS1053::sci_en(void) {                  //SCI enable
    cs_high();
    dcs_high();
    cs_low();
}
void VS1053::sci_dis(void) {                  //SCI disable
    cs_high();
}
void VS1053::sdi_en(void) {                  //SDI enable
    dcs_high();
    cs_high();
    dcs_low();
}
void VS1053::sdi_dis(void) {                  //SDI disable
    dcs_high();
}
void VS1053::reset(void) {                  //hardware reset
    INTERRUPT_HANDLER_DISABLE;
    ThisThread::sleep_for(chrono::milliseconds(10));
    _RST = 0;
    ThisThread::sleep_for(chrono::milliseconds(5));
    _RST = 1;
    ThisThread::sleep_for(chrono::milliseconds(10));
}
void VS1053::power_down(void) {              //hardware and software reset
    cs_low();
    reset();
//    sci_write(0x00, SM_PDOWN);
    sci_write(0x00, 0x10); // tempo
    ThisThread::sleep_for(chrono::milliseconds(10)); //wait(0.01)
    reset();
}
void VS1053::spi_initialise(void) {
    _RST = 1;                                //no reset
    _spi.format(8,0);                        //spi 8bit interface, steady state low
//   _spi.frequency(1000000);                //rising edge data record, freq. 1Mhz
    _spi.frequency(2000000);                //rising edge data record, freq. 2Mhz
 
 
    cs_low();
    for (int i=0; i<4; i++) {
        _spi.write(0xFF);                        //clock the chip a bit
    }
    cs_high();
    dcs_high();
    //wait_us(5);
    timer1.reset();
    timer1.start();
    while(timer1.elapsed_time().count()<5){}
    timer1.stop();
    timer1.reset();
}
void VS1053::sdi_initialise(void) {
    _spi.frequency(8000000);                //set to 8 MHz to make fast transfer
    cs_high();
    dcs_high();
}
void VS1053::sci_write(unsigned char address, unsigned short int data) {
    // TODO disable all interrupts
    //__disable_irq();
    sci_en();                                //enables SCI/disables SDI
 
    while (!_DREQ);                           //wait unitl data request is high
    _spi.write(0x02);                        //SCI write
    _spi.write(address);                    //register address
    _spi.write((data >> 8) & 0xFF);            //write out first half of data word
    _spi.write(data & 0xFF);                //write out second half of data word
 
    sci_dis();                                //enables SDI/disables SCI
    //wait_us(5);
    timer1.reset();
    timer1.start();
    while(timer1.elapsed_time().count()<5){}
    timer1.stop();
    timer1.reset();
    
    
    // TODO enable all interrupts
    //__enable_irq();
}
void VS1053::sdi_write(unsigned char datum) {    
    
    sdi_en();
 
    while (!_DREQ);
    _spi.write(datum);
 
    sdi_dis();    
}
unsigned short VS1053::sci_read(unsigned short int address) {
    // TODO disable all interrupts
    //__disable_irq();
    
    cs_low();                                //enables SCI/disables SDI
 
    while (!_DREQ);                           //wait unitl data request is high
    _spi.write(0x03);                        //SCI write
    _spi.write(address);                    //register address
    unsigned short int received = _spi.write(0x00);    //write out dummy byte
    received <<= 8;
    received |= _spi.write(0x00);            //write out dummy byte
 
    cs_high();                                //enables SDI/disables SCI
 
    // TODO enable all interrupts
    //__enable_irq();
    return received;                        //return received word
}
void VS1053::sine_test_activate(unsigned char wave) {
    cs_high();                                //enables SDI/disables SCI
 
    while (!_DREQ);                           //wait unitl data request is high
    _spi.write(0x53);                        //SDI write
    _spi.write(0xEF);                        //SDI write
    _spi.write(0x6E);                        //SDI write
    _spi.write(wave);                        //SDI write
    _spi.write(0x00);                        //filler byte
    _spi.write(0x00);                        //filler byte
    _spi.write(0x00);                        //filler byte
    _spi.write(0x00);                        //filler byte
 
    cs_low();                                //enables SCI/disables SDI
}
void VS1053::sine_test_deactivate(void) {
    cs_high();
 
    while (!_DREQ);
    _spi.write(0x45);                        //SDI write
    _spi.write(0x78);                        //SDI write
    _spi.write(0x69);                        //SDI write
    _spi.write(0x74);                        //SDI write
    _spi.write(0x00);                        //filler byte
    _spi.write(0x00);                        //filler byte
    _spi.write(0x00);                        //filler byte
    _spi.write(0x00);                        //filler byte
}
 
 
void VS1053::setVolume(float vol) 
{    
    if (vol > -0.1)
        _volume = -0.1;
    else
        _volume = vol;
 
    changeVolume();
}
 
float VS1053::getVolume(void) 
{
    return _volume;
}

 
void VS1053::changeVolume(void) 
{
    // volume calculation        
    unsigned short volCalced = (((char)(_volume / -0.5f)) << 8) + (char)((_volume - _balance) / -0.5f);
   
    sci_write(SCI_VOL, volCalced);    
}