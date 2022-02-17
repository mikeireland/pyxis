#include "RobotDriver.h"

using namespace Control;

bool closed_loop_enable_flag = false;

int main() {
	//Necessary global timing measures
	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;
	using std::chrono::microseconds;

	auto time_point_start = high_resolution_clock::now();
	auto time_point_current = high_resolution_clock::now();
	auto last_stabiliser_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
	auto last_leveller_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
	auto last_leveller_subtimepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
	auto last_navigator_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();
	auto global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

	RobotDriver driver;

	//ID the teensy
	driver.teensy_port.Request(ID);
	driver.teensy_port.PacketManager();
	printf("Device ID: %u\n",driver.teensy_port.device_id_);
	printf("Device Firmware Version: %u\n",driver.teensy_port.device_firmware_v_);

	while((driver.teensy_port.device_id_ != 128) && (driver.teensy_port.device_file_index_ < 16)){
		//ID the teensy
		driver.teensy_port.ClosePort();
		driver.teensy_port.teensy_ = -1;
		driver.teensy_port.OpenPort();
		driver.teensy_port.Request(ID);
		driver.teensy_port.PacketManager();
		printf("Device ID: %u\n",driver.teensy_port.device_id_);
		printf("Device Firmware Version: %u\n",driver.teensy_port.device_firmware_v_);
	}

	//We allow two attempts for convenience
	driver.teensy_port.device_file_index_ = 0;

	while((driver.teensy_port.device_id_ != 128) && (driver.teensy_port.device_file_index_ < 16)){
		//ID the teensy
		driver.teensy_port.ClosePort();
		driver.teensy_port.teensy_ = -1;
		driver.teensy_port.OpenPort();
		driver.teensy_port.Request(ID);
		driver.teensy_port.PacketManager();
		printf("Device ID: %u\n",driver.teensy_port.device_id_);
		printf("Device Firmware Version: %u\n",driver.teensy_port.device_firmware_v_);
	}
	
	//driver.StabiliserTest();
	//driver.StabiliserSetup();
	//driver.EngageLeveller();
	//driver.EngageNavigator();

	//Open loop tests
	//driver.SinusoidalSweepLog(10000,100000,1.0046,1,3,'y',0.002);
	//driver.SinusoidalAmplitudeSweepLinear(1000,0,0.015,100,5,'y');

    //driver.RaiseAndLowerTest();
    //driver.LinearSweepTest();
	//driver.MeasureOrientationMeasurementNoise();
	//driver.NavigatorTest();
	//driver.StabiliserTest();

	//We rest the start clock and start the closed loop operation
	time_point_start = high_resolution_clock::now();
	while(closed_loop_enable_flag) {
		//Boolean to store if we have done anything on this loop and wait a little bit if we haven't
		bool controller_active = false;


		time_point_current = high_resolution_clock::now();
		global_timepoint = duration_cast<microseconds>(time_point_current-time_point_start).count();

		if(global_timepoint-last_stabiliser_timepoint > 1000) {	
			if(driver.stabiliser.enable_flag_) {
				printf("%ld\n",global_timepoint-last_stabiliser_timepoint);
				controller_active = true;
				driver.StabiliserLoop();
			}
			last_stabiliser_timepoint = global_timepoint;
		}

		if(global_timepoint-last_leveller_subtimepoint > 1000) {
			if(driver.leveller.enable_flag_) {
				if((global_timepoint-last_leveller_timepoint) > 10000) {
					last_leveller_timepoint = global_timepoint;
					controller_active = true;
					driver.LevellerLoop();	
				}
				last_leveller_subtimepoint = global_timepoint;
				controller_active = true;
				driver.LevellerSubLoop();	
			}
		}
		
		if((global_timepoint-last_navigator_timepoint) > 10000) {
			last_navigator_timepoint = global_timepoint;
			if(driver.navigator.enable_flag_) {
				controller_active = true;
				driver.NavigatorLoop();
			}
		}

		//If the leveller is only enables for a short run after 20 seconds we disable it
		//This is for the stabiliser runs
		if(driver.short_level_flag2_){
			if(global_timepoint > 10000000){
				driver.SetNewTargetAngle(driver.leveller.pitch_target_cache_,driver.leveller.roll_target_cache_);
				driver.RequestResetStepCount();
				driver.short_level_flag2_ = false;
			}
		}

		if(driver.short_level_flag_){
			if(global_timepoint > 20000000) {
				controller_active = true;
				driver.EngageStabiliser();

			}
		}

		if (!controller_active) {
			usleep(10);
		}

		driver.teensy_port.PacketManager();
	}

	driver.teensy_port.ClosePort();

	return -1;
}
