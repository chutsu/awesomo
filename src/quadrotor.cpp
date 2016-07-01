#include "awesomo/quadrotor.hpp"


Quadrotor::Quadrotor(std::map<std::string, std::string> configs)
{
    std::string config_path;

    // precheck
    if (configs.count("quadrotor") == 0) {
        std::cout << "ERROR! quadrotor config not set!" << std::endl;
    }

    // configs
    this->tracking_config = new TrackingConfig();
    this->landing_config = new LandingConfig();

    // intialize state
    this->mission_state = IDLE_MODE;
    this->pose.x = 0.0;
    this->pose.y = 0.0;
    this->pose.z = 0.0;
    this->hover_point  = new HoverPoint();

    // landing state
    this->landing_zone_belief = 0;

    // estimators
    this->estimator_initialized = false;

    // initialize position controller
    if (configs.count("position_controller")) {
        config_path = configs["position_controller"];
        this->position_controller = new PositionController(config_path);
    } else {
        this->position_controller = NULL;
    }

    // carrot controller
    if (configs.count("carrot_controller")) {
        config_path = configs["carrot_controller"];
        this->carrot_controller = new CarrotController(config_path);
    } else {
        this->carrot_controller = NULL;
    }

    // load quadrotor configuration
    config_path = configs["quadrotor"];
    this->loadConfig(config_path);
}

int Quadrotor::loadConfig(std::string config_file_path)
{
    YAML::Node config;
    YAML::Node tracking;
    YAML::Node landing;

    try {
        // load config
        config = YAML::LoadFile(config_file_path);

        // load hover point config
        this->hover_point->initialized = false; // later in the code this will be set
        this->hover_point->x = 0.0f;
        this->hover_point->y = 0.0f;
        this->hover_point->z = config["hover_point"]["height"].as<float>();

        // load tracking config
        tracking = config["tracking"]["position_offset"];
        this->tracking_config->offset_x = tracking["x"].as<float>();
        this->tracking_config->offset_y = tracking["y"].as<float>();
        this->tracking_config->offset_z = tracking["z"].as<float>();

        // load landing config
        landing = config["landing"]["height_update"];
        this->landing_config->period = landing["period"].as<float>();

        landing = config["landing"]["height_update"]["multiplier"];
        this->landing_config->descend_multiplier = landing["descend"].as<float>();
        this->landing_config->recover_multiplier = landing["recover"].as<float>();

        landing = config["landing"]["disarm_conditions"];
        this->landing_config->x_cutoff = landing["x_cutoff"].as<float>();
        this->landing_config->y_cutoff = landing["y_cutoff"].as<float>();
        this->landing_config->z_cutoff = landing["z_cutoff"].as<float>();
        this->landing_config->belief_threshold = landing["belief_threshold"].as<float>();

    } catch (YAML::BadFile &ex) {
        throw;
    }

    return 0;
}

void Quadrotor::updatePose(Pose pose)
{
    this->pose.x = pose.x;
    this->pose.y = pose.y;
    this->pose.z = pose.z;

    this->pose.roll = pose.roll;
    this->pose.pitch = pose.pitch;
    this->pose.yaw = pose.yaw;
}

void Quadrotor::updateHoverPointWithTag(Pose robot_pose, float tag_x, float tag_y)
{
    this->hover_point->initialized = true;
    this->hover_point->x = robot_pose.x + tag_x;
    this->hover_point->y = robot_pose.y + tag_y;
}

bool Quadrotor::hasLanded(LandingTargetPosition landing_zone)
{
    struct kf *estimator;
    float x_cutoff;
    float y_cutoff;
    float z_cutoff;
    bool tag_x_ok;
    bool tag_y_ok;
    bool tag_z_ok;
    bool mu_x_ok;
    bool mu_y_ok;
    bool mu_z_ok;

    // setup
    estimator = &this->tag_estimator;
    x_cutoff = this->landing_config->x_cutoff;
    y_cutoff = this->landing_config->y_cutoff;
    z_cutoff = this->landing_config->z_cutoff;

    tag_x_ok = (landing_zone.x < x_cutoff) ? true: false;
    tag_y_ok = (landing_zone.y < y_cutoff) ? true: false;
    tag_z_ok = (landing_zone.z < z_cutoff) ? true: false;

    mu_x_ok = (estimator->mu(0) < x_cutoff) ? true: false;
    mu_y_ok = (estimator->mu(1) < y_cutoff) ? true: false;
    mu_z_ok = (estimator->mu(2) < z_cutoff) ? true: false;

    // landed detection
    if (landing_zone.detected && tag_x_ok && tag_y_ok && tag_z_ok) {
        this->landing_zone_belief++;
    } else if (mu_x_ok && mu_y_ok && mu_z_ok) {
        this->landing_zone_belief++;
    }

    // disarm?
    if (this->landing_zone_belief == this->landing_config->belief_threshold) {
        return true;
    }

    return false;
}

