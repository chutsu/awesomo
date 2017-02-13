#include "awesomo_core/awesomo_test.hpp"
#include "awesomo_core/control/tracking_controller.hpp"

#define TEST_CONFIG "tests/configs/control/tracking_controller.yaml"


namespace awesomo {

TEST(TrackingController, constructor) {
  TrackingController controller;

  ASSERT_FALSE(controller.configured);

  ASSERT_FLOAT_EQ(0.0, controller.pctrl_dt);
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_dt);

  ASSERT_FLOAT_EQ(0.0, controller.x_controller.k_p);
  ASSERT_FLOAT_EQ(0.0, controller.x_controller.k_i);
  ASSERT_FLOAT_EQ(0.0, controller.x_controller.k_d);

  ASSERT_FLOAT_EQ(0.0, controller.y_controller.k_p);
  ASSERT_FLOAT_EQ(0.0, controller.y_controller.k_i);
  ASSERT_FLOAT_EQ(0.0, controller.y_controller.k_d);

  ASSERT_FLOAT_EQ(0.0, controller.z_controller.k_p);
  ASSERT_FLOAT_EQ(0.0, controller.z_controller.k_i);
  ASSERT_FLOAT_EQ(0.0, controller.z_controller.k_d);
  ASSERT_FLOAT_EQ(0.0, controller.hover_throttle);

  ASSERT_FLOAT_EQ(0.0, controller.vx_controller.k_p);
  ASSERT_FLOAT_EQ(0.0, controller.vx_controller.k_i);
  ASSERT_FLOAT_EQ(0.0, controller.vx_controller.k_d);

  ASSERT_FLOAT_EQ(0.0, controller.vy_controller.k_p);
  ASSERT_FLOAT_EQ(0.0, controller.vy_controller.k_i);
  ASSERT_FLOAT_EQ(0.0, controller.vy_controller.k_d);

  ASSERT_FLOAT_EQ(0.0, controller.vz_controller.k_p);
  ASSERT_FLOAT_EQ(0.0, controller.vz_controller.k_i);
  ASSERT_FLOAT_EQ(0.0, controller.vz_controller.k_d);

  ASSERT_FLOAT_EQ(0.0, controller.roll_limit[0]);
  ASSERT_FLOAT_EQ(0.0, controller.roll_limit[1]);

  ASSERT_FLOAT_EQ(0.0, controller.pitch_limit[0]);
  ASSERT_FLOAT_EQ(0.0, controller.pitch_limit[1]);

  ASSERT_FLOAT_EQ(0.0, controller.throttle_limit[0]);
  ASSERT_FLOAT_EQ(0.0, controller.throttle_limit[1]);

  ASSERT_FLOAT_EQ(0.0, controller.track_offset[0]);
  ASSERT_FLOAT_EQ(0.0, controller.track_offset[1]);
  ASSERT_FLOAT_EQ(0.0, controller.track_offset[2]);

  ASSERT_FLOAT_EQ(0.0, controller.pctrl_setpoints(0));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_setpoints(0));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_setpoints(1));

  ASSERT_FLOAT_EQ(0.0, controller.vctrl_setpoints(2));
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_setpoints(1));
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_setpoints(2));

  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(0));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(1));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(2));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(3));

  ASSERT_FLOAT_EQ(0.0, controller.vctrl_outputs(0));
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_outputs(1));
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_outputs(2));
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_outputs(3));
}

TEST(TrackingController, configure) {
  TrackingController controller;

  controller.configure(TEST_CONFIG);

  ASSERT_TRUE(controller.configured);

  ASSERT_FLOAT_EQ(0.0, controller.pctrl_dt);
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_dt);

  ASSERT_FLOAT_EQ(0.1, controller.x_controller.k_p);
  ASSERT_FLOAT_EQ(0.2, controller.x_controller.k_i);
  ASSERT_FLOAT_EQ(0.3, controller.x_controller.k_d);

  ASSERT_FLOAT_EQ(0.1, controller.y_controller.k_p);
  ASSERT_FLOAT_EQ(0.2, controller.y_controller.k_i);
  ASSERT_FLOAT_EQ(0.3, controller.y_controller.k_d);

  ASSERT_FLOAT_EQ(0.1, controller.z_controller.k_p);
  ASSERT_FLOAT_EQ(0.2, controller.z_controller.k_i);
  ASSERT_FLOAT_EQ(0.3, controller.z_controller.k_d);
  ASSERT_FLOAT_EQ(0.6, controller.hover_throttle);

  ASSERT_FLOAT_EQ(0.0, controller.vctrl_dt);
  ASSERT_FLOAT_EQ(1.0, controller.vx_controller.k_p);
  ASSERT_FLOAT_EQ(2.0, controller.vx_controller.k_i);
  ASSERT_FLOAT_EQ(3.0, controller.vx_controller.k_d);

  ASSERT_FLOAT_EQ(1.0, controller.vy_controller.k_p);
  ASSERT_FLOAT_EQ(2.0, controller.vy_controller.k_i);
  ASSERT_FLOAT_EQ(3.0, controller.vy_controller.k_d);

  ASSERT_FLOAT_EQ(1.0, controller.vz_controller.k_p);
  ASSERT_FLOAT_EQ(2.0, controller.vz_controller.k_i);
  ASSERT_FLOAT_EQ(3.0, controller.vz_controller.k_d);

  ASSERT_FLOAT_EQ(deg2rad(-20.0), controller.roll_limit[0]);
  ASSERT_FLOAT_EQ(deg2rad(20.0), controller.roll_limit[1]);

  ASSERT_FLOAT_EQ(deg2rad(-20.0), controller.pitch_limit[0]);
  ASSERT_FLOAT_EQ(deg2rad(20.0), controller.pitch_limit[1]);

  ASSERT_FLOAT_EQ(-1.0, controller.throttle_limit[0]);
  ASSERT_FLOAT_EQ(1.0, controller.throttle_limit[1]);

  ASSERT_FLOAT_EQ(1.0, controller.track_offset[0]);
  ASSERT_FLOAT_EQ(2.0, controller.track_offset[1]);
  ASSERT_FLOAT_EQ(3.0, controller.track_offset[2]);

  ASSERT_FLOAT_EQ(-1.0, controller.throttle_limit[0]);
  ASSERT_FLOAT_EQ(1.0, controller.throttle_limit[1]);

  ASSERT_FLOAT_EQ(0.0, controller.pctrl_setpoints(0));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_setpoints(1));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_setpoints(2));

  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(0));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(1));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(2));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(3));
}

