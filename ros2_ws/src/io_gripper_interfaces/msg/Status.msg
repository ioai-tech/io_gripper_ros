std_msgs/Header header

uint8 DISCONNECTED=0
uint8 CONNECTED=1
uint8 READY=2
uint8 MOVING=3
uint8 FAULT=4
uint8 ESTOP=5

uint8 driver_state

uint8 servo_id

uint16 position_raw
float32 position_rad

uint16 velocity_raw
float32 velocity_rad_s

bool has_load
uint16 load_raw

float32 voltage_v
float32 temperature_c

bool communication_ok

string message