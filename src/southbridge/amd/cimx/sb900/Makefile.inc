#
# This file is part of the coreboot project.
#
# Copyright (C) 2011 Advanced Micro Devices, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
#

subdirs-$(CONFIG_SOUTHBRIDGE_AMD_CIMX_SB900) += ../../../../../src/vendorcode/amd/cimx/sb900

# SB900 Platform Files

romstage-y += cfg.c
romstage-y += early.c
romstage-y += smbus.c

ramstage-y += cfg.c
ramstage-y += early.c
ramstage-y += late.c

driver-y   += smbus.c
driver-y   += lpc.c


