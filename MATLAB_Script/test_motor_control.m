clear all; clc;

% Declare ports here
footPedal_port = "/dev/ttyACM0";
footPedal_baudrate = 9600;
footPedal = serialport(footPedal_port, footPedal_baudrate);

stm32_port = "/dev/ttyACM1";
stm32_baudrate = 115200;
stm32 = serialport(stm32_port, stm32_baudrate);

% Configure optional parameters for both ports
configureTerminator(footPedal, "CR/LF");
footPedal.Timeout = 5;
footPedal_connected = true;
disp("Foot Pedal (Adafruit Trinket M0) is connected to MATLAB...");

configureTerminator(stm32, "CR/LF");
stm32.Timeout = 5;
stm32_connected = true;
disp("STM32 F446RE connected to MATLAB. Ready to receive commands...");

% Send byte to STM32 MCU for controlling motor power state
while true
    if footPedal.NumBytesAvailable > 0 % Only read if there is data available
        if footPedal_connected == true
            footPedal_byte = read(footPedal,1,"uint8");
            disp(footPedal_byte);
            if stm32_connected == true
                if footPedal_byte == 1
                    motor_on = uint8(0x01);
                    write(stm32, motor_on, "uint8");
                    flush(stm32);
                elseif footPedal_byte == 0
                    motor_off = uint8(0x00);
                    write(stm32, motor_off, "uint8");              
                    flush(stm32);
                end
            end
        else
            disp("Foot pedal (Adafruit Trinket M0) not connected");
        end
    end
end
