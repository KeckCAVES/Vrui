#!/bin/bash
########################################################################
# Script to remove a set of files from a directory, and then remove the
# directory if it has become empty.
# Copyright (c) 2015 Oliver Kreylos
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

########################################################################
# Script argument list:
########################################################################

# $1 - name of directory containing the set of files
# $2-$n - names of files to remove

# Get the name of the directory:
DIR="$1"
shift

if [ -d "$DIR" ]; then
	# Process and remove command line arguments one at a time:
	while [ $# -gt 0 ]; do
		# Strip path from file name:
		FILE="$(basename $1)"
		rm "$DIR/$FILE" 2>/dev/null
		shift
	done
	
	# Remove the directory if it is empty:
	rmdir "$DIR" 2>/dev/null
fi

exit 0
