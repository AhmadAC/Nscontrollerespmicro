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

// Global variables to hold gamepad state
static uni_gamepad_t current_gamepad = {0};
static bool is_connected = false;
static TaskHandle_t bp32_task_handle = NULL;

// --- Bluepad32 Platform Callbacks ---
static void my_platform_init(int argc, const char** argv) {
    (void)argc;
    (void)argv;
}

static void my_platform_on_init_complete(void) {
    // Start scanning for controllers once Bluetooth boots up
    uni_bt_enable_new_connections_unsafe(true);
}

static void my_platform_on_device_discovered(uni_hid_device_t* d) {
    (void)d;
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

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    (void)event;
    (void)data;
}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    (void)d;
    // Update global gamepad state when data arrives
    if (ctl->klass == UNI_CONTROLLER_CLASS_GAMEPAD) {
        current_gamepad = ctl->gamepad;
    }
}

static int32_t my_platform_get_property(uni_platform_property_t key) {
    (void)key;
    return -1;
}

// Register callbacks
static struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {0};
    plat.name = "MicroPython";
    plat.init = my_platform_init;
    plat.on_init_complete = my_platform_on_init_complete;
    plat.on_device_discovered = my_platform_on_device_discovered;
    plat.on_device_connected = my_platform_on_device_connected;
    plat.on_device_disconnected = my_platform_on_device_disconnected;
    plat.on_device_ready = my_platform_on_device_ready;
    plat.on_oob_event = my_platform_on_oob_event;
    plat.on_controller_data = my_platform_on_controller_data;
    plat.get_property = my_platform_get_property;
    return &plat;
}

// Background task to run Bluetooth quietly
static void bluepad32_task(void *pvParameters) {
    (void)pvParameters;
    btstack_init();
    uni_platform_set_custom(get_my_platform());
    uni_init(0, NULL);
    btstack_run_loop_execute(); // Loops forever
    vTaskDelete(NULL);
}
#else
// Provide dummy types for the QSTR parser so it doesn't fail when external headers are hidden
typedef void* TaskHandle_t;
static TaskHandle_t bp32_task_handle = NULL;
static bool is_connected = false;
typedef struct { int dpad, buttons, axis_x, axis_y, axis_rx, axis_ry; } uni_gamepad_t;
static uni_gamepad_t current_gamepad = {0};
#endif // NO_QSTR

// --- MicroPython Exposed Functions ---

// Python: bluepad32.start()
static mp_obj_t bp32_start(void) {
#ifndef NO_QSTR
    if (bp32_task_handle == NULL) {
        xTaskCreate(bluepad32_task, "bluepad32_task", 4096, NULL, 5, &bp32_task_handle);
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