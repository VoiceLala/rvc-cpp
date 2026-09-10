#include "dvc/dvc.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s (%s)\n", __LINE__, #x, dvc_last_error()); return 1; } } while (0)
int main(void) {
    dvc_config config;
    dvc_context* context = NULL;
    dvc_params params;
    float input[4800] = {0}, output[4800] = {123.f};
    size_t written = 99;
    dvc_default_config(&config);
    CHECK(dvc_create(NULL, &context) == DVC_INVALID_ARGUMENT && !context);
    CHECK(strlen(dvc_last_error()) > 0);
    config.abi_version = 99;
    CHECK(dvc_create(&config, &context) == DVC_INVALID_ARGUMENT && !context);
    dvc_default_config(&config);
    config.crossfade_samples = config.block_size + 1;
    CHECK(dvc_create(&config, &context) == DVC_INVALID_ARGUMENT);
    dvc_default_config(&config);
    CHECK(dvc_create(&config, &context) == DVC_OK && context);
    CHECK(dvc_process(context, input, 4800, output, 4800, &written) == DVC_NOT_LOADED);
    CHECK(written == 0 && output[0] == 123.f);
    CHECK(dvc_convert(context, input, 4800, output, 1, &written) == DVC_BUFFER_TOO_SMALL);
    CHECK(dvc_convert(context, input, 0, output, 4800, &written) == DVC_INVALID_ARGUMENT);
    dvc_default_params(&params);
    params.pitch_semitones = 100.f;
    CHECK(dvc_set_params(context, &params) == DVC_INVALID_ARGUMENT);
    CHECK(dvc_reset(context) == DVC_OK);
    CHECK(dvc_load_model(context, NULL) == DVC_INVALID_ARGUMENT);
    dvc_destroy(context);
    dvc_destroy(NULL);
    puts("C ABI and argument/error tests passed");
    return 0;
}
