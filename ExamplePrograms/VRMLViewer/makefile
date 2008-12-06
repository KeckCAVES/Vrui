########################################################################
# Makefile for the Virtual Reality VRML viewer.
# source dependency maintenance.
# Copyright (c) 2000-2008 Oliver Kreylos
#
# This file is part of the Virtual Reality VRML viewer (VRMLViewer).
#
# The Virtual Reality VRML viewer is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# The Virtual Reality VRML viewer is distributed in the hope that it
# will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with the Virtual Reality VRML viewer; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
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
  CFLAGS += -g0 -O3 -DNDEBUG
endif

# Pattern rules to compile C++ sources:
o/%.o: %.cpp
	@mkdir -p o/$(*D)
	@echo Compiling $<...
	@g++ -c -o $@ $(VRUI_CFLAGS) $(CFLAGS) $<

# Pattern rule to link executables:
exe/%:
	@mkdir -p exe
	@echo Linking $@...
	@g++ -o $@ $^ $(VRUI_LINKFLAGS)

ALL = exe/VRMLViewer

.PHONY: all
all: $(ALL)

.PHONY: clean
clean:
	-rm -rf o exe

FIELD_SOURCES = Fields/SFBool.cpp \
                Fields/SFInt32.cpp \
                Fields/MFInt32.cpp \
                Fields/SFFloat.cpp \
                Fields/MFFloat.cpp \
                Fields/SFString.cpp \
                Fields/MFString.cpp \
                Fields/SFColor.cpp \
                Fields/MFColor.cpp \
                Fields/SFVec2f.cpp \
                Fields/SFVec3f.cpp \
                Fields/SFRotation.cpp \
                Fields/MFRotation.cpp

NODE_SOURCES = VRMLNode.cpp \
               GroupNode.cpp \
               RootNode.cpp \
               AnchorNode.cpp \
               BillboardNode.cpp \
               CollisionNode.cpp \
               TransformNode.cpp \
               ShapeNode.cpp \
               AppearanceNode.cpp \
               AttributeNode.cpp \
               MaterialNode.cpp \
               ImageTextureNode.cpp \
               TextureTransformNode.cpp \
               ColorInterpolatorNode.cpp \
               BoxNode.cpp \
               CylinderNode.cpp \
               ConeNode.cpp \
               SphereNode.cpp \
               FontStyleNode.cpp \
               TextNode.cpp \
               EllipsoidNode.cpp \
               SpaceSeparatedPointSetReaderNode.cpp \
               FormattedPointSetReaderNode.cpp \
               PointSetNode.cpp \
               ArcInfoExportFileIndexedLineSetReaderNode.cpp \
               IndexedLineSetNode.cpp \
               IndexedFaceSetNode.cpp \
               TextureCoordinateNode.cpp \
               ColorNode.cpp \
               NormalNode.cpp \
               CoordinateNode.cpp \
               LODNode.cpp \
               InlineNode.cpp \
               EarthModelNode.cpp \
               VRMLParser.cpp \
               VRMLViewer.cpp


exe/VRMLViewer: $(FIELD_SOURCES:%.cpp=o/%.o) \
                $(NODE_SOURCES:%.cpp=o/%.o)
