#ifndef __AWESOMO_GIMBAL_HPP__
#define __AWESOMO_GIMBAL_HPP__

#include <iostream>
#include <math.h>
#include <map>
#include <yaml-cpp/yaml.h>

#include "awesomo_core/utils/utils.hpp"
#include "awesomo_core/vision/gimbal/sbgc.hpp"


namespace awesomo {

class Gimbal {
public:
  bool configured;

  SBGC sbgc;
  Pose camera_offset;
  double limits[6];
  Vec3 setpoints;
  Vec3 target_bpf;

  // Gimbal data
  Vec3 imu_accel;
  Vec3 imu_gyro;
  Vec3 camera_angles;
  Vec3 frame_angles;
  Vec3 rc_angles;

  Gimbal(void);
  int configure(std::string config_path);
  int updateGimbalStates(void);
  Vec3 getTargetPositionInBodyFrame(Vec3 target_cf);
  Vec3 getTargetPositionInBodyPlanarFrame(Vec3 target_cf, Quaternion &imu_if);
  int getTargetPositionInBodyPlanarFrame(Vec3 target_cf, Vec3 &target_bpf);
  int trackTarget(Vec3 target_cf);
  void printSetpoints(void);
  int setAngle(double roll, double pitch);
};

}  // end of awesomo namespace
#endif
