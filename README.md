# HypWorkCut
This application allows you to use a hot corner (bottom left of the screen) to activate a QT based popup allowing for mouse driven workspace switching. It factors in scaling that you are using and adjust the target area accordingly. This is useful if you do not have waybar exposed and don't wish to remove your hand from the mouse for using a keyboard combo. 

<img width="796" height="720" alt="image" src="https://github.com/user-attachments/assets/38d8866d-a01a-4010-8ec9-6b401c4426de" />

This application loads as a tray / dock icon which you can use to exit /quit the application or manually activate the popup. It is extremely lightweight as I have tried my best to keep its computational impact as negliable as possible.

You will need to add your own png to be used as the dock icon. It should be called HypWorkCut.png, and should be put in the same directory as the binary. 

QT6 is the only required dependency here. This has been tested on Omarchy. 

You can add this as an exec option in your hyprland.conf or manually trigger it via a desktop file. 

```
exec = HypWorkCut
```

I prefer the desktop file method, because if I am working without a mouse, and just using the keyboard laptop, etc. it is cumbersome to use a trackpad to activate workspace switching. Key combos are the way. 

You may find it useful to allow this app to be a pop-up instead of another window by adding this to your hyprland.conf file. This allows the popup to be positioned on the screen where you please. However, it is recommended that you have it to the left of the screen close to the trigger area. Moving the mouse cursor beyond the half way point of the display dismisses HypWorkCut. 

```
# HypWorkCut — Pop-up values
windowrulev2 = float,           class:^(HypWorkCut)$
windowrulev2 = move 20 90,      class:^(HypWorkCut)$
windowrulev2 = noborder,        class:^(HypWorkCut)$
windowrulev2 = rounding 20,     class:^(HypWorkCut)$
windowrulev2 = noinitialfocus,  class:^(HypWorkCut)$
windowrulev2 = size 260 560,    class:^(HypWorkCut)$
```

Under Hyprland 0.53 window rule syntax / formatting changes: 
```
# HypWorkCut — Pop-up values
windowrule {
  name = windowrule-1
  float = on
  move = (20) (90)
  border_size = 0
  rounding = 20
  no_initial_focus = on
  size = 260 560
  match:class = ^(HypWorkCut)$
}
```
