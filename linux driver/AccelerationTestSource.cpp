#include "RobotDriver.h"

int main() {
    Control::RobotDriver driver;
    Control::AccelerationTester tester;

    //Necessary global timing measures
	using std::chrono::high_resolution_clock;
	using std::chrono::duration_cast;
	using std::chrono::duration;
	using std::chrono::microseconds;

	auto time_start = high_resolution_clock::now();
	auto time_current = high_resolution_clock::now();
	auto current_timepoint = duration_cast<microseconds>(time_current-time_start).count();
    auto last_timepoint = duration_cast<microseconds>(time_current-time_start).count();

    bool run_test_flag = true;


    for(int i = 0; i < 1000; i++) {
		driver.RequestAccelerations();
		driver.teensy_port.PacketManager();

		tester.acc0_ground_state_.x += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer0_in_.x[0],driver.teensy_port.accelerometer0_in_.x[1]);
		tester.acc0_ground_state_.y += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer0_in_.y[0],driver.teensy_port.accelerometer0_in_.y[1]);
		tester.acc0_ground_state_.z += -0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer0_in_.z[0],driver.teensy_port.accelerometer0_in_.z[1]);
		tester.acc1_ground_state_.x += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer1_in_.x[0],driver.teensy_port.accelerometer1_in_.x[1]);
		tester.acc1_ground_state_.y += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer1_in_.y[0],driver.teensy_port.accelerometer1_in_.y[1]);
		tester.acc1_ground_state_.z += -0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer1_in_.z[0],driver.teensy_port.accelerometer1_in_.z[1]);
		tester.acc2_ground_state_.x += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer2_in_.x[0],driver.teensy_port.accelerometer2_in_.x[1]);
		tester.acc2_ground_state_.y += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer2_in_.y[0],driver.teensy_port.accelerometer2_in_.y[1]);
		tester.acc2_ground_state_.z += -0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer2_in_.z[0],driver.teensy_port.accelerometer2_in_.z[1]);
		tester.acc3_ground_state_.x += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer3_in_.x[0],driver.teensy_port.accelerometer3_in_.x[1]);
		tester.acc3_ground_state_.y += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer3_in_.y[0],driver.teensy_port.accelerometer3_in_.y[1]);
		tester.acc3_ground_state_.z += -0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer3_in_.z[0],driver.teensy_port.accelerometer3_in_.z[1]);
		tester.acc4_ground_state_.x += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer4_in_.x[0],driver.teensy_port.accelerometer4_in_.x[1]);
		tester.acc4_ground_state_.y += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer4_in_.y[0],driver.teensy_port.accelerometer4_in_.y[1]);
		tester.acc4_ground_state_.z += -0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer4_in_.z[0],driver.teensy_port.accelerometer4_in_.z[1]);
		tester.acc5_ground_state_.x += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer5_in_.x[0],driver.teensy_port.accelerometer5_in_.x[1]);
		tester.acc5_ground_state_.y += 0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer5_in_.y[0],driver.teensy_port.accelerometer5_in_.y[1]);
		tester.acc5_ground_state_.z += -0.001*AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer5_in_.z[0],driver.teensy_port.accelerometer5_in_.z[1]);
		usleep(500);
	}

    //We run repeatedly run the acceleration wave tests until we cancel out of them
    while(run_test_flag) {
        time_current = high_resolution_clock::now();
        current_timepoint = duration_cast<microseconds>(time_current-time_start).count();
        
        //We update the velocity each ms. This is effectively our acceleration sampling resolution
        if(current_timepoint - last_timepoint > 1000) {
            //We push the latest measurments from the accelerometers to the tester so that they can be stored
            tester.acc0_latest_measurements_.x = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer0_in_.x[0],driver.teensy_port.accelerometer0_in_.x[1]);
            tester.acc0_latest_measurements_.y = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer0_in_.y[0],driver.teensy_port.accelerometer0_in_.y[1]);
            tester.acc0_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer0_in_.z[0],driver.teensy_port.accelerometer0_in_.z[1]);
            tester.acc1_latest_measurements_.x = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer1_in_.x[0],driver.teensy_port.accelerometer1_in_.x[1]);
            tester.acc1_latest_measurements_.y = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer1_in_.y[0],driver.teensy_port.accelerometer1_in_.y[1]);
            tester.acc1_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer1_in_.z[0],driver.teensy_port.accelerometer1_in_.z[1]);
            tester.acc2_latest_measurements_.x = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer2_in_.x[0],driver.teensy_port.accelerometer2_in_.x[1]);
            tester.acc2_latest_measurements_.y = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer2_in_.y[0],driver.teensy_port.accelerometer2_in_.y[1]);
            tester.acc2_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer2_in_.z[0],driver.teensy_port.accelerometer2_in_.z[1]);
            tester.acc3_latest_measurements_.x = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer3_in_.x[0],driver.teensy_port.accelerometer3_in_.x[1]);
            tester.acc3_latest_measurements_.y = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer3_in_.y[0],driver.teensy_port.accelerometer3_in_.y[1]);
            tester.acc3_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer3_in_.z[0],driver.teensy_port.accelerometer3_in_.z[1]);
            tester.acc4_latest_measurements_.x = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer4_in_.x[0],driver.teensy_port.accelerometer4_in_.x[1]);
            tester.acc4_latest_measurements_.y = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer4_in_.y[0],driver.teensy_port.accelerometer4_in_.y[1]);
            tester.acc4_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer4_in_.z[0],driver.teensy_port.accelerometer4_in_.z[1]);
            tester.acc5_latest_measurements_.x = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer5_in_.x[0],driver.teensy_port.accelerometer5_in_.x[1]);
            tester.acc5_latest_measurements_.y = AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer5_in_.y[0],driver.teensy_port.accelerometer5_in_.y[1]);
            tester.acc5_latest_measurements_.z = -AccelerationBytesToPhysicalDouble(driver.teensy_port.accelerometer5_in_.z[0],driver.teensy_port.accelerometer5_in_.z[1]);


            //We write the last measurements into a file
            tester.time_ = current_timepoint/1000000.0;
            tester.WriteToFile();

            //Determine the new target velocity and push it back to the driver
            tester.UpdateVelocity();
            driver.BFF_velocity_target_ = tester.BFF_velocity_input_;

            //Push the new requests back to the device
            driver.UpdateBFFVelocity(driver.BFF_velocity_target_);
            driver.RequestAccelerations();
            last_timepoint = current_timepoint;
        }
        driver.teensy_port.PacketManager();
    }

    return -1;
}