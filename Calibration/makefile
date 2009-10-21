########################################################################
# Makefile for the Vrui calibration utility package.
# Copyright (c) 2008-2009 Oliver Kreylos
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or (at
# your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
########################################################################

# Set the Vrui installation directory:
VRUIDIR = $(HOME)/Vrui-1.0

# Set up additional flags for the C++ compiler:
CFLAGS = 

# Include the Vrui application makefile fragment:
ifdef DEBUG
  # Build debug version of the applications, using the debug version of Vrui:
  include $(VRUIDIR)/etc/Vrui.debug.makeinclude
  CFLAGS += -g2 -O0
else
  # Build release version of the applications, using the release version of Vrui:
  include $(VRUIDIR)/etc/Vrui.makeinclude
  CFLAGS += -g0 -O3
endif

# List all project targets:
ALL = XBackground \
      MeasureEnvironment \
      ScreenCalibrator \
      AlignTrackingMarkers

.PHONY: all
all: $(ALL)

# Rule to remove all targets:
.PHONY: clean
clean:
	-rm -rf o
	-rm -f $(ALL)

# Pattern rule to compile C++ sources:
o/%.o: %.cpp
	@mkdir -p o/$(*D)
	@echo Compiling $<...
	@g++ -c -o $@ $(VRUI_CFLAGS) $(CFLAGS) $<

# Pattern rule to link executables:
%: o/%.o
	@echo Linking $@...
	@g++ -o $@ $^ $(VRUI_LINKFLAGS)
	
# List all build rules:

XBackground: o/XBackground.o

MeasureEnvironment: o/TotalStation.o \
                    o/MeasureEnvironment.o

ScreenCalibrator: o/ScreenCalibrator.o

AlignTrackingMarkers: o/ReadOptiTrackMarkerFile.o \
                      o/AlignTrackingMarkers.o
