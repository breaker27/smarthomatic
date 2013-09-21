smarthomatic EEPROM Editor
==========================

This program is part of smarthomatic, http://www.smarthomatic.com.
Copyright (c) 2013 Uwe Freese

This editor is used to create or modify configuration settings stored in
the EEPROM of the smarthomatic devices. It needs a Java runtime 1.6+ to
run.

smarthomatic is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

smarthomatic is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along
with smarthomatic. If not, see <http://www.gnu.org/licenses/>.

Start
-----

Click on the *.jar file or start the editor with the following command:
java -jar {editor}.jar

Usage
-----

Please read the documentation on the smarthomatic website at
http://www.smarthomatic.com.

Design Principles
-----------------

The editor GUI is created dynamically out of the e2p_layout.xml. Swing
components for editing data must not be added statically. This is to allow
changes in the EEPROM layout without the need to modify the editor.

For every data type, there is an "editor" component (derived from JPanel).
This element can read and write data from an EEPROM binary file and holds
the data for editing.

There is no distinction between data classes and GUI (editor) classes for
the data elements for simplification. Getting a value from EEPROM
therefore makes it necessary to load it into the Swing components. This
may be not optimal from a speed perspective, but nontheless this is no
problem, because it's quick enough. The advantage of not having additional
classes for data storage is more important.
