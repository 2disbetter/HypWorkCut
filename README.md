# HypWorkCut
This application allows you to use a hot corner (bottom left of the screen) to activate a QT based popup allowing for mouse driven workspace switching. This is useful if you do not have waybar exposed and don't wish to remove your hand from the mouse for using a keyboard combo. 

<img width="796" height="720" alt="image" src="https://github.com/user-attachments/assets/38d8866d-a01a-4010-8ec9-6b401c4426de" />

This application loads as a tray / dock icon which you can use to exit /quit the application or manually activate the popup. It is extremely lightweight as I have tried my best to keep its computational impact as negliable as possible.

You will need to add your own png to be used as the dock icon. It should be called HypWorkCut.png, and should be put in the same directory as the binary. 

QT6 is the only required dependency here. This has been tested on Omarchy. 

You can add this as an exec option in your hyprland.conf or manually trigger it via a desktop file. 

```
exec = HypWorkCut
```

I prefer the desktop file method, because if I am working without a mouse, and just using the keyboard laptop, etc. it is cumbersome to use a trackpad to activate workspace switching. Key combos are the way. 