Attitude Quadrotor::positionControllerCalculate(
    Position setpoint,
    Pose robot_pose,
    float dt
)
{
    Attitude a;
    // printf("setpoint: %f %f %f\n", setpoint.x, setpoint.y, setpoint.z);
    // printf("robot_pose: %f %f %f\n", robot_pose.x, robot_pose.y, robot_pose.z);

    this->position_controller->calculate(setpoint, robot_pose, dt);

    a.x = this->position_controller->rpy_quat.x();
    a.y = this->position_controller->rpy_quat.y();
    a.z = this->position_controller->rpy_quat.z();
    a.w = this->position_controller->rpy_quat.w();

    a.roll = this->position_controller->roll;
    a.pitch = this->position_controller->pitch;
    a.yaw = 0;

    return a;
}

void Quadrotor::resetPositionController(void)
{
    this->position_controller->x.sum_error = 0.0;
    this->position_controller->x.prev_error = 0.0;
    this->position_controller->x.output = 0.0;

    this->position_controller->y.sum_error = 0.0;
    this->position_controller->y.prev_error = 0.0;
    this->position_controller->y.output = 0.0;

    this->position_controller->T.sum_error = 0.0;
    this->position_controller->T.prev_error = 0.0;
    this->position_controller->T.output = 0.0;
}

void Quadrotor::runIdleMode(Pose robot_pose)
{
    std::cout << "IDLE MODE!" << std::endl;

    // transition to offboard mode
    this->mission_state = DISCOVER_MODE;

    // hover inplace
    if (this->hover_point->initialized == false) {
        this->hover_point->initialized = true;
        this->hover_point->x = robot_pose.x;
        this->hover_point->y = robot_pose.y;
        this->hover_point->z;  // configured in config file
    }
}

Position Quadrotor::runHoverMode(Pose robot_pose, float dt)
{
    Position cmd;

    // pre-check - if hover point not set, hover at current pose
    if (this->hover_point->initialized == false) {
        this->hover_point->initialized = true;
        this->hover_point->x = robot_pose.x;
        this->hover_point->y = robot_pose.y;
        this->hover_point->z;  // configured in config file
    }

    // command position
    cmd.x = this->hover_point->x;
    cmd.y = this->hover_point->y;
    cmd.z = this->hover_point->z;

    this->positionControllerCalculate(cmd, robot_pose, dt);

    return cmd;
}

void Quadrotor::initializeCarrotController(void)
{
    Position p;
    Eigen::Vector3d wp;
    Eigen::Vector3d carrot;
    Eigen::Vector3d position;

    // setup
    std::cout << "Initializing Carrot Controller!" << std::endl;

    // current position + some altitude
    p.x = this->pose.x;
    p.y = this->pose.y;
    p.z = this->hover_point->z;

    // waypoint 1
    wp << p.x, p.y, p.z;
    this->carrot_controller->wp_start = wp;
    this->carrot_controller->waypoints.push_back(wp);

    // waypoint 2
    wp << p.x + 5, p.y, p.z;
    this->carrot_controller->wp_end = wp;
    this->carrot_controller->waypoints.push_back(wp);

    // waypoint 3
    wp << p.x + 5, p.y + 5, p.z;
    this->carrot_controller->waypoints.push_back(wp);

    // waypoint 4
    wp << p.x, p.y + 5, p.z;
    this->carrot_controller->waypoints.push_back(wp);

    // back to waypoint 1
    wp << p.x, p.y, p.z;
    this->carrot_controller->waypoints.push_back(wp);

    // initialize carrot controller
    this->carrot_controller->initialized = 1;
    this->mission_state = CARROT_MODE;
    std::cout << "Transitioning to Carrot Controller Mode!" << std::endl;
}

Position Quadrotor::runCarrotMode(Pose robot_pose, float dt)
{
    int retval;
    Position cmd;
    Eigen::Vector3d position;
    Eigen::Vector3d carrot;

    // setup
    position << robot_pose.x, robot_pose.y, robot_pose.z;

    // calculate new carrot point
    if (this->carrot_controller->update(position, carrot)) {
        std::cout << "No more waypoints!" << std::endl;
        std::cout << "Transitioning to Hover Mode!" << std::endl;

        this->mission_state = HOVER_MODE;
        this->hover_point->initialized = true;
        this->hover_point->x = this->carrot_controller->wp_end(0);
        this->hover_point->y = this->carrot_controller->wp_end(1);
        this->hover_point->z = this->carrot_controller->wp_end(2);

        cmd.x = this->hover_point->x;
        cmd.y = this->hover_point->y;
        cmd.z = this->hover_point->z;

    } else if (retval == 1) {
        this->hover_point->initialized = true;
        this->hover_point->x = carrot(0);
        this->hover_point->y = carrot(1);
        this->hover_point->z = carrot(2);

        cmd.x = carrot(0);
        cmd.y = carrot(1);
        cmd.z = carrot(2);

        this->hover_point->initialized = true;
        this->hover_point->x = this->carrot_controller->wp_end(0);
        this->hover_point->y = this->carrot_controller->wp_end(1);
        this->hover_point->z = this->carrot_controller->wp_end(2);


    } else {
        cmd = this->runHoverMode(robot_pose, dt);
        return cmd;
    }

    cmd.x = carrot(0);
    cmd.y = carrot(1);
    cmd.z = carrot(2);

    return cmd;
}

