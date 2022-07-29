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
    
    ssize_t ret;                            // I don't know why it's necessary

    struct input_event event;                // Event variable
    
    memset(&event, 0, sizeof(event));        // Clear event
    
    event.type = t;
    event.code = c;
    event.value = v;
    
    ret = write(f, &event, sizeof(event));  // And here, too, I don’t      the function simply 
                                            // know why we assign a        writes the event to 
                                            // result of the function to   the event file
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
    ABS (0003): ABS_MT_SLOT           : value 0, min 0, max 9,      fuzz 0, flat 0, resolution 0
                ABS_MT_TOUCH_MAJOR    : value 0, min 0, max 255,    fuzz 0, flat 0, resolution 0
                ABS_MT_POSITION_X     : value 0, min 0, max 1080,   fuzz 0, flat 0, resolution 0
                ABS_MT_POSITION_Y     : value 0, min 0, max 2400,   fuzz 0, flat 0, resolution 0
                ABS_MT_TRACKING_ID    : value 0, min 0, max 65535,  fuzz 0, flat 0, resolution 0
                ABS_MT_PRESSURE       : value 0, min 0, max 255,    fuzz 0, flat 0, resolution 0
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

# A little about chroot in arch

## Backstory

I was looking for a compiler for aarch64, I even found it (see something `gcc aarch64` in Google for your Linux distro), but apparently the root in android is a humiliated boy who can’t do anything, in particular, run binaries. I was tired of it, besides one busybox was clearly not enough for convenient debugging of the code and sending the binary via adb after the slightest changes in the code is not the best. I decided to do everything in beauty, so that it was divine!

## Preparation

For all of the below to work, you need to have Magisk installed on your device (you can look for information on [xda](https://www.xda-developers.com/ "xda") or [4pda](https://4pda.to/ "4pda")) and adb on your desktop.

## let's get to the point

1. We go to the [arch linux arm site](https://archlinuxarm.org/about/downloads "Arch ARM downloads"), download the necessary package for your device, to find out what you need through adb or a terminal emulator, write "uname -m" this will be the architecture of your device
2. It is highly desirable to have an SD card, because when you try to unpack the archive into internal storage, all links and rights will break. At least this is what happens on my Redmi Note 9 Pro with AOSP from the 9S version
3. When connecting the card, the android offers to do something with the card, we politely refuse and umount it
4. Next, look for it in block devices, for further examples I will use `/dev/block/mmcblk0p1`
5. We format the card (if the card is already with the desired file system, change it to another, and then back) and mount it
```sh
mkfs.ext4 /dev/block/mmcblk0p1
mkdir /mnt/arch
mount /dev/block/mmcblk0p1 /mnt/arch
```
6. Unpack the downloaded archive to the root of the SD card
7. Next, we link the `dev`, `proc` and `sys` directories from Android to our future chroot:
```sh
mount --bind /dev /mnt/arch/dev
mount --bind /proc /mnt/arch/proc
mount --bind /sys /mnt/arch/sys
```
8. And we begin to mock arch by copying the system and apex folders for the android binaries to work correctly:
```sh
cp -r /apex/ /mnt/arch/
cp -r /system /mnt/arch/
```
9. Copy getevent if you want to quickly watch events from the chroot. If you don't need the previous one and you can skip this step:
```sh
cp /sbin/getevent /mnt/arch/usr/bin/
```
10. Next, go to chroot and enjoy the fact that you have almost a full-fledged Linux in your pocket:
```sh
chroot /mnt/arch /sbin/su - root
```
11. If you're having trouble resolving hostnames, add the correct DNS to resolv.conf. There may also be problems with access to this file, for example, I had it. It's okay, just delete the file and create a new one:
```sh
echo "nameserver 8.8.8.8" >> /etc/resolv.conf
# If it gives an error
rm /etc/resolv.conf
echo "nameserver 8.8.8.8" >> /etc/resolv.conf # The file will automatically be created
```
12. In general, it's all, it remains to install gcc, git and software convenient for you:
```sh
pacman -Suy gcc git
```
⋅⋅⋅The Arch ARM repositories have almost all the programs we are used to. If there is no program, you can always compile from source, with make it will not be difficult.

When rebooting the device, remount the card along with dev, proc and sys.



