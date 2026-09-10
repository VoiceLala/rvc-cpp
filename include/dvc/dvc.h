#ifndef DVC_DVC_H
#define DVC_DVC_H
#include <stddef.h>
#include <stdint.h>
#if defined(_WIN32)
# if defined(DVC_BUILDING_LIBRARY)
#  define DVC_API __declspec(dllexport)
# else
#  define DVC_API __declspec(dllimport)
# endif
#else
# define DVC_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define DVC_ABI_VERSION 1u
typedef struct dvc_context dvc_context;
typedef enum dvc_status {
    DVC_OK = 0, DVC_INVALID_ARGUMENT = 1, DVC_NOT_LOADED = 2,
    DVC_BUFFER_TOO_SMALL = 3, DVC_MODEL_ERROR = 4,
    DVC_RUNTIME_ERROR = 5, DVC_UNSUPPORTED = 6
} dvc_status;
typedef enum dvc_provider { DVC_CPU = 0, DVC_DIRECTML = 1 } dvc_provider;
typedef struct dvc_config {
    uint32_t struct_size;
    uint32_t abi_version;
    uint32_t sample_rate;
    uint32_t block_size;
    uint32_t context_samples;
    uint32_t crossfade_samples;
    uint32_t search_samples;
    uint32_t provider;
    uint32_t device_id;
    uint32_t threads;
} dvc_config;
typedef struct dvc_model_config {
    uint32_t struct_size;
    const char* voice_path;   /* UTF-8; consumed during load, not retained. */
    const char* content_path;
    const char* pitch_path;
    uint32_t sample_rate;
    uint32_t feature_dimension;
    uint32_t speaker_count;
} dvc_model_config;
typedef struct dvc_params {
    uint32_t struct_size;
    float pitch_semitones;
    float noise_scale;
    float pitch_threshold;
    uint32_t speaker_id;
    uint64_t seed;
} dvc_params;
DVC_API const char* dvc_version(void);
DVC_API void dvc_default_config(dvc_config* config);
DVC_API void dvc_default_model_config(dvc_model_config* config);
DVC_API void dvc_default_params(dvc_params* params);
/* Create sets *out to NULL on failure. Destroy(NULL) is valid. */
DVC_API dvc_status dvc_create(const dvc_config* config, dvc_context** out);
DVC_API void dvc_destroy(dvc_context* context);
/* Failed loads preserve the previous model and stream state. */
DVC_API dvc_status dvc_load_model(dvc_context* context, const dvc_model_config* model);
DVC_API dvc_status dvc_set_params(dvc_context* context, const dvc_params* params);
/* Resets streaming history and random state, preserving model and parameters. */
DVC_API dvc_status dvc_reset(dvc_context* context);
/* Mono float32, finite PCM. output_capacity and counts are in samples.
 * convert: independent clip, 320 or more 16 kHz-equivalent samples, <=30 sec.
 * process: exactly config.block_size samples; includes buffering delay.
 * Both write exactly input_count on success. Errors set written to zero;
 * output is untouched. Same-handle calls must be serialized by the caller.
 * In-place input/output is allowed. These allocate/run inference: NOT audio callbacks.
 */
DVC_API dvc_status dvc_convert(dvc_context* context, const float* input, size_t input_count, float* output, size_t output_capacity, size_t* written);
DVC_API dvc_status dvc_process(dvc_context* context, const float* input, size_t input_count, float* output, size_t output_capacity, size_t* written);
/* Per-thread last error, including create failures; valid until the next call
 * on that thread. Copy before switching threads or making another API call. */
DVC_API const char* dvc_last_error(void);
#ifdef __cplusplus
}
#endif
#endif
