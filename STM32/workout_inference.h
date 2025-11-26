/* workout_inference.h
 * Workout classification using X-CUBE-AI
 * Daphne Felt - ECEN 5613
 */

#ifndef WORKOUT_INFERENCE_H
#define WORKOUT_INFERENCE_H

#include <stdint.h>
#include <stdbool.h>

// Configuration
#define SAMPLE_RATE_HZ      100
#define WINDOW_SIZE_SEC     5
#define BUFFER_SIZE         (SAMPLE_RATE_HZ * WINDOW_SIZE_SEC)  // 500 samples
#define NUM_FEATURES        4   // x, y, z, heart_rate
#define NUM_CLASSES         6
#define HR_THRESHOLD        80

// Workout class labels (must match model training order)
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
    float x[BUFFER_SIZE];
    float y[BUFFER_SIZE];
    float z[BUFFER_SIZE];
    uint16_t write_idx;
    bool is_full;
} AccelBuffer;

// Inference result
typedef struct {
    WorkoutClass predicted_class;
    float confidence;          // 0-100%
    float class_scores[NUM_CLASSES];  // All class probabilities
    uint32_t inference_time_ms;
    uint32_t timestamp;
} WorkoutResult;

// Function prototypes
bool Workout_Init(void);
void Workout_AddSample(int16_t x, int16_t y, int16_t z);
bool Workout_ShouldInfer(void);
bool Workout_RunInference(WorkoutResult *result);
const char* Workout_GetName(WorkoutClass cls);

// Utility functions
uint8_t Workout_GetHR(void);  // Placeholder - replace with actual sensor
uint32_t Workout_GetBufferFillLevel(void);  // Returns 0-100%
bool Workout_IsBufferFull(void);
void Workout_ResetBuffer(void);

#endif // WORKOUT_INFERENCE_H
