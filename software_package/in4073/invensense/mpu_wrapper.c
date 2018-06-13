/*------------------------------------------------------------------
 *  mpu_wrapper.c -- invensense sdk setup
 *
 *  I. Protonotarios
 *  Embedded Software Lab
 *
 *  July 2016
 *------------------------------------------------------------------
 */
#include "in4073.h"
#include "drone.h"

#define QUAT_SENS       0x040000000 //1073741824.f //2^30

void update_euler_from_quaternions(int32_t *quat) 
{
	float q[4];

	q[0] = (float)quat[0]/QUAT_SENS;
	q[1] = (float)quat[1]/QUAT_SENS;
	q[2] = (float)quat[2]/QUAT_SENS;
	q[3] = (float)quat[3]/QUAT_SENS;

	phi = (int16_t) (atan2(2.0*(q[2]*q[3] + q[0]*q[1]), q[0]*q[0] - q[1]*q[1] - q[2]*q[2] + q[3]*q[3])*10430.0) - phi_offset;
	theta = (int16_t) (-1.0 * asin(-2.0*(q[1]*q[3] - q[0]*q[2]))*10430.0) - theta_offset;
	psi = (int16_t) (atan2(2.0*(q[1]*q[2] + q[0]*q[3]), q[0]*q[0] + q[1]*q[1] - q[2]*q[2] - q[3]*q[3])*10430.0) - psi_offset;
}

// reading & conversion takes 3.2 ms!!! hurray (still lots of time till 10)
void get_dmp_data(void)
{
	int8_t read_stat;
	int16_t gyro[3], accel[3], dmp_sensors;
	int32_t quat[4];

	if (!(read_stat = dmp_read_fifo(gyro, accel, quat, NULL, &dmp_sensors, &sensor_fifo_count)))
	{
		update_euler_from_quaternions(quat);
		if (dmp_sensors & INV_XYZ_GYRO)
		{
			sp = gyro[0];
			sq = gyro[1];
			sr = gyro[2];
		}
		if (dmp_sensors & INV_XYZ_ACCEL)
		{
			sax = accel[0];
			say = accel[1];
			saz = accel[2];
		}
	}
	else DEBUG_PRINT("Error reading sensor fifo\n\f");
	//printf("Error reading sensor fifo: %d\n", read_stat);
}

void get_raw_sensor_data(void)
{
	int8_t read_stat;
	int16_t gyro[3], accel[3];
	uint8_t sensors;

	if (!(read_stat = mpu_read_fifo(gyro, accel, NULL, &sensors, &sensor_fifo_count)))
	{
		if (sensors & INV_XYZ_GYRO) //16.4 LSB/deg/s (+-2000 deg/s)  You might need this for Kalman
		{
			sp = gyro[0];
			sq = gyro[1];
			sr = gyro[2];
		}
		if (sensors & INV_XYZ_ACCEL) //	16384 LSB/g (+-2g)  You might need this for Kalman
		{
			sax = accel[0];
			say = accel[1];
			saz = accel[2];
		}
	}
	else printf("Error reading raw sensor fifo: %d\n", read_stat);
}



bool check_sensor_int_flag(void)
{
	if (nrf_gpio_pin_read(INT_PIN) || sensor_fifo_count) return true;
	return false;
}

void imu_init(bool dmp, uint16_t freq)
{
	static int8_t gyro_orientation[9] = {	1, 0, 0,
											0, 1, 0,
											0, 0, 1	};

	// tap feature is there to set freq to 100Hz, a bug provided by invensense :)
	uint16_t dmp_features = DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO | DMP_FEATURE_GYRO_CAL | DMP_FEATURE_TAP;

	//mpu	
	printf("\rmpu init result: %d\n", mpu_init(NULL));
	printf("\rmpu set sensors: %d\n", mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL));
	printf("\rmpu conf fifo  : %d\n", mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL));
	mpu_set_int_level(0);
	mpu_set_int_latched(1);

	if (dmp)
	{
		printf("\r\ndmp load firm  : %d\n", dmp_load_motion_driver_firmware());
		printf("\rdmp set orient : %d\n", dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation)));
	
		printf("\rdmp en features: %d\n", dmp_enable_feature(dmp_features));
		printf("\rdmp set rate   : %d\n", dmp_set_fifo_rate(100));
		printf("\rdmp set state  : %d\n", mpu_set_dmp_state(1));
		printf("\rdlpf set freq  : %d\n", mpu_set_lpf(10));
	} else {
		unsigned char data = 0;
		printf("\rdisable dlpf   : %d\n", i2c_write(0x68, 0x1A, 1, &data));
		// if dlpf is disabled (0 or 7) then the sample divider that feeds the fifo is 8kHz (derrived from gyro).
		data = 8000 / freq - 1;
		printf("\rset sample rate: %d\n", i2c_write(0x68, 0x19, 1, &data));
	}
}
