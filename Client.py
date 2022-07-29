gmport socket
import time
import pygame
import os

pygame.init()

print("Test")

#msgFromClient       = "Hello UDP Server"
#bytesToSend         = str.encode(msgFromClient)
serverAddressPort   = ("192.168.1.3", 20001)
bufferSize          = 128

W, H = 1400, 800

sc = pygame.display.set_mode((W, H))

# Create a UDP socket at client side
UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

fwd = False
bwd = False
lef = False
rig = False



flag = False

prev = ""

active_touch = [False, False, False, False, False, False, False, False, False, False]

fire = False
incline_r = False
incline_l = False
scope = False

inv = False

prev_pos = (0, 0)
prpr = (0, 0)

ttt = time.time()

ddd = time.time()

def clamp(n, smallest, largest): return max(smallest, min(n, largest))

# Send to server using created UDP socket
while True:
    if flag == True:
        flag = False
        continue
    pac = ""
    for i in pygame.event.get():
        if i.type == pygame.KEYDOWN:
            if i.key == pygame.K_w:
                fwd = True
            elif i.key == pygame.K_s:
                bwd = True
            elif i.key == pygame.K_a:
                lef = True
            elif i.key == pygame.K_d:
                rig = True
            elif i.key == pygame.K_q:
                UDPClientSocket.sendto(str.encode("41"), serverAddressPort)
            elif i.key == pygame.K_e:
                UDPClientSocket.sendto(str.encode("51"), serverAddressPort)
            elif i.key == pygame.K_SPACE:
                UDPClientSocket.sendto(str.encode("61"), serverAddressPort)
            elif i.key == pygame.KMOD_CTRL:
                UDPClientSocket.sendto(str.encode("62"), serverAddressPort)
            elif i.key == pygame.K_v:
                UDPClientSocket.sendto(str.encode("64"), serverAddressPort)
            elif i.key == pygame.K_r:
                UDPClientSocket.sendto(str.encode("65"), serverAddressPort)
            elif i.key == pygame.K_z:
                UDPClientSocket.sendto(str.encode("67"), serverAddressPort)
            elif i.key == pygame.K_g:
                UDPClientSocket.sendto(str.encode("68"), serverAddressPort)
            elif i.key == pygame.K_h:
                UDPClientSocket.sendto(str.encode("69"), serverAddressPort)
            elif i.key == pygame.K_i:
                UDPClientSocket.sendto(str.encode("6a"), serverAddressPort)
                inv = not inv
        elif i.type == pygame.KEYUP:
            if i.key == pygame.K_w:
                fwd = False
            elif i.key == pygame.K_s:
                bwd = False
            elif i.key == pygame.K_a:
                lef = False
            elif i.key == pygame.K_d:
                rig = False
            elif i.key == pygame.K_q:
                UDPClientSocket.sendto(str.encode("40"), serverAddressPort)
            elif i.key == pygame.K_e:
                UDPClientSocket.sendto(str.encode("50"), serverAddressPort)
        elif i.type == pygame.MOUSEBUTTONDOWN:
            if i.button == 1:
                UDPClientSocket.sendto(str.encode("31"), serverAddressPort)
            elif i.button == 2:
                UDPClientSocket.sendto(str.encode("63"), serverAddressPort)
            elif i.button == 3:
                scope = True
                UDPClientSocket.sendto(str.encode("21"), serverAddressPort)
        elif i.type == pygame.MOUSEBUTTONUP:
            if i.button == 1:
                fire = False
                UDPClientSocket.sendto(str.encode("30"), serverAddressPort)
            elif i.button == 3:
                UDPClientSocket.sendto(str.encode("20"), serverAddressPort)
                scope = False
    
    
    
    pos = pygame.mouse.get_pos()

    if fwd:
        pac += "f"
    elif bwd:
        pac += "b"
    if lef:
        pac += "l"
    elif rig:
        pac += "r"

    if pac == "":
        pac ="s"
    
    d = pac
    if d == "f":
        pac = 1
    elif d == "ff":
        pac = 2
    elif d == "fl":
        pac = 3
    elif d == "fr":
        pac = 4
    elif d == "l":
        pac = 5
    elif d == "r":
        pac = 6
    elif d == "bl":
        pac = 7
    elif d == "br":
        pac = 8
    elif d == "b":
        pac = 9
    else:
        pac = 0

    UDPClientSocket.sendto(str.encode("0"+str(pac)), serverAddressPort)
    
    prpr = [pos[0] - W/2, pos[1] - H/2]

    ddd = time.time()
    if inv == False and (prpr[0] > 400 or prpr[0] < -400 or prpr[1] > 300 or prpr[1] < -300):
        ttt = time.time()
        UDPClientSocket.sendto(str.encode("1-999-999"), serverAddressPort)
        #UDPClientSocket.sendto(str.encode("1-999-999"), serverAddressPort)
        pygame.mouse.set_pos((W / 2, H / 2))
        prpr = [0,0]
    else:
        pac = "1"
    
        pac += str(clamp(int(int(prpr[0])), -999, 999)).rjust(4, " ")
        pac += str(clamp(int(int(prpr[1])), -999, 999)).rjust(4, " ")
        UDPClientSocket.sendto(str.encode(pac), serverAddressPort)

    time.sleep(0.005)
