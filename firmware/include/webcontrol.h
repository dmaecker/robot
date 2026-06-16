#pragma once

// Start the HTTP server (serves the joystick page) and the /ws WebSocket.
void setupWebControl();

// Service the WebSocket and push the current command to the motors.
void webControlLoop();
