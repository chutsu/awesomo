#ifndef ATL_GAZEBO_BRIDGE_LZ_NODE_HPP
#define ATL_GAZEBO_BRIDGE_LZ_NODE_HPP

#include <ros/ros.h>
#include <std_msgs/Float64.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/Vector3.h>

#include "atl/utils/utils.hpp"
#include "atl/ros/utils/node.hpp"
#include "atl_gazebo/clients/lz_gclient.hpp"


namespace atl {
namespace ros {

// NODE SETTINGS
#define NODE_NAME "atl_lz"
#define NODE_RATE 100

// PUBLISH TOPICS
#define POSE_RTOPIC "/atl/lz/pose"

// SUBSCRIBE TOPICS
#define POSITION_SET_RTOPIC "/atl/lz/position/set"
#define VELOCITY_SET_RTOPIC "/atl/lz/velocity/set"
#define ANGULAR_VEL_SET_RTOPIC "/atl/lz/angular_velocity/set"

class LZNode : public gaz::LZGClient, public ROSNode {
public:
  LZNode(int argc, char **argv) : ROSNode(argc, argv) {}
  int configure(const std::string &node_name, int hz);
  void poseCallback(ConstPosePtr &msg);
  void positionCallback(const geometry_msgs::Vector3 &msg);
  void velocityCallback(const std_msgs::Float64 &msg);
  void angularVelocityCallback(const std_msgs::Float64 &msg);
};

}  // namespace ros
}  // namespace atl
#endif
