smarthomatic EEPROM Editor
==========================

This editor is used to create or modify configuration settings stored in
the EEPROM of the smarthomatic devices. It needs a Java runtime 1.6+ to
run.

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
