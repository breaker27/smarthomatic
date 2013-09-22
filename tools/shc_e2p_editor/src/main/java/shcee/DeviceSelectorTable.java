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

import java.io.File;
import java.util.Comparator;

import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.SwingConstants;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;
import javax.swing.table.TableModel;
import javax.swing.table.TableRowSorter;

public class DeviceSelectorTable extends JTable
{
	private static final long serialVersionUID = 3772504528864774739L;
	private DefaultTableModel model;
	private TableRowSorter<TableModel> sorter;
	
	public DeviceSelectorTable()
	{
		super();

		// setup model
        model = new DeviceTableModel();
        model.setColumnCount(3);
		model.setColumnIdentifiers(new String[] { "DeviceID", "Name", "DeviceType" });
        setModel(model);
        
        this.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
            public void valueChanged(ListSelectionEvent event) {
            	onRowSelected();
            }
        });

		setRowSelectionAllowed(true);
	    setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        // setup columns
        TableColumn colDeviceID = getColumnModel().getColumn(0);
        colDeviceID.setMinWidth(60);
        colDeviceID.setMaxWidth(60);

	    // setup TableRowSorter
         sorter = new TableRowSorter<TableModel>();
        this.setRowSorter(sorter);
        sorter.setModel(model);

        sorter.setComparator(0, new Comparator<Object>()
		{
			@Override
			public int compare(Object o1, Object o2)
			{
				return Integer.parseInt(o1.toString()) - (Integer.parseInt(o2.toString()));
			}
		});

	    // set headings centered
        ((DefaultTableCellRenderer)this.getTableHeader().getDefaultRenderer()).setHorizontalAlignment(SwingConstants.CENTER);

        // disable column reordering
        this.getTableHeader().setReorderingAllowed(false);
        
        // set some columns to centered
        DefaultTableCellRenderer centerRenderer = new DefaultTableCellRenderer();
        centerRenderer.setHorizontalAlignment(SwingConstants.CENTER);
        this.getColumnModel().getColumn(0).setCellRenderer(centerRenderer);

        // load data
	    updateTable();
	    
        // sort table and select first row
	    sorter.toggleSortOrder(0);
	    
	    if (this.getRowCount() > 0)
	    {
	        this.setRowSelectionInterval(0, 0);
	    }
	}

	/**
	 * Add a table row with the given data.
	 * @param deviceID
	 * @param name
	 * @param typeID
	 */
	private void addRow(int deviceID, String name, String typeID)
	{
		Object[] rowData = new Object[] {deviceID, name, typeID};
		
		model.addRow(rowData);
	}
	
	/**
	 * Look for *.xml files in the program directory and insert devices which files conform to
	 * the schema (xsd) to the table.
	 */
	public void updateTable()
	{
		model.setRowCount(0);
		
		File d = new File(".");

		if (d.isDirectory())
		{
			File[] filesAndDirs = d.listFiles();
			
		    for (int i = 0; i < filesAndDirs.length; i++)
		    {
		    	if (filesAndDirs[i].isFile()
		    			&& filesAndDirs[i].getName().toLowerCase().endsWith(".e2p"))
	    		{
		    		long length = filesAndDirs[i].length();
		    		
		    		if ((length == 512) || (length == 1024))
		    		{
						String name = filesAndDirs[i].getName();
						name = name.substring(0, name.length() - 4);
						
						addRow(0, name, "");
		    		}
	    		}
		    }
		}
		
		for (int i = 0; i < model.getRowCount(); i++)
		{
			updateTableDataForRow(i);
		}
		
		sort();
	}
	
	/**
	 * (Re)sort the table by the currently selected sort order.
	 */
	public void sort()
	{
		// call sorter.sort manually after updating instead of using
		// sorter.setSortsOnUpdates(true);, because the table would be resorted
		// while the different cells of one row would be updated!
		sorter.sort();
	}
	
	/**
	 * Load device data on right side panel for editing.
	 * @param firstIndex
	 */
	protected void onRowSelected()
	{
		int row = getSelectedRow();
		
		if (row != -1) // avoid problems with empty table
		{
			SHCEEMain.mySHCEEMain.loadE2pFile((String)getValueAt(getSelectedRow(), 1) + ".e2p");
		}
	}

	/**
	 * Read one binary EEPROM files into the editors and get some values to
	 * display in the cells of the given row.
	 * @param row The table row to update.
	 */
	public void updateTableDataForRow(int row)
	{
		String filename = (String)getValueAt(row, 1) + ".e2p";
		
		SHCEEMain.mySHCEEMain.loadE2pFile(filename);
		
		ValueEditorPanel ed = SHCEEMain.mySHCEEMain.valueEditor;

		String deviceID = ed.findValue("DeviceID", true);
		String deviceType = ed.findValue("DeviceType", true);

		setValueAt(deviceID, row, 0);
		setValueAt(deviceType, row, 2);
	}

	/**
	 * Search through the table and select the row with the given device name.
	 * If not found, do nothing.
	 * @param devicename The name of the device to search for.
	 */
	public void selectElement(String devicename)
	{
		for (int i = 0; i < model.getRowCount(); i++)
		{
			if (((String)getValueAt(i, 1)).equals(devicename))
			{
				this.setRowSelectionInterval(i, i);
				return;
			}
		}
	}
}
