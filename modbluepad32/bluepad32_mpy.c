#include "py/runtime.h"

// Example function: bluepad32.info()
STATIC mp_obj_t bluepad32_info(void) {
    return mp_obj_new_str("Bluepad32 MicroPython Module loaded!", 36);
}
// Define the function object (0 arguments)
STATIC MP_DEFINE_CONST_FUN_OBJ_0(bluepad32_info_obj, bluepad32_info);

// Map the functions to the module dictionary
STATIC const mp_rom_map_elem_t bluepad32_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_bluepad32) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&bluepad32_info_obj) },
};
STATIC MP_DEFINE_CONST_DICT(bluepad32_module_globals, bluepad32_module_globals_table);

// Define the module object
const mp_obj_module_t bluepad32_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&bluepad32_module_globals,
};

// Register the module so "import bluepad32" works
MP_REGISTER_MODULE(MP_QSTR_bluepad32, bluepad32_user_cmodule);
