#define HOST_IP_ADDR_WT32 "192.168.50.205" // Hard-coded to send data to MaxD //
#define PORT_WT32 3333
#define PORT 3333

bool muteRecords[8] = {0,0,0,0,0,0,0,0}; // mute info per note
int note; // 4 bits note info // 8 notes correspond to 8 colors // (0-7) -> (36,38,43,50,42,46,39,75),67,49 // más de 8 !

// Attempt at storing a sequence as an array of struct //
// Need to find out how to make it possible to store two notes on the same step. For example, there might be a 'Bass drum' and a High hat' on the same step.

typedef struct {
    bool on;          
    uint8_t chan;
    uint8_t bar;
    uint8_t note;
    uint8_t length;
    bool mute;
} steps_t;

steps_t steps[64]; // Declare an array of type struct steps

#define N_CLIENTS 8
#define N_NOTES_PER_CLIENT 128
char clientIPAddresses[N_CLIENTS][22]; // 8 potential clients, IPv6 format + 1 for string termination by strncat // somehow the space need increased by '1'...perhaps the previous access point attributed values below '100' for the last value
uint16_t seq[N_CLIENTS][N_NOTES_PER_CLIENT] = {0}; // array of channel x note where each element represents the sequence in bits
bool seq_changed[N_CLIENTS][N_NOTES_PER_CLIENT] = {0}; // array of channel x note where each element represents a flag indicating if the sequence has been updated
int nmbrClients = 0;
bool loadedSeq[1264] = {}; // to store the loaded sequences
int loadedClients = 0;
char str_ip[16] = "192.168.0.66"; // send IP to clients !! // stand in ip necessary for memory space?

extern "C" {
  #include "lwip/err.h" // udp_server example
  #include "lwip/sockets.h"
  #include "lwip/sys.h"
  #include <lwip/netdb.h>
  
  static const char *SOCKET_TAG = "Socket";
  static const char *SEQ_TAG = "SEQ";

  char addr_str[128]; // Copié depuis le client
  int addr_family;    
  int ip_protocol;
  int sock_WT32 = 0;
  struct sockaddr_in dest_addr_WT32;
  }
  
int clientIPCheck ( char myArray[] ) { // ESP_LOGI(SOCKET_TAG, "This is myArray : %s", myArray); 
    for ( int i = 0; i < 8; i++ ) { // max of 8 IP addresses connected
      if( strcmp( myArray, clientIPAddresses[i] ) == 0 ) {
        return i; // IP Address already exists at position 'i' in the array
        break; 
        }
    }
    return 42; // not in the array, add it
  }


////////////////////// sockette server ///////////////////////
//#if defined USE_SOCKETS

int sock; 

