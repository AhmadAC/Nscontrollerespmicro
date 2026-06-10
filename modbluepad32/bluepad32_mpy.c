#include "py/runtime.h"
#include "py/obj.h"

#include <stdio.h>
#include <string.h>

// Protect the MicroPython string generator from parsing complex external ESP-IDF/BTStack libraries
#ifndef NO_QSTR
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_err.h"

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

// 1. init
static void my_platform_init(int argc, const char** argv) {
    (void)argc;
    (void)argv;
}

// 2. on_init_complete
static void my_platform_on_init_complete(void) {
    // Start scanning for controllers once Bluetooth boots up
    uni_bt_enable_new_connections_unsafe(true);
}

// 3. on_device_discovered
static uni_error_t my_platform_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    (void)addr;
    (void)name;
    (void)cod;
    (void)rssi;
    return UNI_ERROR_SUCCESS;
}

// 4. on_device_connected
static void my_platform_on_device_connected(uni_hid_device_t* d) {
    (void)d;
    is_connected = true;
}

// 5. on_device_disconnected
static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    (void)d;
    is_connected = false;
    memset(&current_gamepad, 0, sizeof(current_gamepad));
}

// 6. on_device_ready
static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    (void)d;
    return UNI_ERROR_SUCCESS;
}

// 7. on_oob_event
static void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    (void)event;
    (void)data;
}

// 8. on_controller_data
static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    (void)d;
    // Update global gamepad state when data arrives
    if (ctl->klass == UNI_CONTROLLER_CLASS_GAMEPAD) {
        current_gamepad = ctl->gamepad;
    }
}

// 9. get_property - Configured to return valid property structures, silencing console warnings
static const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
    static const uni_property_t props[] = {
        // Global: Virtual Device enabled
        {
            .idx = UNI_PROPERTY_IDX_VIRTUAL_DEVICE_ENABLED,
            .name = UNI_PROPERTY_NAME_VIRTUAL_DEVICE_ENABLED,
            .type = UNI_PROPERTY_TYPE_BOOL,
            .default_value.boolean = false,
            .flags = 0,
        },
        // Unijoysticle thresholds (safely answers index 11 lookups)
        {
            .idx = UNI_PROPERTY_IDX_UNI_BB_FIRE_THRESHOLD,
            .name = "uni.bb.fire_threshold",
            .type = UNI_PROPERTY_TYPE_U8,
            .default_value.u8 = 32,
            .flags = 0,
        },
        // Unijoysticle thresholds (safely answers index 12 lookups)
        {
            .idx = UNI_PROPERTY_IDX_UNI_BB_MOVE_THRESHOLD,
            .name = "uni.bb.move_threshold",
            .type = UNI_PROPERTY_TYPE_U8,
            .default_value.u8 = 32,
            .flags = 0,
        }
    };
    for (size_t i = 0; i < sizeof(props) / sizeof(props[0]); i++) {
        if (props[i].idx == idx) {
            return &props[i];
        }
    }
    return NULL;
}

// Register callbacks using C99 designated initializers.
static struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {
        .name = "MicroPython",
        .init = my_platform_init,
        .on_init_complete = my_platform_on_init_complete,
        .on_device_discovered = my_platform_on_device_discovered,
        .on_device_connected = my_platform_on_device_connected,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_device_ready = my_platform_on_device_ready,
        .on_oob_event = my_platform_on_oob_event,
        .on_controller_data = my_platform_on_controller_data,
        .get_property = my_platform_get_property,
    };
    return &plat;
}

// Background task to run Bluetooth quietly
static void bluepad32_task(void *pvParameters) {
    (void)pvParameters;
    
    printf("Bluepad32: Task started. Initializing ESP32 Bluetooth hardware...\n");

    // 1. Initialize NVS (The Bluetooth controller requires NVS to be active)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    
    // 2. Configure BTstack for ESP32 VHCI Controller
    // CRITICAL FIX: This safely handles both hardware power-on AND internal BTStack linking.
    btstack_init();

    // 3. Register our fully implemented platform callbacks
    // IMPORTANT: Must be called AFTER btstack_init but BEFORE uni_init
    uni_platform_set_custom(get_my_platform());
    
    // 4. Initialize Bluepad32 (this configures the Gamepad parsing engine)
    uni_init(0, NULL);
    
    printf("Bluepad32: Engine initialized. Handing over to BTStack run loop...\n");

    // 5. Hand over this thread to the BTStack run loop (loops forever handling controller events)
    btstack_run_loop_execute();
    
    vTaskDelete(NULL);
}
#endif // NO_QSTR

// --- MicroPython Exposed Functions ---

// Python: bluepad32.start()
static mp_obj_t bp32_start(void) {
#ifndef NO_QSTR
    if (bp32_task_handle == NULL) {
        // Increase stack size to 16KB to prevent Stack Overflows
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