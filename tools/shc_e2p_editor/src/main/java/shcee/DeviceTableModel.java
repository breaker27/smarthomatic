/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2013 Uwe Freese
*
* smarthomatic is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version.
*
* smarthomatic is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with smarthomatic. If not, see <http://www.gnu.org/licenses/>.
*/

package shcee;

import javax.swing.table.DefaultTableModel;
 
/**
 * @author: Uwe Freese
 */

public class DeviceTableModel extends DefaultTableModel {
	
	private static final long serialVersionUID = 814749968441025062L;
	
	public DeviceTableModel(Object[][] data, String[] columnNames)
	{
		super(data, columnNames);
	}

	public DeviceTableModel()
	{
		super();
	}

	@Override  
    public Class<?> getColumnClass(int col) {  
		if (col == 0)
		{
			return String.class;
		}
		else
		{
			return String.class;  //other columns accept String values
		}
	}  

	@Override  
	public boolean isCellEditable(int row, int col) {  
		return false;
	}
}