extern "C" {
  static void udp_server_task(void *pvParameters)
  {

    char mstr[16]; // char mstr[64]
    char addr_str[128];
    int addr_family = AF_INET6;
    int ip_protocol = IPPROTO_IPV6;
    struct sockaddr_in6 dest_addr; // IPV6

    while (1) { 

        bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
        dest_addr.sin6_family = AF_INET6;
        dest_addr.sin6_port = htons(PORT);
        
        inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);

        sock = socket(addr_family, SOCK_DGRAM, ip_protocol);

        ESP_LOGI(SOCKET_TAG, "Socket created id : %i", sock);

        if (sock < 0) {
            ESP_LOGE(SOCKET_TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        ESP_LOGI(SOCKET_TAG, "Socket created");

        int err1 = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        if (err1 < 0) {
            ESP_LOGE(SOCKET_TAG, "Socket unable to bind: errno %d", errno);
            ESP_LOGE(SOCKET_TAG, "Error occurred during sending: errno %d", errno);
            break;
        }

        ESP_LOGI(SOCKET_TAG, "Socket bound, port %d", PORT);

        while (1) {

          ESP_LOGI(SOCKET_TAG, "Waiting for data\n");

          struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
          socklen_t socklen = sizeof(source_addr);
            
          //mstr should be 79 values long;
          int len = recvfrom(sock, mstr, sizeof(mstr), 0, (struct sockaddr *)&source_addr, &socklen);
           
          // for(int i = 0; i<sizeof(mstr); i++){
          //   test = mstr[i];
          //   ESP_LOGE(SOCKET_TAG, "data %i", test);
          // }

          /////////////////// FROM TINYOSC ///////////////////////
          //tosc_printOscBuffer(rx_buffer, len);
          tosc_printOscBuffer(mstr, len);

            tosc_message osc; // declare the TinyOSC structure
            char buffer[1024]; // char buffer[64]; declare a buffer into which to read the socket contents
            //int oscLen = 0; // the number of bytes read from the socket

            // tosc_parseMessage(&osc, rx_buffer, len);
            tosc_parseMessage(&osc, mstr, len);

            //tosc_printMessage(&osc);
            tosc_getAddress(&osc),  // the OSC address string, e.g. "/button1"
            tosc_getFormat(&osc);  // the OSC format string, e.g. "f"

            //printf("%s %s", 
            ////&osc->len,              // the number of bytes in the OSC message
            //tosc_getAddress(&osc),  // the OSC address string, e.g. "/button1"
            //tosc_getFormat(&osc));  // the OSC format string, e.g. "f"

           //switch (tosc_getFormat(&osc)) {
            switch(osc.format[0]) {
    
                case 'm': {
                    unsigned char *m = tosc_getNextMidi(&osc);
                    printf(" 0x%02X%02X%02X%02X", m[0], m[1], m[2], m[3]);
                    printf("\n");
                    //CCChannel = m[0];
                    //sensorValue = m[1];
                    //CCmessage = m[2];
                    steps[m[3]].on = m[2]; // m[2] is on/off info and m[3] is the step number

                    // If the sequencer is going to only be 16 steps...we might not need all these 64
                    /* for (int i = 0 ; i < 64 ; i++ ){
                      ESP_LOGI(SEQ_TAG, "step : %i, note value :%i", i, steps[i].on); 
                    } */

                    //printf("%02X",m[0]);
                    //printf("\n");
      
                    break;
                }
    
                default:
                    printf(" Unknown format: '%c'", osc.format[0]);
                    break;
                } 
  
                printf("\n");
            /////////////////// END TINYOSC ///////////////////////

            ///// midi channel ///// 1-16 midi channels
            // ESP_LOGI(SOCKET_TAG, "channel : %i", channel); 
  
            ///// note /////
            ESP_LOGI(SOCKET_TAG, "note : %i", note); 

            ///// noteDuration //////
            // ESP_LOGI(SOCKET_TAG, "noteDuration : %f", noteDuration); 

            ///// Step ///// 0-63 steps
            //ESP_LOGI(SOCKET_TAG, "note : %i", note); // Change this
            // Where is the note going in the sequence
          
            // read in the bit value for mute and store 
            muteRecords[note] = mstr[14];
            // ESP_LOGI(SOCKET_TAG, "mute ? : %i", mstr[10]);  
          
            // Error occurred during receiving ?
            if (len < 0) {
                ESP_LOGE(SOCKET_TAG, "recvfrom failed: errno %d", errno);
                break;
              }
            
            else {   // Data received
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1); // Get the sender's ip address as string
              }
                
           ESP_LOGI(SOCKET_TAG, "Received %d bytes from %s:", len, addr_str);
                
            //inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1); // Get the sender's ip address as string
            int checkIPExist = clientIPCheck(addr_str); // Does it exist in the array?
            uint8_t chan = checkIPExist; // hacky way to get "channel" // If the the IP doesn't exist, channel is set to '42' ? // second time it's '0' because it exists
            // ESP_LOGI(SEQ_TAG, "chan %d", chan);

            uint8_t note = mstr[13];
            bool on = mstr[14];
            uint8_t step = mstr[15] % 16; // there are 64 steps but sequencer only goes to 16. just loop them for now
            uint16_t step_mask = 1 << step;

            if ( note != 66 ){ // First time an IP registers we don't want any values in seq[chan][note]
              seq_changed[chan][note] = true;
              if (on) {
                seq[chan][note] |= step_mask;
              } else{
                seq[chan][note] &= ~step_mask;
              }

              char seq_string[17] = {0};
              for (int pos = 0; pos < 16; pos++) {
                if (seq[chan][note] & (1 << pos)) {
                  seq_string[pos] = 'X';
                } else {
                  seq_string[pos] = 'O';
                }
              }
              // Otherwise we had : I (9244) Sequence: setting chan 42 note 66 step 2 to 1. 04
              if (on) {
                ESP_LOGI(SEQ_TAG, "chan %d note %d on: %s", chan, note, seq_string);
              } else {
                ESP_LOGI(SEQ_TAG, "chan %d note %d off: %s", chan, note, seq_string);
              }
            }

            if ( startStopState == false ) { // only check for clients if we are in stopped mode, it hangs the playback otherwise
              
              // int checkIPExist = clientIPCheck(addr_str); // Does it exist in the array?

             // ESP_LOGI(SOCKET_TAG, "result of clientIPCheck : %i", checkIPExist);

              if ( checkIPExist == 42 ) { // if it doesn't exist, add it
              
                strncat(clientIPAddresses[nmbrClients], inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str)-1),21); // add that address to the array // needs to be one lower that the array declared to hold the IPs

                ESP_LOGI(SOCKET_TAG, "Added client address : %s", clientIPAddresses[nmbrClients]);

                nmbrClients++; // Count the newly registered client
                ESP_LOGI(SOCKET_TAG, "How many clients ? : %i", nmbrClients);

                }

              else { // IP already exists 

                ESP_LOGI(SOCKET_TAG, "Address already exists : %s", addr_str);    // ip address already exists in the array so do nothing // at what position
                
                if ( seqToLoad ) { // 

                  ESP_LOGI(SOCKET_TAG, "Sending love instead of an IP to : %s", addr_str); 

                  bool tmpArray[79];
                  
                  for ( int i = 0; i < 80; i++ ) { // checkIPExist is the offset

                    tmpArray[i] = loadedSeq[i+(79*checkIPExist)]; // populate tmpmstr array
                    ESP_LOGI(SOCKET_TAG, "tmpArray[%i] = %i ", i, tmpArray[i]); 

                  }

                  int err = sendto(sock, tmpArray, sizeof(tmpArray), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                  loadedClients++; // keep track of how many clients have been loaded
                  ESP_LOGI(SOCKET_TAG, "loadedClients : %i\n", loadedClients); 
                  
                  if( loadedClients == nmbrClients ) { // All the clients are loaded

                    seqToLoad = false;
                    loadedClients = 0;

                    }
                    
                }

              } // End of "else if IP exists"

              ESP_LOGI(SOCKET_TAG, "nmbrClients : %i\n", nmbrClients); 
            
            } 

            int err = sendto(sock, str_ip, sizeof(str_ip), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
            ESP_LOGI(SOCKET_TAG, "Sent my IP %s", str_ip); 
            
            
     
        } // End of while

        if (sock != -1) {
            ESP_LOGI(SOCKET_TAG, "Shutting down socket and restarting");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
  }
}
//#endif
////////// fin sockette server //////////////


