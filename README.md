# TranslucentSM
A lightweight utility that makes the Windows Start Menu translucent/transparent.<br>
This app utilizes XAML Diagnostics to inject a dll into a process and modifies the XAML.

# Settings
For now it uses registry to store the values. You can find them at ```HKEY_CURRENT_USER\SOFTWARE\TranslucentSM```. <br>
There are two features currently. All values must be between 1 and 9.<br>
**After you change the settings, you must terminate StartMenuExperienceHost.exe and relaunch the app.**
### `TintLuminosityOpacity` 
Controls the luminosity brush (some secondary layer ig). 

### `TintOpacity`
The main acrylic brush.

# Screenshots
![image](https://github.com/rounk-ctrl/TranslucentSM/assets/70931017/4a569f8c-f66a-45d3-9841-07d4a39a5063)

![image](https://github.com/rounk-ctrl/TranslucentSM/assets/70931017/5c81d8d3-880a-48bf-a863-4fb3ccf96b49)
