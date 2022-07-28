# android-touch-emulator
This is a client-server application that allows you to control your smartphone from another device.

In the source code, the program allows you to control PUBG from a computer, multi-touch is implemented.

It is based on the 'sendevent.c' source code from Google. It allows you to send events to input devices located in '/dev/input/eventX' where X is the device number. You can view a list of your devices using the command (requires root):
'''sh
getevents -p
'''
If you do not specify parameters, then the command will display events occurring on the device, such as screen touches, button presses, vibration, and others.

------

'Server.c' must be compiled for the architecture of your smartphone, the most popular nowadays is 'aarch64'

------

For the convenience of debugging and satisfying my perverse fantasies, the entire server part was written on the phone in a chroot in Arch Linux ARM.

------


