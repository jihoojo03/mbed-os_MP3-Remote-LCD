/* Author: Minjoo Kim & Jihoon Jeong  */
 
 #include "mbed.h" 
 #include "VS1053.h" 
 #include "ReceiverIR.h"
 #include "Adafruit_SSD1306.h"
 #include "SDBlockDevice.h"
 #include "FATFileSystem.h"
 
 
const int VS1053B_BUFFER_SIZE = (16 * 1024 + 1);
char VS1053_BUFFER[VS1053B_BUFFER_SIZE];
char* VS1053B_BUFFER_POINTER  = VS1053_BUFFER;

BlockDevice *bd = BlockDevice::get_default_instance();
FATFileSystem fs("fs");
FILE *song;

//PinName mosi, PinName miso, PinName sck, PinName cs, PinName rst,
//    PinName dreq, PinName dcs, char* buffer, int buffer_size)
//VS1053 mp3(D11, D12, D13, D6, D9, D3, D8, VS1053_BUFFER, VS1053B_BUFFER_SIZE); //dcs = XDCS(D8) or SDCS(D2)
//ReceiverIR rec(D5);
//Adafruit_SSD1306_Spi gOLED(D11, D13, D10, D4, D7);
//BufferedSerial pc(CONSOLE_TX, CONSOLE_RX, 115200);

VS1053 mp3(D11, D12, D13, D10, D9, D3, D8, VS1053_BUFFER, VS1053B_BUFFER_SIZE); //dcs = XDCS(D8) or SDCS(D2)
ReceiverIR rec(D5);
Adafruit_SSD1306_Spi gOLED(D11, D13, D6, D2, D7);
BufferedSerial pc(CONSOLE_TX, CONSOLE_RX, 115200);
Ticker tic;

/* Global Variables to store Status*/
int new_song_number=0;  //Variable to store the New Song Number
int cur_song_number=0;  //Variable to store the Current Song Number
int volume_set=2;       //Variable to store the Volume
int vol=5;              //Variable to store the showing Volume
int prev_song_time=0;   //Variable to store the previous time
int cur_song_time=0;    //Variable to store the current time
int previous_volume;    //Variable to store the volume when muted
bool pause=false;       //Variable to store the status of Pause button 
bool mute=false;        //Variable to store the status of mute button
bool play_new=false;    //Variable to store the status of new song button
bool shuf=false;

char current_song[50] = "/fs/music/";
char buffer[80];
char token_buf[4];

int check=0;    //Capacitative touch generates interrupt on both press and release. This variable tracks this and updates only on press.
char song_name[MAX_SONG_NUM][20]={"rollin", "add imagination","assignment song","go to the front","the love of god", "next level"}; //Array of song names entered manually
int song_len[MAX_SONG_NUM] = {198, 241, 172, 127, 407, 221};

void time_mark(){
    if(!pause)
        cur_song_time++;   
}

void display_OLED(){
    gOLED.clearDisplay();
    
    // Text
    gOLED.setTextCursor(0, 0);
    gOLED.setTextSize(2);
    gOLED.printf("%s", song_name[new_song_number]);
    gOLED.setTextCursor(16*6, 3*8);
    gOLED.setTextSize(1);
    gOLED.printf("%02d/%02d", new_song_number + 1, MAX_SONG_NUM);
    gOLED.setTextCursor(0, 5*8);
    if(!mute) gOLED.printf("VOL %02d", vol);
    else gOLED.printf("MUTE");
    gOLED.setTextCursor(16*6, 7*8);
    gOLED.printf("%02d:%02d", song_len[new_song_number] / 60, song_len[new_song_number] % 60);
    gOLED.setTextCursor(0, 7*8);
    gOLED.printf("%02d:%02d", cur_song_time / 60, cur_song_time % 60);    
    
    // Pictogram
    if (!pause) gOLED.drawTriangle(15*6, 4*8 + 4, 15*6, 7*8 - 4, 17*6, 44, WHITE);
    else {
        gOLED.fillRect(15*6, 4*8 + 4, 4, 16, WHITE);
        gOLED.fillRect(15*6 + 8, 4*8 + 4, 4, 16, WHITE);
    }
    
    if (!shuf){
        gOLED.drawLine(18*6 + 2, 40, 20*6 + 2, 40, WHITE);
        gOLED.drawLine(20*6, 38, 20*6 + 2, 40, WHITE);
        gOLED.drawLine(20*6, 42, 20*6 + 2, 40, WHITE);
    
        gOLED.drawLine(18*6 + 2, 48, 20*6 + 2, 48, WHITE);
        gOLED.drawLine(20*6, 46, 20*6 + 2, 48, WHITE);
        gOLED.drawLine(20*6, 50, 20*6 + 2, 48, WHITE);
    }
    else{
        gOLED.drawLine(18*6 + 2, 40, 19*6, 40, WHITE);
        gOLED.drawLine(19*6, 40, 19*6 + 4, 48, WHITE);
        gOLED.drawLine(19*6 + 4, 40, 20*6 + 2, 40, WHITE);
        gOLED.drawLine(20*6, 38, 20*6 + 2, 40, WHITE);
        gOLED.drawLine(20*6, 42, 20*6 + 2, 40, WHITE);
    
        gOLED.drawLine(18*6 + 2, 48, 19*6, 48, WHITE);
        gOLED.drawLine(19*6, 48, 19*6 + 4, 40, WHITE);
        gOLED.drawLine(19*6 + 4, 48, 20*6 + 2, 48, WHITE);
        gOLED.drawLine(20*6, 46, 20*6 + 2, 48, WHITE);
        gOLED.drawLine(20*6, 50, 20*6 + 2, 48, WHITE);
    
    }
    
    
    gOLED.display(); 
}

