# ESP32 app that fetches some data over http and displays on rm67162 OLED display

## What is this?

I wanted to a list of last few files that were added to a certain place
to be visible at a glance, without opening any apps or enterint any commands.
So, on the remote server, I generate a text document with the file name
and its timestamp in .csv format, accessible over http[s]. ESP32 app
periodically connects to the WiFi, fetches this document, and displays
it (having parsed .csv and removed some parts of the file names: display
is too small!).

This is a "useful" application of the `rm67162` driver that I have hacked
[here](https://git.average.org/cgit/lvgl_esp_lcd.git/).

Run `idf.py mkconfig` to enter your WiFi SSID and WPA password (only WPAn
is supported), url and timezone.

## References

* T-Display-S3-AMOLED [https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED](https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED)
* ESP-IDF [https://docs.espressif.com/projects/esp-idf/en/](https://docs.espressif.com/projects/esp-idf/en/)
* LVGL [https://docs.lvgl.io/](https://docs.lvgl.io/)

