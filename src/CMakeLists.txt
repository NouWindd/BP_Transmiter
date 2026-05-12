idf_component_register(
    SRCS "main.c" "joystick.c" "display.c" "utils.c" "ui_controller.c" "imu.c" "transmitter.c"
    INCLUDE_DIRS "."
    REQUIRES    esp_driver_i2c  u8g2    u8g2-hal-esp-idf
)