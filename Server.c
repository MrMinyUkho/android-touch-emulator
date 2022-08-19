#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
    
#define PORT     20001
#define MAXLINE     64          // Buffer size

int ev_ID;
int f;
bool istouch = false;
uint16_t prev = 0;
int touches[10];
uint16_t curr = 0;

bool mouse_up = false;

struct input_event {
    struct timeval time;
    uint16_t type;
    uint16_t code;
    uint32_t value;
};

#define EVIOCGVERSION     _IOR('E', 0x01, int)                                  /* get driver version */
#define EVIOCGID          _IOR('E', 0x02, struct input_id)                      /* get device ID */
#define EVIOCGKEYCODE     _IOR('E', 0x04, int[2])                               /* get keycode */
#define EVIOCSKEYCODE     _IOW('E', 0x04, int[2])                               /* set keycode */
#define EVIOCGNAME(len)   _IOC(_IOC_READ, 'E', 0x06, len)                       /* get device name */
#define EVIOCGPHYS(len)   _IOC(_IOC_READ, 'E', 0x07, len)                       /* get physical location */
#define EVIOCGUNIQ(len)   _IOC(_IOC_READ, 'E', 0x08, len)                       /* get unique identifier */
#define EVIOCGKEY(len)    _IOC(_IOC_READ, 'E', 0x18, len)                       /* get global keystate */
#define EVIOCGLED(len)    _IOC(_IOC_READ, 'E', 0x19, len)                       /* get all LEDs */
#define EVIOCGSND(len)    _IOC(_IOC_READ, 'E', 0x1a, len)                       /* get all sounds status */
#define EVIOCGSW(len)     _IOC(_IOC_READ, 'E', 0x1b, len)                       /* get all switch states */
#define EVIOCGBIT(ev,len) _IOC(_IOC_READ, 'E', 0x20 + ev, len)                  /* get event bits */
#define EVIOCGABS(abs)    _IOR('E', 0x40 + abs, struct input_absinfo)           /* get abs value/limits */
#define EVIOCSABS(abs)    _IOW('E', 0xc0 + abs, struct input_absinfo)           /* set abs value/limits */
#define EVIOCSFF          _IOC(_IOC_WRITE, 'E', 0x80, sizeof(struct ff_effect)) /* send a force effect to a force feedback device */
#define EVIOCRMFF         _IOW('E', 0x81, int)                                  /* Erase a force effect */
#define EVIOCGEFFECTS     _IOR('E', 0x84, int)                                  /* Report number of effects playable at the same time */
#define EVIOCGRAB         _IOW('E', 0x90, int)                                  /* Grab/Release device */

void send_ev(uint16_t t, uint16_t c, uint32_t v){
    
    ssize_t ret;

    struct input_event event;
    
    memset(&event, 0, sizeof(event));
    
    event.type = t;
    event.code = c;
    event.value = v;
    
    ret = write(f, &event, sizeof(event));
}

int choise_by_touch(int t_marker){
    for(int i = 0; i < 10; ++i){
        if(touches[i] == t_marker){
            return i;
        }
    }
    return -1;
}

void press(uint32_t x, uint32_t y, int t){
    ev_ID+=1;

    int tn = 0;

    istouch = false;
    for(int i = 0; i < 10; ++i){
        if(touches[i] != 0){
            istouch = true;
        }
    }
    
    tn = choise_by_touch(t);
    if(tn != -1) return;

    for(int i = 0; i < 10; ++i){
        if(touches[i] == 0){
            tn = i;
        }
    }

    if(tn == -1) return;

    if(tn != prev){
        send_ev(3, 47, tn);    // ABS_MT_SLOT
    }
    
    send_ev(3, 57, ev_ID);    // ABS_MT_TRACKING_ID
    send_ev(3, 58, 20);       // ABS_MT_PRESSURE
    send_ev(3, 48, 15);       // ABS_MT_TOUCH_MAJOR
    
    if(tn != prev){
        send_ev(3, 47, tn);    // ABS_MT_SLOT
    }
    
    send_ev(3, 54, x);        // ABS_MT_POSITION_Y
    send_ev(3, 53, 1080 - y); // ABS_MT_POSITION_X
    
    if(istouch == false){
        send_ev(0, 0, 0);     // finish transaction
        send_ev(1, 330, 1);   // BTN_TOUCH
    }
    
    send_ev(0, 0, 0);         // finish transaction
    prev = tn;
}