TEST(TrackingController, calculatePositionErrors) {
  Vec3 errors;
  double yaw, dt;
  TrackingController controller;

  // setup
  controller.configure(TEST_CONFIG);
  controller.track_offset << 0.0, 0.0, 0.0;

  // CHECK HOVERING PID OUTPUT
  errors << 0, 0, 0;
  yaw = 0.0;
  dt = 0.1;
  controller.calculatePositionErrors(errors, yaw, dt);
  controller.printOutputs();

  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(0));
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(1));
  ASSERT_FLOAT_EQ(controller.hover_throttle, controller.pctrl_outputs(3));

  // CHECK MOVING TOWARDS THE X LOCATION
  errors << 1, 0, 0;
  yaw = 0.0;
  dt = 0.1;

  controller.reset();
  controller.calculatePositionErrors(errors, yaw, dt);
  controller.printOutputs();

  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(0));
  ASSERT_TRUE(controller.pctrl_outputs(1) > 0.0);

  // CHECK MOVING TOWARDS THE Y LOCATION
  errors << 0, 1, 0;
  yaw = 0.0;
  dt = 0.1;

  controller.reset();
  controller.calculatePositionErrors(errors, yaw, dt);
  controller.printOutputs();

  ASSERT_TRUE(controller.pctrl_outputs(0) < 0.0);
  ASSERT_FLOAT_EQ(0.0, controller.pctrl_outputs(1));

  // CHECK MOVING TOWARDS THE X AND Y LOCATION
  errors << 1, 1, 0;
  yaw = 0.0;
  dt = 0.1;

  controller.reset();
  controller.calculatePositionErrors(errors, yaw, dt);
  controller.printOutputs();

  ASSERT_TRUE(controller.pctrl_outputs(0) < 0.0);
  ASSERT_TRUE(controller.pctrl_outputs(1) > 0.0);

  // CHECK MOVING YAW
  errors << 0, 0, 0;
  yaw = deg2rad(90.0);
  dt = 0.1;

  controller.reset();
  Vec4 pctrl_outputs = controller.calculatePositionErrors(errors, yaw, dt);
  controller.printOutputs();

  ASSERT_FLOAT_EQ(yaw, pctrl_outputs(2));
}

TEST(TrackingController, calculateVelocityErrors) {
  Vec3 verrors;
  double dt;
  TrackingController controller;

  // setup
  controller.configure(TEST_CONFIG);
  controller.track_offset << 0.0, 0.0, 0.0;

  // CHECK HOVERING PID OUTPUT
  verrors << 0, 0, 0;
  dt = 0.1;

  controller.calculateVelocityErrors(verrors, 0.0, dt);
  controller.printOutputs();

  ASSERT_FLOAT_EQ(0.0, controller.vctrl_outputs(0));
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_outputs(1));
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_outputs(3));

  // CHECK MOVING TOWARDS THE X LOCATION
  verrors << 1, 0, 0;
  dt = 0.1;

  controller.reset();
  controller.calculateVelocityErrors(verrors, 0.0, dt);
  controller.printOutputs();

  ASSERT_FLOAT_EQ(0.0, controller.vctrl_outputs(0));
  ASSERT_TRUE(controller.vctrl_outputs(1) > 0.0);

  // CHECK MOVING TOWARDS THE Y LOCATION
  verrors << 0, 1, 0;
  dt = 0.1;

  controller.reset();
  controller.calculateVelocityErrors(verrors, 0.0, dt);
  controller.printOutputs();

  ASSERT_TRUE(controller.vctrl_outputs(0) < 0.0);
  ASSERT_FLOAT_EQ(0.0, controller.vctrl_outputs(1));

  // CHECK MOVING TOWARDS THE X AND Y LOCATION
  verrors << 1, 1, 0;
  dt = 0.1;

  controller.reset();
  controller.calculateVelocityErrors(verrors, 0.0, dt);
  controller.printOutputs();

  ASSERT_TRUE(controller.vctrl_outputs(0) < 0.0);
  ASSERT_TRUE(controller.vctrl_outputs(1) > 0.0);
}

}  // end of awesomo namepsace
