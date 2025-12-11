# Example ESP32 app using rm67162 OLED display

## What is this and why?

* Example app built against esp-idf, (_NOT_ Arduino)
* Uses esp-idf's `esp_lcd` component to drive the display
* Includes driver for rm67162 display controller chip, that is missing
  in the upstream `esp_lcd` (as of this writing).
* This driver only works when controller is connected over 3-Wire SPI,
  and does not use DC pin ("16 bit transfer mode"), and tested only
  against LilyGO T-Display-S3-AMOLED dev module.
  * @electronicsguy [reports](https://github.com/crosser/lvgl_esp_lcd/issues/1)
    that the this code works with Waveshare ESP32-S3-AMOLED-1.91 without change.

Why?

LilyGO provides a driver for this controller, which is

* Arduino-specific
* Uses polling SPI transactions (no dual buffer support possible)

Arduino infrastructure is not well suited for my use case, and I wanted
to be able to build my application using `esp-idf` instead. But I did not
find a driver for rm67162 that would be usable under esp-idf, so I ported
the code taken from LilyGO examples.

LVGL is used for the drawing library, drawing code is taken from one of
their examples.

This example app shows how to bind `esp_lcd`-managed display to LVGL in
such a way that asyncronous transactions and dual buffering can be used.
(Which is honestly quite an arduous job.)

## References

* T-Display-S3-AMOLED [https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED](https://github.com/Xinyuan-LilyGO/T-Display-S3-AMOLED)
* ESP-IDF [https://docs.espressif.com/projects/esp-idf/en/](https://docs.espressif.com/projects/esp-idf/en/)
* ESP-IDF `esp_lcd` component [https://github.com/espressif/esp-idf/tree/master/components/esp\_lcd](https://github.com/espressif/esp-idf/tree/master/components/esp_lcd)
* LVGL [https://docs.lvgl.io/](https://docs.lvgl.io/)