Position Quadrotor::runDiscoverMode(
    Pose robot_pose,
    LandingTargetPosition landing_zone,
    float dt
)
{
    Position cmd;
	Eigen::VectorXd mu(9);

    if (landing_zone.detected == true) {
        // initialize kalman filter
        mu << landing_zone.x,  // pos_x relative to quad
              landing_zone.y,  // pos_y relative to quad
              landing_zone.z,  // pos_z relative to quad
              0.0, 0.0, 0.0,  // vel_x, vel_y, vel_z
              0.0, 0.0, 0.0;  // acc_x, acc_y, acc_z
        apriltag_kf_setup(&this->tag_estimator, mu);
        this->estimator_initialized = true;

        // transition to tracker mode
        std::cout << "Transitioning to Tracker Mode!" << std::endl;
        this->mission_state = TRACKING_MODE;
        this->tracking_start = time(NULL);

        // keep track of hover point
        this->hover_point->initialized = true;
        this->hover_point->x = robot_pose.x + landing_zone.x;
        this->hover_point->y = robot_pose.y + landing_zone.y;
    }

    // hover in-place
    cmd = this->runHoverMode(robot_pose, dt);

    return cmd;
}

Position Quadrotor::trackApriltag(
    Pose robot_pose,
    LandingTargetPosition landing_zone,
    float dt
)
{
    Position tag;
	Eigen::VectorXd y(3);
	float target_lost_elasped;

	// calculate how long we've lost target
	target_lost_elasped = difftime(time(NULL), this->tag_last_updated);

    // estimate tag position
    y << landing_zone.x, landing_zone.y, landing_zone.z;
    apriltag_kf_estimate(&this->tag_estimator, y, dt, landing_zone.detected);

    // build position command
    tag.x = this->apriltag_estimator.mu(0);
    tag.y = this->apriltag_estimator.mu(1);
    tag.z = this->hover_point->z;

    // keep track of target position
    if (landing_zone.detected == true) {
        // update hover point
        this->hover_point->initialized = true;
        this->hover_point->x = robot_pose.x + landing_zone.x;
        this->hover_point->y = robot_pose.y + landing_zone.y;
        this->target_last_updated = time(NULL);
    }

    // transition to landing?
    // elasped = difftime(time(NULL), this->tracking_start);
    // if (elasped > 5 && landing_zone.detected && landing_zone.x < 2.0 && landing_zone.y < 2.0) {
    //     this->mission_state = LANDING_MODE;
    //     this->height_last_updated = time(NULL);
    //     std::cout << "Transitioning to Landing Mode!" << std::endl;
    // }

    return tag;
}

Position Quadrotor::runTrackingMode(
    Pose robot_pose,
    LandingTargetPosition landing_zone,
    float dt
)
{
    double elasped;
    float min_track_time;
    bool x_ok;
    bool y_ok;

    // setup
    min_track_time = this->tracking_config->min_track_time;
    x_ok = (landing_zone.x < this->landing_config->x_threshold) ? true : false;
    y_ok = (landing_zone.y < this->landing_config->y_threshold) ? true : false;

    // transition to landing?
    elasped = difftime(time(NULL), this->tracking_start);
    if (elasped > min_track_time && x_ok && y_ok) {
        this->mission_state = LANDING_MODE;
        this->height_last_updated = time(NULL);
        std::cout << "Transitioning to Landing Mode!" << std::endl;

    }

    return this->trackApriltag(robot_pose, landing_zone, dt);
}

