
/* workout_inference.h
 * Workout classification using X-CUBE-AI
 * Daphne Felt - ECEN 5613
 */

#ifndef WORKOUT_INFERENCE_H
#define WORKOUT_INFERENCE_H

#include <stdint.h>
#include <stdbool.h>

#define SAMPLE_RATE_HZ      100
#define WINDOW_SIZE_SEC     2
#define BUFFER_SIZE         (SAMPLE_RATE_HZ * WINDOW_SIZE_SEC)  // 200 samples
#define NUM_FEATURES        3   // x, y, z
#define NUM_CLASSES         6

// Quantization params from TFLite model, we quantized to uint8
#define INPUT_QUANT_SCALE  0.070879f
#define INPUT_QUANT_ZERO   130
#define OUTPUT_QUANT_SCALE 0.093426f
#define OUTPUT_QUANT_ZERO  101

// Workout class labels, based on the training order
typedef enum {
    WORKOUT_WEIGHTLIFT = 0,
    WORKOUT_WALKING = 1,
    WORKOUT_PLANK = 2,
    WORKOUT_JUMPING_JACKS = 3,
    WORKOUT_SQUATS = 4,
    WORKOUT_JUMP_ROPE = 5
} WorkoutClass;

// Circular buffer for accelerometer data
typedef struct {
    uint8_t x[BUFFER_SIZE];
    uint8_t y[BUFFER_SIZE];
    uint8_t z[BUFFER_SIZE];
    uint16_t write_idx;
    bool is_full;
} AccelBuffer;

// result struct for inference
typedef struct {
    WorkoutClass predicted_class;
    float confidence;
    float class_scores[NUM_CLASSES];
    uint32_t inference_time_ms;
    uint32_t timestamp;
} WorkoutResult;

bool Workout_Init(void);
void Workout_AddSample(float x, float y, float z);
bool Workout_ShouldInfer(void);
bool Workout_RunInference(WorkoutResult *result);
const char* Workout_GetName(WorkoutClass cls);
void Workout_ResetBuffer(void);

#endif
