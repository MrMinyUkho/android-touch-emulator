# android-touch-emulator
This is a client-server application that allows you to control your smartphone from another device.

In the source code, the program allows you to control PUBG from a computer, multi-touch is implemented.

It is based on the `sendevent.c` source code from Google. It allows you to send events to input devices located in `/dev/input/eventX` where X is the device number. You can view a list of your devices using the command (requires root):
```sh
getevents -p
```
If you do not specify parameters, then the command will display events occurring on the device, such as screen touches, button presses, vibration, and others.

------

`Server.c` must be compiled for the architecture of your smartphone, the most popular nowadays is `aarch64`

------

For the convenience of debugging and satisfying my perverse fantasies, the entire server part was written on the phone in a chroot in Arch Linux ARM.

# How it work?

I didn't quite understand it myself, but I'll try to explain

`sendevent` takes as input the path to the event, the type of the event, the code of the event, and the value we send. For example send x position of the touch:
```sh
sendevent /dev/input/eventX 3 54 600
```

In code it look like this:
```C
void send_ev(uint16_t t, uint16_t c, uint32_t v){
	
	ssize_t ret;							// I don't know why it's necessary

	struct input_event event;				// Event variable
	
	memset(&event, 0, sizeof(event));		// Clear event
	
	event.type = t;
	event.code = c;
	event.value = v;
	
	ret = write(f, &event, sizeof(event));	// And here, too, I don’t 		the function simply 
											// know why we assign a			writes the event to 
											// result of the function to	the event file
											// the variable from the		
											// beginning of the function	
}
```

This is part of the output of the getevent -pl command
```
add device 1: /dev/input/event1
  name:     "fts_ts"
  events:
    KEY (0001): KEY_W                 KEY_E                 KEY_O                 KEY_S
                KEY_L                 KEY_Z                 KEY_C                 KEY_V
                KEY_M                 KEY_UP                KEY_LEFT              KEY_RIGHT
                KEY_DOWN              KEY_POWER             KEY_SLEEP             KEY_WAKEUP
                BTN_TOUCH
    ABS (0003): ABS_MT_SLOT           : value 0, min 0, max 9,		fuzz 0, flat 0, resolution 0
                ABS_MT_TOUCH_MAJOR    : value 0, min 0, max 255,	fuzz 0, flat 0, resolution 0
                ABS_MT_POSITION_X     : value 0, min 0, max 1080,	fuzz 0, flat 0, resolution 0
                ABS_MT_POSITION_Y     : value 0, min 0, max 2400,	fuzz 0, flat 0, resolution 0
                ABS_MT_TRACKING_ID    : value 0, min 0, max 65535,	fuzz 0, flat 0, resolution 0
                ABS_MT_PRESSURE       : value 0, min 0, max 255,	fuzz 0, flat 0, resolution 0
  input props:
    INPUT_PROP_DIRECT
```
1. `ABS_MT_SLOT` - index of touch(This is what we used to call "My phone supports 10 touches").
2. `ABS_MT_TOUCH_MAJOR` - width of touch, I don't use this, so that is constant. 
3. `ABS_MT_POSITION_X` - x position
4. `ABS_MT_POSITION_Y` - y position
5. `ABS_MT_TRACKING_ID` - touch ID. With each new touch, I increase it by one, I hope this is logical.
6. `ABS_MT_PRESSURE` - pressure of touch, I don't use this, so that is constant.

From `KEY event` we only need `BTN_TOUCH`, when we touch the touchscreen it's true, else - false. 

And so let's move on to how to send a touch(you can also see in code `Server.c` in functions `press`, `change_xy`, `release`):
1. If the active click is different from the current one, send `ABS_MT_SLOT`
2. Send `ABS_MT_TRACKING_ID`
3. Send `ABS_MT_PRESSURE`
4. Send `ABS_MT_TOUCH_MAJOR`
5. Repeatly send `ABS_MT_SLOT`
6. Send `ABS_MT_POSITION_X` and `ABS_MT_POSITION_Y`
7. Send `BTN_TOUCH`(if touch false: set touch true)
8. Send signal of finish transaction(aka sendevent 0 0 0)

It was touched without releasing, then we can either release or move the touch:

Move the touch:
1. If the active click is different from the current one, send `ABS_MT_SLOT`
2. Send `ABS_MT_POSITION_X` and `ABS_MT_POSITION_Y`
3. Send signal of finish transaction

Release:
1. If the active click is different from the current one, send `ABS_MT_SLOT`
2. Send `ABS_MT_TRACKING_ID` 4294967295, func `send_ev` use unsigned int, when it give max of uint32_t it transform to -1(Maybe, I don't shure)
3. Send signal of finish transaction
4. If it's last touch send `ABS_MT_SLOT` false, and another signal of finish transaction

I hope this is completely true and the bugs that will arise during development will not be related to the sequence of events.