void operate_data(uint8_t *buf, int bitlength) { 
    const int n = bitlength / 8 + (((bitlength % 8) != 0) ? 1 : 0); 
    for (int i = 0; i < n; i++) { 
        sprintf(token_buf, "%x", buf[i]);
        pc.write(token_buf, strlen(token_buf)); 
    } 
    pc.write("\r\n", 2);
    
    if(strcmp(token_buf, "ba") == 0){
        sprintf(buffer, "It's CH-\r\n");
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "b9") == 0){
        sprintf(buffer, "It's CH\r\n");
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "b8") == 0){
        sprintf(buffer, "It's CH+\r\n");
        shuf = (!shuf);
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "bb") == 0){
        sprintf(buffer, "It's <<\r\n");
        if(cur_song_time >= 2){
            cur_song_time = 0;
            prev_song_time = 0;
            cur_song_number = -1;
        }
        else{
            new_song_number -= 1;               // Goto Prev song on completion of one song
            if(new_song_number == -1)
                new_song_number = MAX_SONG_NUM - 1; 
        }
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "bf") == 0){
        sprintf(buffer, "It's >>\r\n");
        new_song_number += 1;               // Goto Next song on completion of one song
        if(new_song_number == MAX_SONG_NUM)
            new_song_number = 0; 
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "bc") == 0){
        sprintf(buffer, "It's >||\r\n");
        pause = (!pause);
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "f8") == 0){
        sprintf(buffer, "It's -\r\n");
        volume_set = volume_set - 10;
        vol--;
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "ea") == 0){
        sprintf(buffer, "It's +\r\n");
        volume_set = volume_set + 10;
        vol++;
        pc.write(buffer, strlen(buffer));  
    }    
    else if(strcmp(token_buf, "f6") == 0){
        sprintf(buffer, "It's EQ\r\n");
        mute = (!mute);
        if(mute){
               previous_volume = mp3.getVolume();
               volume_set = -100.0;
        }else{
               volume_set = previous_volume;
        }
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "e6") == 0){
        sprintf(buffer, "It's 100+\r\n");
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "f2") == 0){
        sprintf(buffer, "It's 200+\r\n");
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "e9") == 0){
        sprintf(buffer, "It's 0\r\n");
        new_song_number = 0;
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "f3") == 0){
        sprintf(buffer, "It's 1\r\n");
        new_song_number = 1;
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "e7") == 0){
        sprintf(buffer, "It's 2\r\n");
        new_song_number = 2;
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "a1") == 0){
        sprintf(buffer, "It's 3\r\n");
        new_song_number = 3;
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "f7") == 0){
        sprintf(buffer, "It's 4\r\n");
        new_song_number = 4;
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "e3") == 0){
        sprintf(buffer, "It's 5\r\n");
        new_song_number = 5;
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "a5") == 0){
        sprintf(buffer, "It's 6\r\n");
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "bd") == 0){
        sprintf(buffer, "It's 7\r\n");
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "ad") == 0){
        sprintf(buffer, "It's 8\r\n");
        pc.write(buffer, strlen(buffer));  
    }
    else if(strcmp(token_buf, "b5") == 0){
        sprintf(buffer, "It's 9\r\n");
        pc.write(buffer, strlen(buffer));  
    }
        
    display_OLED();
}

 int main () 
 {       
    ThisThread::sleep_for(chrono::milliseconds(300));

    gOLED.clearDisplay();
    
    gOLED.setTextSize(2);
    gOLED.printf(" Min-Ju &\r\n");
    gOLED.printf(" Ji-hoon!\r\n\n");
    gOLED.printf("MP3 Player");
    gOLED.display();
 
    ThisThread::sleep_for(chrono::milliseconds(2000));
    
    gOLED.clearDisplay();
    gOLED.display();
 
     /*============================================================ 
      * MP3 Initialising 
      *==========================================================*/
     unsigned char array1[512]; 
     
     mp3._RST = 1; 
     mp3.cs_high();                                  //chip disabled 
     mp3.spi_initialise();                           //initialise MBED 
     mp3.sci_write(0x00,(SM_SDINEW+SM_STREAM+SM_DIFF)); 
     mp3.sci_write(0x03, 0x9800); 
     mp3.sdi_initialise();  
     
     
     sprintf(buffer, "\n\r MP3 Player \n\r");
     pc.write(buffer,strlen(buffer));
     
     ThisThread::sleep_for(chrono::milliseconds(300));
     
     
    /*============================================================ 
    * IR Receiver Initialising 
    *==========================================================*/    
     
    RemoteIR::Format format;
    uint8_t buf[32];
    int bitcount = 0;
    
    ThisThread::sleep_for(chrono::milliseconds(300));
    
    
    /*============================================================ 
    * File Receiving & Initialize
    *==========================================================*/    
     
    int err = fs.mount(bd);
    if (err) {
        // Reformat if we can't mount the filesystem
        err = fs.reformat(bd);
        if (err) {
            error("error: %s (%d)\n", strerror(-err), err);
        }
    }
    
    strcat(current_song, song_name[cur_song_number]);
    strcat(current_song, ".mp3");
    
    /*============================================================ 
    * Play the Song
    *==========================================================*/    
    
    while(true){
        display_OLED();
        
        song = fopen(current_song, "r");
    
        if(!song){
            sprintf(buffer, "\n\r Error!! \n\r");
            pc.write(buffer,strlen(buffer));
            ThisThread::sleep_for(chrono::milliseconds(10));
            exit(1);
        }
        
        cur_song_time = 0;
        tic.attach(&time_mark, 1s);
        while(!feof(song))
        {
                if(!pause){
                    fread(&array1, 1, 512, song);           
                    for(int i=0; i<512; i++)
                    {
                        mp3.sdi_write(array1[i]);
                    }
                    mp3.setVolume(volume_set);
                }
           
                if(rec.getState() == ReceiverIR::Received && check == 0){
                    bitcount = rec.getData(&format, buf, sizeof(buf) * 8);
                    operate_data(buf, bitcount);
                    sprintf(buffer, "\n\r Check it \n\r");
                    pc.write(buffer,strlen(buffer));
                    check = 1;
                }
                else if(rec.getState() == ReceiverIR::Receiving && check == 1){
                    check = 0;   
                }
            
                if(new_song_number != cur_song_number){
                    play_new = true;
                    break;
                }
                
                if (cur_song_time != prev_song_time){
                    prev_song_time = cur_song_time;
                    gOLED.setTextSize(1);
                    gOLED.setTextCursor(0, 7*8);
                    gOLED.printf("%02d:%02d", cur_song_time / 60, cur_song_time % 60);
                    gOLED.display();     
                }
        }
        fclose(song);
        tic.detach();

        
        if(!shuf || cur_song_number == -1){
            if(play_new){ 
                cur_song_number = new_song_number;
                play_new=false;
            }
            else{
                new_song_number += 1;               // Goto Next song on completion of one song
                if(new_song_number == MAX_SONG_NUM)
                    new_song_number = 0; 
                cur_song_number = new_song_number;
            }
        }
        else{
            do{
                srand((unsigned int) time(NULL));
                new_song_number = rand() % MAX_SONG_NUM;
            }while(new_song_number == cur_song_number);
            play_new=false;
            cur_song_number = new_song_number;
        }
        
        strcpy(current_song, "/fs/music/");
        strcat(current_song, song_name[cur_song_number]);
        strcat(current_song, ".mp3");
    }

 } 
 
 