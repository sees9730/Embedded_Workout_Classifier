/* workout_inference.c
 * Workout classification using X-CUBE-AI
 * Daphne Felt - ECEN 5613
 */

#include "workout_inference.h"
#include <string.h>

// X-CUBE-AI generated headers (will be available after code generation)
#include "ai_datatypes_defines.h"
#include "ai_platform.h"
#include "network.h"
#include "network_data.h"

// Circular buffer for accelerometer data
static AccelBuffer accel_buf;
static uint32_t sample_count = 0;

// Workout class names (must match model training order)
static const char* workout_names[NUM_CLASSES] = {
    "WeightLift",
    "Walking", 
    "Plank",
    "JumpingJacks",
    "Squats",
    "JumpRope"
};

// X-CUBE-AI variables
static ai_handle network = AI_HANDLE_NULL;
static ai_buffer *ai_input;
static ai_buffer *ai_output;

// Buffers for input/output data
// X-CUBE-AI expects aligned buffers
AI_ALIGNED(4)
static ai_float input_data[BUFFER_SIZE * NUM_FEATURES];

AI_ALIGNED(4)
static ai_float output_data[NUM_CLASSES];

/* Initialize X-CUBE-AI network and buffers */
bool Workout_Init(void) {
    ai_error err;
    
    // Initialize buffers
    memset(&accel_buf, 0, sizeof(AccelBuffer));
    sample_count = 0;
    
    // Create the AI network
    err = ai_network_create(&network, AI_NETWORK_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE) {
        return false;  // Creation failed
    }
    
    // Initialize the network
    const ai_network_params params = {
        AI_NETWORK_DATA_WEIGHTS(ai_network_data_weights_get()),
        AI_NETWORK_DATA_ACTIVATIONS(NULL)  // Use static activations
    };
    
    if (!ai_network_init(network, &params)) {
        return false;  // Initialization failed
    }
    
    // Get pointers to input/output buffers
    ai_input = ai_network_inputs_get(network, NULL);
    ai_output = ai_network_outputs_get(network, NULL);
    
    // Set up input buffer
    ai_input[0].data = AI_HANDLE_PTR(input_data);
    
    // Set up output buffer
    ai_output[0].data = AI_HANDLE_PTR(output_data);
    
    return true;
}

/* Add new accelerometer sample to circular buffer */
void Workout_AddSample(int16_t x, int16_t y, int16_t z) {
    // Convert raw values to g (assuming ±8g full scale, 12-bit resolution)
    // 12-bit signed: -2048 to +2047
    accel_buf.x[accel_buf.write_idx] = (x * 8.0f) / 2048.0f;
    accel_buf.y[accel_buf.write_idx] = (y * 8.0f) / 2048.0f;
    accel_buf.z[accel_buf.write_idx] = (z * 8.0f) / 2048.0f;
    
    accel_buf.write_idx++;
    if (accel_buf.write_idx >= BUFFER_SIZE) {
        accel_buf.write_idx = 0;
        accel_buf.is_full = true;
    }
    
    sample_count++;
}

/* Check if we should run inference */
bool Workout_ShouldInfer(void) {
    // Only infer if buffer is full and HR > threshold
    if (!accel_buf.is_full) {
        return false;
    }
    
    uint8_t hr = Workout_GetHR();
    return (hr > HR_THRESHOLD);
}

/* Prepare input data from circular buffer */
static void prepare_input_buffer(void) {
    uint16_t read_idx = accel_buf.write_idx;  // Start from oldest sample
    float current_hr = (float)Workout_GetHR();
    
    // Fill input buffer: shape is [500, 4]
    // Order: [x0, y0, z0, hr0, x1, y1, z1, hr1, ...]
    for (int t = 0; t < BUFFER_SIZE; t++) {
        input_data[t * NUM_FEATURES + 0] = accel_buf.x[read_idx];
        input_data[t * NUM_FEATURES + 1] = accel_buf.y[read_idx];
        input_data[t * NUM_FEATURES + 2] = accel_buf.z[read_idx];
        input_data[t * NUM_FEATURES + 3] = current_hr;
        
        read_idx++;
        if (read_idx >= BUFFER_SIZE) {
            read_idx = 0;
        }
    }
}

/* Run inference using X-CUBE-AI */
bool Workout_RunInference(WorkoutResult *result) {
    if (network == AI_HANDLE_NULL || result == NULL) {
        return false;
    }
    
    uint32_t start_time = HAL_GetTick();
    
    // Prepare input data from circular buffer
    prepare_input_buffer();
    
    // Run inference
    ai_i32 batch = ai_network_run(network, ai_input, ai_output);
    if (batch != 1) {
        return false;  // Inference failed
    }
    
    uint32_t end_time = HAL_GetTick();
    
    // Process output (softmax probabilities)
    // Find class with highest probability
    int max_idx = 0;
    float max_val = output_data[0];
    
    for (int i = 1; i < NUM_CLASSES; i++) {
        if (output_data[i] > max_val) {
            max_val = output_data[i];
            max_idx = i;
        }
    }
    
    // Fill result structure
    result->predicted_class = (WorkoutClass)max_idx;
    result->confidence = max_val * 100.0f;  // Convert to percentage
    result->inference_time_ms = end_time - start_time;
    result->timestamp = sample_count;
    
    // Copy all class scores
    for (int i = 0; i < NUM_CLASSES; i++) {
        result->class_scores[i] = output_data[i] * 100.0f;
    }
    
    return true;
}

/* Get workout name from class */
const char* Workout_GetName(WorkoutClass cls) {
    if (cls < NUM_CLASSES) {
        return workout_names[cls];
    }
    return "Unknown";
}

/* Placeholder HR function - replace with actual sensor */
uint8_t Workout_GetHR(void) {
    // TODO: Replace with actual heart rate sensor reading
    // For now, simulate based on sample count
    if (sample_count > 500 && sample_count < 2000) {
        return 95;  // Simulated elevated HR during exercise
    }
    return 72;  // Resting HR
}

/* Get buffer fill level as percentage */
uint32_t Workout_GetBufferFillLevel(void) {
    if (accel_buf.is_full) {
        return 100;
    }
    return (accel_buf.write_idx * 100) / BUFFER_SIZE;
}

/* Check if buffer is full */
bool Workout_IsBufferFull(void) {
    return accel_buf.is_full;
}

/* Reset circular buffer */
void Workout_ResetBuffer(void) {
    accel_buf.write_idx = 0;
    accel_buf.is_full = false;
}
