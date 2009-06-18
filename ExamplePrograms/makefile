########################################################################
# Makefile for the Vrui example programs.
# Copyright (c) 2003-2009 Oliver Kreylos
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
ALL = VruiDemo \
      VruiLocatorDemo \
      VruiCustomToolDemo \
      VruiCalibrator \
      ShowEarthModel \
      Jello \
      ClusterJello \
      SharedJelloServer \
      SharedJello

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
VruiDemo: o/VruiDemo.o

VruiLocatorDemo: o/VruiLocatorDemo.o

VruiCustomToolDemo: o/VruiCustomToolDemo.o

VruiCalibrator: o/VruiCalibrator.o

o/ShowEarthModel.o: CFLAGS += -DSHOWEARTHMODEL_TOPOGRAPHY_IMAGEFILENAME='"$(shell pwd)/EarthTopography.ppm"'

ShowEarthModel: o/EarthFunctions.o \
                o/PointSet.o \
                o/SeismicPath.o \
                o/EarthquakeSet.o \
                o/EarthquakeTool.o \
                o/ShowEarthModel.o

Jello: o/JelloAtom.o \
       o/JelloCrystal.o \
       o/JelloRenderer.o \
       o/Jello.o

ClusterJello: o/JelloAtom.o \
              o/JelloCrystal.o \
              o/JelloRenderer.o \
              o/ClusterJello.o

SharedJelloServer: o/JelloAtom.o \
                   o/JelloCrystal.o \
                   o/SharedJelloServer.o

SharedJello: o/JelloAtom.o \
             o/JelloCrystal.o \
             o/JelloRenderer.o \
             o/SharedJello.o
