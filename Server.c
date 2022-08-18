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
#define MAXLINE     64

int ev_ID;
int f;
bool istouch = false;
uint16_t prev = 0;
int touches[10];
uint16_t curr = 0;
int32_t rx = 0;
int32_t ry = 0;

int32_t tx = 0;
int32_t ty = 0;

int32_t trx = 0;
int32_t try = 0;

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

void press(uint32_t x, uint32_t y, uint16_t t){
    ev_ID+=1;
    if(t != prev){
        send_ev(3, 47, t);    // ABS_MT_SLOT
    }
    send_ev(3, 57, ev_ID);    // ABS_MT_TRACKING_ID
    send_ev(3, 58, 20);       // ABS_MT_PRESSURE
    send_ev(3, 48, 15);       // ABS_MT_TOUCH_MAJOR
    if(t != prev){
        send_ev(3, 47, t);    // ABS_MT_SLOT
    }
    send_ev(3, 54, x);        // ABS_MT_POSITION_Y
    send_ev(3, 53, 1080 - y); // ABS_MT_POSITION_X

    istouch = false;
    for(int i = 0; i < 10; ++i){
        if(touches[i] != 0){
            istouch = true;
        }
    }
    
    if(istouch == false){
        send_ev(0, 0, 0);     // finish transaction
        send_ev(1, 330, 1);   // BTN_TOUCH
    }
    send_ev(0, 0, 0);         // finish transaction
    prev = t;
}

void change_xy(uint32_t x, uint32_t y, uint16_t t){
    if (t != prev) {          
        send_ev(3, 47, t);    // ABS_MT_SLOT
    }
    send_ev(3, 54, x);        // ABS_MT_POSITION_Y
    send_ev(3, 53, 1080 - y); // ABS_MT_POSITION_X
    send_ev(0, 0, 0);         // finish transaction
    prev = t;
}