void change_xy(uint32_t x, uint32_t y, int t){
    int tn = 0;

    tn = choise_by_touch(t);
    if(tn == -1) return

    if (tn != prev) {
        send_ev(3, 47, t);    // ABS_MT_SLOT
    }

    send_ev(3, 54, x);        // ABS_MT_POSITION_Y
    send_ev(3, 53, 1080 - y); // ABS_MT_POSITION_X
    send_ev(0, 0, 0);         // finish transaction
    
    prev = tn;
}

void release(int t){

    int tn = 0;

    tn = choise_by_touch(t);
    if(tn == -1) return

    if(t != prev){
        send_ev(3, 47, tn);    // ABS_MT_SLOT
    }
    send_ev(3, 57, 4294967295); // ABS_MT_TRACKING_ID

    istouch = false;
    for(int i = 0; i < 10; ++i){
        if(touches[i] != 0){
            istouch = true;
        }
    }

    if(istouch == false){
        send_ev(0, 0, 0);     // finish transaction
        send_ev(1, 330, 0);   // BTN_TOUCH
    }
    send_ev(0, 0, 0);         // finish transaction
    touches[tn] = 0;
    prev = tn;
}

int main(int argc, char *argv[]) {
    
    ev_ID = 0;

    f = open("/dev/input/event1", O_RDWR);
    
    memset(&touches, 0, sizeof(touches));

    int sockfd; 
    char buffer[MAXLINE]; 
    struct sockaddr_in servaddr, cliaddr; 
        
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
        
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr,  0, sizeof(cliaddr)); 
        
    // Filling server information 
    servaddr.sin_family      = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = inet_addr("0.0.0.0"); // If it does not work, then replace it 
                                                     // with the address of the smartphone in the local network
    servaddr.sin_port        = htons(PORT);

    printf("1%d", INADDR_ANY);
        
    // Bind the socket with the server address 
    if (bind(sockfd, (const struct sockaddr *)&servaddr,  
        sizeof(servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
        
    int len, n; 
    
    len = sizeof(cliaddr);  //len is value/result 
    
    while(1){
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                     MSG_WAITALL, ( struct sockaddr *) &cliaddr, 
                     &len); 
        buffer[n] = '\0';
        if(buffer[0] == 'p' || buffer[0] == 'c'){           // press or drag command "p" or "c" + "x len = 4" + "y len = 4" + "*touch marker* len = 2"
            char x_s[4];
            char y_s[4];
            char m_s[2];
            int x;
            int y;
            int m;

            x_s[0] = buffer[1];
            x_s[1] = buffer[2];
            x_s[2] = buffer[3];
            x_s[3] = buffer[4];

            y_s[0] = buffer[5];
            y_s[1] = buffer[6];
            y_s[2] = buffer[7];
            y_s[3] = buffer[8];

            m_s[0] = buffer[9];
            m_s[1] = buffer[10];

            x = atoi(x_s);
            y = atoi(y_s);
            m = atoi(m_s);

            if(buffer == 'p'){
                press(x, y, t);
            } else {
                change_xy(x, y, t);
            }
        } else if(buffer[0] == 'r'){                        // release command "r" + "*touch marker* len = 2"
            int m;

            m_s[0] = buffer[9];
            m_s[1] = buffer[10];

            m = atoi(m_s);

            release(m);
        }
    }
    return 0;
}
