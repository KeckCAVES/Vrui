########################################################################
# Makefile template for inter-platform development with automatic
# source dependency maintenance.
# Copyright (c) 1999-2006 Oliver Kreylos
#
# This file is part of the WhyTools Build Environment.
# 
# The WhyTools Build Environment is free software; you can redistribute
# it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# The WhyTools Build Environment is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
# of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with the WhyTools Build Environment; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
########################################################################

# Include definitions for the system environment
include $(VRUIPACKAGEROOT)/BuildRoot/SystemDefinitions

# Specify default optimization/debug level
ifdef DEBUG
  DEFAULTDEBUGLEVEL = 2
  DEFAULTOPTLEVEL = 0
else
  DEFAULTDEBUGLEVEL = 0
  DEFAULTOPTLEVEL = 3
endif

########################################################################
# Specifiy additional compiler and linker flags
########################################################################

########################################################################
# List packages used by this project
# (Supported packages can be found in $(BUILDROOT)/Packages)
########################################################################

PACKAGES = 

########################################################################
# Specify all final targets
########################################################################

ALL = 

.PHONY: all
all: $(ALL)

########################################################################
# Specify other actions to be performed on a `make clean'
########################################################################

.PHONY: extraclean
extraclean:

.PHONY: extrasqueakyclean
extrasqueakyclean:

# Include basic makefile
include $(VRUIPACKAGEROOT)/BuildRoot/BasicMakefile

########################################################################
# Specify rules for all non-C/C++ sources
########################################################################

########################################################################
# Specify extra flags for all source files that need them
########################################################################

########################################################################
# Specify build rules for dynamic shared objects
########################################################################

########################################################################
# Specify build rules for executables
########################################################################
