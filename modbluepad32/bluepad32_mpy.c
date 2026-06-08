#include "py/runtime.h"
#include "py/obj.h"

#include <stdio.h>
#include <string.h>

// Protect the MicroPython string generator from parsing complex external ESP-IDF/BTStack libraries
#ifndef NO_QSTR
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Bluepad32 headers
#include "uni.h"
#include "platform/uni_platform.h"
#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"
#endif

// Provide dummy types for the QSTR parser since we hid the actual includes
#ifdef NO_QSTR
typedef struct { int dpad, buttons, axis_x, axis_y, axis_rx, axis_ry; } uni_gamepad_t;
typedef void uni_hid_device_t;
typedef struct { int klass; uni_gamepad_t gamepad; } uni_controller_t;
typedef int uni_error_t;
struct uni_platform { const char* name; };
#define UNI_ERROR_SUCCESS 0
#define UNI_CONTROLLER_CLASS_GAMEPAD 1
typedef void* TaskHandle_t;
#endif

// Global variables to hold gamepad state
static uni_gamepad_t current_gamepad = {0};
static bool is_connected = false;
static TaskHandle_t bp32_task_handle = NULL;

#ifndef NO_QSTR
// --- Bluepad32 Platform Callbacks ---
static void my_platform_on_init_complete(void) {
    // Start scanning for controllers once Bluetooth boots up
    uni_bt_enable_new_connections_unsafe(true);
}

static void my_platform_on_device_connected(uni_hid_device_t* d) {
    (void)d;
    is_connected = true;
}

static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    (void)d;
    is_connected = false;
    memset(&current_gamepad, 0, sizeof(current_gamepad));
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    (void)d;
    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    (void)d;
    // Update global gamepad state when data arrives
    if (ctl->klass == UNI_CONTROLLER_CLASS_GAMEPAD) {
        current_gamepad = ctl->gamepad;
    }
}

// Global struct to guarantee memory persistence across the entire lifecycle
static struct uni_platform my_custom_platform = {0};

// Register callbacks
static struct uni_platform* get_my_platform(void) {
    my_custom_platform.name = "MicroPython";
    my_custom_platform.on_init_complete = my_platform_on_init_complete;
    my_custom_platform.on_device_connected = my_platform_on_device_connected;
    my_custom_platform.on_device_disconnected = my_platform_on_device_disconnected;
    my_custom_platform.on_device_ready = my_platform_on_device_ready;
    my_custom_platform.on_controller_data = my_platform_on_controller_data;
    return &my_custom_platform;
}

// Background task to run Bluetooth quietly
static void bluepad32_task(void *pvParameters) {
    (void)pvParameters;
    
    // Register our custom platform callbacks
    uni_platform_set_custom(get_my_platform());
    
    // Initialize Bluepad32 (this configures the ESP32 BT hardware and calls btstack_init)
    uni_init(0, NULL);
    
    // Hand over this thread to the BTStack run loop (loops forever)
    btstack_run_loop_execute();
    
    vTaskDelete(NULL);
}
#endif // NO_QSTR

// --- MicroPython Exposed Functions ---

// Python: bluepad32.start()
static mp_obj_t bp32_start(void) {
#ifndef NO_QSTR
    if (bp32_task_handle == NULL) {
        // Increase stack size to 16KB to prevent Stack Overflows (which overwrite the return address causing PC=0x00000000 crashes)
        // Pin BTStack to Core 0 (PRO_CPU) to isolate it from MicroPython running on Core 1
        xTaskCreatePinnedToCore(bluepad32_task, "bluepad32_task", 16384, NULL, 5, &bp32_task_handle, 0);
        return mp_const_true;
    }
#endif
    return mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_0(bp32_start_obj, bp32_start);

// Python: bluepad32.get_gamepad()
static mp_obj_t bp32_get_gamepad(void) {
    if (!is_connected) {
        return mp_const_none; // Return None if no controller is paired
    }
    mp_obj_t tuple[6];
    tuple[0] = mp_obj_new_int(current_gamepad.dpad);
    tuple[1] = mp_obj_new_int(current_gamepad.buttons);
    tuple[2] = mp_obj_new_int(current_gamepad.axis_x);
    tuple[3] = mp_obj_new_int(current_gamepad.axis_y);
    tuple[4] = mp_obj_new_int(current_gamepad.axis_rx);
    tuple[5] = mp_obj_new_int(current_gamepad.axis_ry);
    return mp_obj_new_tuple(6, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_0(bp32_get_gamepad_obj, bp32_get_gamepad);

// Register Module Dictionary
static const mp_rom_map_elem_t bluepad32_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_bluepad32) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&bp32_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_gamepad), MP_ROM_PTR(&bp32_get_gamepad_obj) },
};
static MP_DEFINE_CONST_DICT(bluepad32_module_globals, bluepad32_module_globals_table);

const mp_obj_module_t bluepad32_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&bluepad32_module_globals,
};
MP_REGISTER_MODULE(MP_QSTR_bluepad32, bluepad32_user_cmodule);