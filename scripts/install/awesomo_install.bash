#!/bin/bash
set -e  # exit on first error
CATKIN_PATH="$HOME/catkin_ws"
SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# refresh permission
sudo -v

# intall dependencies
# sudo bash $SCRIPT_PATH/ximea_install.bash
sudo bash $SCRIPT_PATH/pointgrey_install.bash
sudo bash $SCRIPT_PATH/swathmore_apriltags.bash
sudo apt-get install ros-indigo-mavros \
                     ros-indigo-mavros-extras \
                     ros-indigo-mavros-msgs

# link repo to catkin workspace
ln -sfn "$REPO_PATH" "$CATKIN_PATH/src/awesomo"

# compile
cd "$CATKIN_PATH"
source /opt/ros/indigo/setup.bash
catkin_make