/**
  ******************************************************************************
  * @file    network_data_params.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2025-11-29T14:25:30-0700
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#include "network_data_params.h"


/**  Activations Section  ****************************************************/
ai_handle g_network_activations_table[1 + 2] = {
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
  AI_HANDLE_PTR(NULL),
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
};




/**  Weights Section  ********************************************************/
AI_ALIGNED(32)
const ai_u64 s_network_weights_array_u64[19] = {
  0x817f43ce3e7f8a4cU, 0x13U, 0x0U, 0x118f7c811d137f00U,
  0x7f9c447f347fad81U, 0x71ce7f81896aaad9U, 0xfffffef7000000c2U, 0xffffff7300000032U,
  0x1f4ffffff62U, 0x13effffff8fU, 0x9899c1c26addbc7fU, 0x32ee231ff7d8aa81U,
  0x94734381f2bdf1afU, 0x2e158f461a9b1a7fU, 0xfa8f99a1816f0f98U, 0x5d10e391bd815190U,
  0xfffffd250000019eU, 0xfffffb47000002bfU, 0xffffff32000007a6U,
};


ai_handle g_network_weights_table[1 + 2] = {
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
  AI_HANDLE_PTR(s_network_weights_array_u64),
  AI_HANDLE_PTR(AI_MAGIC_MARKER),
};