Position Quadrotor::runLandingMode(
    Pose robot_pose,
    LandingTargetPosition landing_zone,
    float dt
)
{
    Position tag;
    Position estimation;
	Eigen::VectorXd mu(9);
	Eigen::VectorXd y(3);
    double elasped;
    float descend_multiplier;
    float recover_multiplier;
    float x_threshold;
    float y_threshold;

    // setup
    descend_multiplier = this->landing_config->descend_multiplier;
    recover_multiplier = this->landing_config->recover_multiplier;
    x_threshold = this->landing_config->x_threshold;
    y_threshold = this->landing_config->y_threshold;

    // estimate apriltag
    cmd = this->trackApriltag(robot_pose, landing_zone, dt);

    // build position command
    tag.x = this->apriltag_estimator.mu(0);
    tag.y = this->apriltag_estimator.mu(1);
    tag.z = this->hover_point->z;

    estimation.x = this->apriltag_estimator.mu(0);
    estimation.y = this->apriltag_estimator.mu(1);
    estimation.z = this->apriltag_estimator.mu(2);

    // printf(
    //     "estimation: %f %f %f\n",
    //     this->apriltag_estimator.mu(0),
    //     this->apriltag_estimator.mu(1),
    //     this->apriltag_estimator.mu(2)
    // );

    // keep track of target position
    if (landing_zone.detected == true) {
        this->hover_point->initialized = true;
        this->hover_point->x = robot_pose.x + tag.x;
        this->hover_point->y = robot_pose.y + tag.y;
        this->target_last_updated = time(NULL);

    }

    // lower height or increase height
    elasped = difftime(time(NULL), this->height_last_updated);
    if (elasped > 1 && landing_zone.detected == true) {
        if (landing_zone.x < 0.5 && landing_zone.y < 0.5) {
            this->hover_point->z = this->hover_point->z * 0.7;
            printf("Lowering hover height to %f\n", this->hover_point->z);

        } else {
            this->hover_point->z = this->hover_point->z * 1.2;
            printf("Increasing hover height to %f\n", this->hover_point->z);

        }

        this->height_last_updated = time(NULL);
    }

    // kill engines (landed?)
    if (landing_zone.x < 0.5 && landing_zone.y < 0.5 && landing_zone.z < 0.2) {
        printf("Mission Accomplished - disarming quadrotor!\n");
        this->mission_state = MISSION_ACCOMPLISHED;

    } else if (estimation.x < 0.5 && estimation.y < 0.5 && estimation.z < 0.2) {
        printf("Mission Accomplished - disarming quadrotor!\n");
        this->mission_state = MISSION_ACCOMPLISHED;

    }

    return tag;
}

int Quadrotor::followWaypoints(
    Pose robot_pose,
    LandingTargetPosition landing_zone,
    float dt
)
{
    Position setpoint;
    Pose pose;

    // update pose
    this->updatePose(robot_pose);

    // mission
    switch (this->mission_state) {
    case IDLE_MODE:
        this->runIdleMode(robot_pose);
        setpoint = this->runHoverMode(robot_pose, dt);
        break;

    case HOVER_MODE:
        setpoint = this->runHoverMode(robot_pose, dt);
        break;

    case CARROT_INITIALIZE_MODE:
        this->initializeCarrotController();
        setpoint = this->runHoverMode(robot_pose, dt);
        break;

    case CARROT_MODE:
        setpoint = this->runCarrotMode(robot_pose, dt);
        break;

    case MISSION_ACCOMPLISHED:
        return 1;
        break;

    }

    // position controller calculate
    this->positionControllerCalculate(setpoint, robot_pose, dt);

    return 0;
}

int Quadrotor::runMission(
    Pose robot_pose,
    LandingTargetPosition landing_zone,
    float dt
)
{
    Position tag_position;
    Position tag_origin;
    Pose fake_robot_pose;
    double target_lost_elasped;

    // calculate time elasped for target lost
    target_lost_elasped = difftime(time(NULL), this->target_last_updated);

    // update pose
    this->updatePose(robot_pose);

    // mission
    switch (this->mission_state) {
    case IDLE_MODE:
        this->runIdleMode(robot_pose);
        this->runHoverMode(robot_pose, dt);
        return 1;
        break;

    case DISCOVER_MODE:
        this->runDiscoverMode(robot_pose, landing_zone);
        this->runHoverMode(robot_pose, dt);
        return 1;
        break;

    case TRACKING_MODE:
        this->runTrackingMode(robot_pose, landing_zone, dt);
        break;

    case LANDING_MODE:
        this->runLandingMode(robot_pose, landing_zone, dt);
        break;

    case MISSION_ACCOMPLISHED:
        return 0;
        break;

    }

    // change from GPS to using AprilTag as world origin
    // modify setpoint and robot pose as we use GPS or AprilTag
    // swap robot pose with setpoint, since we are using AprilTag as
    // world origin, so now if
    //
    // this is now NED
    fake_robot_pose.x = 0.0;
    fake_robot_pose.y = 0.0;
    fake_robot_pose.z = 0.0 + robot_pose.z;  // don't desend this needs to be made a variable or something

    tag_origin.x = tag_position.x;
    tag_origin.y = tag_position.y;
    tag_origin.z = tag_position.z;

    if (landing_zone.detected) {
        this->positionControllerCalculate(tag_origin, fake_robot_pose, dt);

    } else {
        this->runHoverMode(robot_pose, dt);

    }

    return 1;
}
