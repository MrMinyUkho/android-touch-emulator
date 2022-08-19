import socket

serverAddressPort   = ("192.168.1.3", 20001)
bufferSize          = 128

UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

print("Enter command('h' for help)")

while True:
    c = input("> ").split()
    pac = ""
    if c[0] == "h":
        print("X Y - coordinates of touch, M - touch marker")
        print("p X Y M           press(start touch)")
        print("c X Y M           change coordinates of touch")
        print("r M               release(stop touch)")
        print()
        print("q                 exit")
    elif c[0] not in ["p", "c", "r", "q"]:
        print("Unknown command. Type 'h' for help")
        continue 
    elif c[0] == "q":
        print("Goodbye!")
        quit()
    else c[0] == "p" or c[0] == "c":
        if len(c) == 4:
            print("Incorrect parameters")
        x = abs(int(c[1]))
        if x > 9999:
            print("Error, x > 9999")
            continue
        y = abs(int(c[2]))
        if y > 9999:
            print("Error, y > 9999")
            continue
        m = abs(int(c[3]))
        if m > 99:
            print("Error, m > 99")
            continue
        UDPClientSocket.sendto(str.encode(c[0] + c[1].rjust(4, " ") + c[2].rjust(4, " ") + c[3].rjust(2, " ")), serverAddressPort)
    else c[0] == "q":
        m = abs(int(c[1]))
        if m > 99:
            print("Error, m > 99")
            continue
        UDPClientSocket.sendto(str.encode(c[0] + c[1].rjust(2, " ")), serverAddressPort)