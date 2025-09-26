#include <mutex>

constexpr double PI = 3.14159265358979323846;
constexpr double DEG_TO_RAD = PI / 180.0;
constexpr double ARCSEC_TO_RAD = DEG_TO_RAD / 3600.0; // Arcseconds to radians conversion factor
constexpr double ROBOT_RADIUS = 0.295; // Radius of the robot in meters, from centre to the wheels.
constexpr double ACT_RADIUS = 0.1705; // Radius of the actuators in meters, from centre of robot to the actuators.
constexpr double SIN30 = 0.5; // sin(30 degrees)
constexpr double COS30 = 0.8660254037844386; // cos(30 degrees) = sqrt(3)/2


// These are parameters read in from the toml file in main.cpp, and used in robotControllerServerFuncs.cpp
extern double g_roll_gain;
extern double g_pitch_gain;
extern double g_roll_target;
extern double g_pitch_target;
extern double g_x_acc_offset0;
extern double g_x_acc_offset1;
extern double g_x_acc_offset2;
extern double g_y_acc_offset0;
extern double g_y_acc_offset1;
extern double g_y_acc_offset2;
extern double g_z_acc_offset0;
extern double g_z_acc_offset1;
extern double g_z_acc_offset2;

// For the watchdog thread
extern int alive_counter;
extern int last_alive_counter;

// A structure to hold the velocities of the robo
struct Velocities
{
    double velocity; // An overall scale parameter.
    double x,y,z,roll,yaw,pitch,el;
};

struct Doubles 
{
    double x = 0; 
    double y = 0;
    double z = 0;
};

#define ST_IDLE 0
#define ST_WAITING 1
#define ST_MOVING 2
#define ST_TRACKING 3


// Status that can be returned through the server
struct Status {
	double roll, pitch; // Current roll and pitch angles from accelerometers.
    int delta_motors[7] = {0, 0, 0, 0, 0, 0, 0}; // Motor delta-steps
    int loop_status, loop_counter;
    int st_status = ST_IDLE; 
};

// A structure to hold the LEDs positions

// The header for robotThread.cpp, which contains the main robot loop
#define ROBOT_IDLE 1
#define ROBOT_TRANSLATE 2
#define ROBOT_RESONANCE 3
#define ROBOT_TRACK 4
#define ROBOT_DISCONNECT 5
extern int GLOBAL_SERVER_STATUS;
extern bool GLOBAL_STATUS_CHANGED;
extern int loop_counter;
extern Velocities g_vel, g_zero_vel;

extern Status g_status;
extern std::mutex GLOB_STATUS_LOCK;
void MoveSingleActuator(int actuator_index, double velocity);
void MoveSingleMotor(int motor_index, double velocity);

// Heading gain, plus gians and integral terms for yaw ane elevation control
extern double h_gain;
extern double g_ygain;
extern double g_egain;
extern double g_yint;
extern double g_eint;
extern double g_esum;
extern double g_ysum;
extern double g_heading;
extern double g_az;
extern double g_alt;
extern double g_az_off;
extern double g_alt_off;
extern double g_yaw_target;
extern double g_el_target;

// Log filename
extern std::string g_filename;

// Functions needed to be publicly accessible for the robot controller server
int robot_loop();
