#  FindGNOME2.cmake
#  
#  Copyright 2023 Unknown <rpmbuild@fedora14-vmware.vm>
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  


set(GNOME_LIBS gnome-1.2 gnomeui-1.2 ORBit-1.2 gtkxmhtml gnorba-1.2 zvt)
pkg_check_modules(GNOME12 REQUIRED ${GNOME_LIBS})
