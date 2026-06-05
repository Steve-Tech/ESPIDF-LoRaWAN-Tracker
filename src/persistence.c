#include <esp_err.h>
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

static const char* TAG = "persistence";

nvs_handle_t persistence_handle;

void persistence_init(void) {
    ESP_LOGI(TAG, "Initializing ... ");

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void persistence_open(const char* namespace_name) {
    ESP_ERROR_CHECK(
        nvs_open(namespace_name, NVS_READWRITE, &persistence_handle));
}

void persistence_close(void) { nvs_close(persistence_handle); }

bool persistence_has_key(const char* key) {
    return nvs_find_key(persistence_handle, key, NULL) == ESP_OK;
}

void persistence_get_val(const char* key, void* out_value, size_t length) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        nvs_get_blob(persistence_handle, key, out_value, &length));
}

void persistence_set_val(const char* key, const void* value, size_t length) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(
        nvs_set_blob(persistence_handle, key, value, length));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(persistence_handle));
}
