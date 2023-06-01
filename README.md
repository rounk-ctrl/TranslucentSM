# TranslucentSM
A lightweight utility that makes the Windows Start Menu translucent/transparent.

![HYYL5145](https://user-images.githubusercontent.com/70931017/235969892-d120bdc4-c9aa-431b-afcd-1aeb016babee.PNG)

This app utilizes XAML Diagnostics to inject a dll into a process and modifies the XAML.

# Settings
For now it uses registry to store the values. You can find them at ```HKEY_CURRENT_USER\SOFTWARE\TranslucentSM```. <br>
There are two features currently. All values must be between 1 and 9.
### `TintLuminosityOpacity` 
Controls the luminosity brush (some secondary layer ig). 

### `TintOpacity`
The main acrylic brush.
