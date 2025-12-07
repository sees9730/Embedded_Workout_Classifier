To run this code on an STM32:
- Download the project in the STM32 folder, named WorkoutInference
- Download STM32CubeIDE 1.18.1, and load in the project
- Wire up a USB UART serial port module on PA2 and PA3 of the STM32 (UART2)
- Build and Run the project with the STM32 and UART serial port plugged into your computer
- Open Putty, and connect to the COM your UART serial port is connected to, with a Baud Rate of 115200.
- Orient the embedded system around your wrist and work out
- The workout type and probablities should print out on the Putty Terminal!
