/*
 * workout_inference.c
 *
 *  Created on: Nov 26, 2025
 *      Author: dafe2002
 */


/* workout_inference.c
 * Workout classification using X-CUBE-AI
 * Daphne Felt - ECEN 5613
 */

#include "uart.h"
#include "workout_inference.h"
#include <string.h>

// X-CUBE-AI generates these
#include "ai_datatypes_defines.h"
#include "ai_platform.h"
#include "network.h"
#include "network_data.h"

// the circular accel data buffer
static AccelBuffer accel_buf;
static uint32_t sample_count = 0;

static const char* workout_names[NUM_CLASSES] = {
    "WeightLift",
    "Walking",
    "Plank",
    "JumpingJacks",
    "Squats",
    "JumpRope"
};

// X-CUBE-AI vars
static ai_handle network = AI_HANDLE_NULL;
static ai_buffer *ai_input;
static ai_buffer *ai_output;

AI_ALIGNED(32)
static ai_u8 input_data[BUFFER_SIZE * NUM_FEATURES];

AI_ALIGNED(32)
static ai_u8 output_data[NUM_CLASSES];

AI_ALIGNED(32) ai_u8 activations[AI_NETWORK_DATA_ACTIVATIONS_SIZE];

// init ai network and buffers
bool Workout_Init(void) {
    ai_error err;

    memset(&accel_buf, 0, sizeof(AccelBuffer));
    sample_count = 0;

    // Create the AI network
    err = ai_network_create(&network, AI_NETWORK_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE) {
    	sendString("err on creating the ai network \r\n");
        return false;  // Creation failed
    }

    // init the network
    const ai_network_params params = {
		AI_NETWORK_DATA_WEIGHTS(ai_network_data_weights_get()),
		AI_NETWORK_DATA_ACTIVATIONS(activations)
    };

    if (!ai_network_init(network, &params)) {
    	sendString("err on initing the ai network \r\n");
    	err = ai_network_get_error(network);
    	char buf[50];
    	sprintf(buf, "Init failed: type=%d, code=%d\r\n", err.type, err.code);
    	sendStringGreen(buf);
        return false;  // Initialization failed
    }

    // pointers to i/o buffers
    ai_input = ai_network_inputs_get(network, NULL);
    ai_output = ai_network_outputs_get(network, NULL);

    // Set up both
    ai_input[0].data = AI_HANDLE_PTR(input_data);
    ai_output[0].data = AI_HANDLE_PTR(output_data);

    return true;
}

void Workout_AddSample(float x, float y, float z) {

	// need to quantize the input
	int16_t x_quant = (int16_t)((x / INPUT_QUANT_SCALE) + INPUT_QUANT_ZERO);
	int16_t y_quant = (int16_t)((y / INPUT_QUANT_SCALE) + INPUT_QUANT_ZERO);
	int16_t z_quant = (int16_t)((z / INPUT_QUANT_SCALE) + INPUT_QUANT_ZERO);

	// clap to uint8 range
	if (x_quant < 0) x_quant = 0;
	if (x_quant > 255) x_quant = 255;
	if (y_quant < 0) y_quant = 0;
	if (y_quant > 255) y_quant = 255;
	if (z_quant < 0) z_quant = 0;
	if (z_quant > 255) z_quant = 255;

	accel_buf.x[accel_buf.write_idx] = (uint8_t)x_quant;
	accel_buf.y[accel_buf.write_idx] = (uint8_t)y_quant;
	accel_buf.z[accel_buf.write_idx] = (uint8_t)z_quant;

    accel_buf.write_idx++;
    if (accel_buf.write_idx >= BUFFER_SIZE) {
        accel_buf.write_idx = 0;
        accel_buf.is_full = true;
    }

    sample_count++;
}

bool Workout_ShouldInfer(void) {
    // Only infer if buffer is full, otherwise we don't have enough data
    return accel_buf.is_full;
}

static void prepare_input_buffer(void) {
    uint16_t read_idx = accel_buf.write_idx; // start from oldest sample

    // buffer shape is [200, 3]: [x0, y0, z0, x1, y1, z1, etc]
    for (int t = 0; t < BUFFER_SIZE; t++) {
        input_data[t * NUM_FEATURES + 0] = accel_buf.x[read_idx];
        input_data[t * NUM_FEATURES + 1] = accel_buf.y[read_idx];
        input_data[t * NUM_FEATURES + 2] = accel_buf.z[read_idx];

        read_idx++;
        if (read_idx >= BUFFER_SIZE) {
            read_idx = 0;
        }
    }
}

bool Workout_RunInference(WorkoutResult *result) {
    if (network == AI_HANDLE_NULL || result == NULL) {
        return false;
    }

    // prep input data from circular buffer
    prepare_input_buffer();

    // do inference
    ai_i32 batch = ai_network_run(network, ai_input, ai_output);
    if (batch != 1) {
        return false; // fail
    }

    // Dequantize output: val = (quantized - zero_point) * scale
    float dequantized_output[NUM_CLASSES];
    ai_u8 *output_uint8 = (ai_u8*)ai_output[0].data;
    for (int i = 0; i < NUM_CLASSES; i++) {
    	dequantized_output[i] = ((float)output_uint8[i] - OUTPUT_QUANT_ZERO) * OUTPUT_QUANT_SCALE;
    }

    // Process output (softmax probabilities, want to find highest)
    int max_idx = 0;
    float max_val = output_data[0];

    for (int i = 1; i < NUM_CLASSES; i++) {
        if (output_data[i] > max_val) {
            max_val = output_data[i];
            max_idx = i;
        }
    }

    result->predicted_class = (WorkoutClass)max_idx;
    result->confidence = max_val / 10.0f;
    result->inference_time_ms = 0;
    result->timestamp = sample_count;
    for (int i = 0; i < NUM_CLASSES; i++) {
        result->class_scores[i] = output_data[i] / 10.0f;
    }

    return true;
}

// get the name
const char* Workout_GetName(WorkoutClass cls) {
    if (cls < NUM_CLASSES) {
        return workout_names[cls];
    }
    return "Unknown";
}

void Workout_ResetBuffer(void) {
    accel_buf.write_idx = 0;
    accel_buf.is_full = false;
}
