#!/bin/bash
########################################################################
# Script to run a Vrui application on an HTC Vive head-mounted display.
# Copyright (c) 2018 Oliver Kreylos
# 
# This file is part of the Virtual Reality User Interface Library
# (Vrui).
# 
# The Virtual Reality User Interface Library is free software; you can
# redistribute it and/or modify it under the terms of the GNU General
# Public License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# The Virtual Reality User Interface Library is distributed in the hope
# that it will be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
# PURPOSE.  See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the Virtual Reality User Interface Library; if not, write
# to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
# Boston, MA 02111-1307 USA
########################################################################

# Configure environment:
VRUIBINDIR=/usr/local/bin

# Extract the name of the Vrui application and remove it from the
# command line:
VRUIAPPNAME=$1
shift

# Find the video output port to which the Vive is connected:
VIVE_OUTPUT_PORT=$($VRUIBINDIR/FindHMD -size 2160 1200 -rate 89.53 2>/dev/null)
RESULT=$?
if (($RESULT == 0)); then
	# Tell OpenGL to synchronize with the Vive's display:
	export __GL_SYNC_DISPLAY_DEVICE=$VIVE_OUTPUT_PORT
	
	# Run the application in Vive mode and send the HMD window to the Vive's display:
	$VRUIAPPNAME -rootSection Vive -setConfig HMDWindow/outputName=$VIVE_OUTPUT_PORT "$@"
elif (($RESULT == 1)); then
	echo No Vive HMD found\; please connect your Vive and try again
else
	echo Disabled Vive HMD found on output $VIVE_OUTPUT_PORT\; please enable your Vive using xrandr or nvidia-settings and try again
fi
