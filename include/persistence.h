#pragma once

#include <nvs.h>

extern nvs_handle_t persistence_handle;

void persistence_init(void);

void persistence_open(const char* namespace_name);

void persistence_close(void);

bool persistence_has_key(const char* key);

void persistence_get_val(const char* key, void* out_value, size_t length);

void persistence_set_val(const char* key, const void* value, size_t length);