void release(uint16_t t){
    if(t != prev){
        send_ev(3, 47, t);    // ABS_MT_SLOT
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
    touches[t] = 0;
    prev = t;
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
    memset(&cliaddr, 0, sizeof(cliaddr)); 
        
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = inet_addr("192.168.1.3"); 
    servaddr.sin_port = htons(PORT);

    printf("1%d", INADDR_ANY);
        
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,  
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

        if (buffer[0] == '1') {
            
            char buf[4];

            //printf("\nRecive %d %d %d %d %d %d %d %d %d %d\n", touches[0], touches[1], touches[2], touches[3], touches[4], touches[5], touches[6], touches[7], touches[8], touches[9]);
            
            buf[0] = buffer[1];
            buf[1] = buffer[2];
            buf[2] = buffer[3];
            buf[3] = buffer[4];

            tx = atoi(buf);
    
            buf[0] = buffer[5];
            buf[1] = buffer[6];
            buf[2] = buffer[7];
            buf[3] = buffer[8];

            ty = atoi(buf);

            if(tx != -999 && ty != -999){
                bool qwer = true;
                //printf("\n%d %d %d %d %d %d %d %d %d %d\n", touches[0], touches[1], touches[2], touches[3], touches[4], touches[5], touches[6], touches[7], touches[8], touches[9]);
                for(int i = 0; i < 10; ++i){
                    //printf("%d", i);
                    if(touches[i] == 2){
                        qwer = false;
                        if(prev != i){
                            curr = i;
                        } else {
                            curr = -1;
                        }
                        prev = i;
                        break;
                    }
                }
                if(qwer){
                    for(int i = 0; i < 10; ++i){
                        if(touches[i] == 0){
                            if(i == prev){
                                press(1250, 530, -1);
                                curr = -1;
                            } else {
                                press(1250, 530, i);
                                curr = -1;
                                prev = i;
                            }
                            touches[i] = 2;
                            break;
                        }
                    }
                    istouch = true;
                }
                change_xy(1250+tx, 530+ty, curr);        
            } else {
                istouch = false;
                for(int i = 0; i < 10; ++i){
                    if(touches[i] != 0){
                        istouch = true;
                    }
                }
                for(int i = 0; i < 10; ++i){
                    if(touches[i] == 2){
                        if(i == prev){
                            release(-1);
                        } else {
                            release(i);
                            prev = i;
                        }
                        touches[i] = 0;
                    }
                }
                continue;
            }
        } else if (buffer[0] == '3'){
            if(buffer[1] == '1'){
                bool qwer = true;
                //printf("\n%d %d %d %d %d %d %d %d %d %d\n", touches[0], touches[1], touches[2], touches[3], touches[4], touches[5], touches[6], touches[7], touches[8], touches[9]);
                for(int i = 0; i < 10; ++i){
                    //printf("%d", i);
                    if(touches[i] == 3){
                        qwer = false;
                        if(prev != i){
                            curr = i;
                        } else {
                            curr = -1;
                        }
                        prev = i;
                        break;
                    }
                }
                if(qwer){
                    for(int i = 0; i < 10; ++i){
                        if(touches[i] == 0){
                            if(i == prev){
                                press(2320, 870, -1);
                                curr = -1;
                            } else {
                                press(2320, 870, i);
                                curr = -1;
                                prev = i;
                            }
                            touches[i] = 3;
                            break;
                        }
                    }
                    istouch = true;
                }
            } else {
                istouch = false;
                for(int i = 0; i < 10; ++i){
                    if(touches[i] != 0){
                        istouch = true;
                    }
                }
                for(int i = 0; i < 10; ++i){
                    if(touches[i] == 3){
                        if(i == prev){
                            release(-1);
                        } else {
                            release(i);
                            prev = i;
                        }
                        touches[i] = 0;
                    }
                }
            }

        } else if (buffer[0] == '0'){
            int d = buffer[1] - '0';
            if(d > 0){
                bool qwer = true;
                //printf("\n%d %d %d %d %d %d %d %d %d %d\n", touches[0], touches[1], touches[2], touches[3], touches[4], touches[5], touches[6], touches[7], touches[8], touches[9]);
                for(int i = 0; i < 10; ++i){
                    //printf("%d", i);
                    if(touches[i] == 1){
                        qwer = false;
                        if(prev != i){
                            curr = i;
                        } else {
                            curr = -1;
                        }
                        prev = i;
                        break;
                    }
                }
                if(qwer){
                    for(int i = 0; i < 10; ++i){
                        if(touches[i] == 0){
                            if(i == prev){
                                press(415, 615, -1);
                                curr = -1;
                            } else {
                                press(415, 615, i);
                                curr = -1;
                                prev = i;
                            }
                            touches[i] = 1;
                            break;
                        }
                    }
                    istouch = true;
                }
            } else {
                istouch = false;
                for(int i = 0; i < 10; ++i){
                    if(touches[i] != 0){
                        istouch = true;
                    }
                }    
                for(int i = 0; i < 10; ++i){
                    if(touches[i] == 1){
                        if(i == prev){
                            release(-1);
                        } else {
                            release(i);
                            prev = i;
                        }
                        touches[i] = 0;
                    }
                }
            }
    
            switch(d){
                case 1:
                    change_xy(415, 490, curr);
                break;
                case 2:
                    change_xy(415, 360, curr);
                break;
                case 3:
                    change_xy(320, 545, curr);
                break;
                case 4:
                    change_xy(515, 545, curr);
                break;
                case 5:
                    change_xy(305, 615, curr);
                break;
                case 6:
                    change_xy(540, 615, curr);
                break;
                case 7:
                    change_xy(330, 700, curr);
                break;
                case 8:
                    change_xy(500, 700, curr);
                break;
                case 9:
                    change_xy(415, 740, curr);
                break;
            }
        } else if (buffer[0] == '2'){
            if(buffer[1] == '1'){
                bool qwer = true;
                //printf("\n%d %d %d %d %d %d %d %d %d %d\n", touches[0], touches[1], touches[2], touches[3], touches[4], touches[5], touches[6], touches[7], touches[8], touches[9]);
                for(int i = 0; i < 10; ++i){
                    //printf("%d", i);
                    if(touches[i] == 4){
                        qwer = false;
                        if(prev != i){
                            curr = i;
                        } else {
                            curr = -1;
                        }
                        prev = i;
                        break;
                    }
                }
                if(qwer){
                    for(int i = 0; i < 10; ++i){
                        if(touches[i] == 0){
                            if(i == prev){
                                press(75, 775, -1);
                                curr = -1;
                            } else {
                                press(75, 775, i);
                                curr = -1;
                                prev = i;
                            }
                            touches[i] = 4;
                            break;
                        }
                    }
                    istouch = true;
                }
            } else {
                istouch = false;
                for(int i = 0; i < 10; ++i){
                    if(touches[i] != 0){
                        istouch = true;
                    }
                }
                for(int i = 0; i < 10; ++i){
                    if(touches[i] == 4){
                        if(i == prev){
                            release(-1);
                        } else {
                            release(i);
                            prev = i;
                        }
                        touches[i] = 0;
                    }
                }
            }

        } else if (buffer[0] == '4'){
            if(buffer[1] == '1'){
                bool qwer = true;
                //printf("\n%d %d %d %d %d %d %d %d %d %d\n", touches[0], touches[1], touches[2], touches[3], touches[4], touches[5], touches[6], touches[7], touches[8], touches[9]);
                for(int i = 0; i < 10; ++i){
                    //printf("%d", i);
                    if(touches[i] == 5){
                        qwer = false;
                        if(prev != i){
                            curr = i;
                        } else {
                            curr = -1;
                        }
                        prev = i;
                        break;
                    }
                }
                if(qwer){
                    for(int i = 0; i < 10; ++i){
                        if(touches[i] == 0){
                            if(i == prev){
                                press(1840, 860, -1);
                                curr = -1;
                            } else {
                                press(1840, 860, i);
                                curr = -1;
                                prev = i;
                            }
                            touches[i] = 5;
                            break;
                        }
                    }
                    istouch = true;
                }
            } else {
                istouch = false;
                for(int i = 0; i < 10; ++i){
                    if(touches[i] != 0){
                        istouch = true;
                    }
                }
                for(int i = 0; i < 10; ++i){
                    if(touches[i] == 5){
                        if(i == prev){
                            release(-1);
                        } else {
                            release(i);
                            prev = i;
                        }
                        touches[i] = 0;
                    }
                }
            }

        } else if (buffer[0] == '5'){
            if(buffer[1] == '1'){
                bool qwer = true;
                //printf("\n%d %d %d %d %d %d %d %d %d %d\n", touches[0], touches[1], touches[2], touches[3], touches[4], touches[5], touches[6], touches[7], touches[8], touches[9]);
                for(int i = 0; i < 10; ++i){
                    //printf("%d", i);
                    if(touches[i] == 6){
                        qwer = false;
                        if(prev != i){
                            curr = i;
                        } else {
                            curr = -1;
                        }
                        prev = i;
                        break;
                    }
                }
                if(qwer){
                    for(int i = 0; i < 10; ++i){
                        if(touches[i] == 0){
                            if(i == prev){
                                press(1980, 850, -1);
                                curr = -1;
                            } else {
                                press(1980, 850, i);
                                curr = -1;
                                prev = i;
                            }
                            touches[i] = 6;
                            break;
                        }
                    }
                    istouch = true;
                }
            } else {
                istouch = false;
                for(int i = 0; i < 10; ++i){
                    if(touches[i] != 0){
                        istouch = true;
                    }
                }
                for(int i = 0; i < 10; ++i){
                    if(touches[i] == 6){
                        if(i == prev){
                            release(-1);
                        } else {
                            release(i);
                            prev = i;
                        }
                        touches[i] = 0;
                    }
                }
            }

        } else if (buffer[0] == '6'){
            for(int i = 0; i < 10; ++i){
                if(touches[i] == 0){
                    if(prev == i){
                        curr = -1;
                    } else {
                        curr = i;
                    }
                    if(buffer[1] == '1'){
                        press(200, 760, curr);
                    } else if(buffer[1] == '2'){
                        press(450, 1000, curr);
                    } else if(buffer[1] == '3'){
                        press(580, 790, curr);
                    } else if(buffer[1] == '4'){
                        press(300, 800, curr);
                    } else if(buffer[1] == '5'){
                        press(2300, 750, curr);
                    } else if(buffer[1] == '7'){
                        press(1980, 970, curr);
                    } else if(buffer[1] == '8'){
                        press(1650, 1000, curr);
                    } else if(buffer[1] == '9'){
                        press(700, 1000, curr);
                    } else if(buffer[1] == 'a'){
                        press(240, 930, curr);
                    }
                    release(-1);
                    break;
                }
            }
        }         
    }
    return 0;
}
