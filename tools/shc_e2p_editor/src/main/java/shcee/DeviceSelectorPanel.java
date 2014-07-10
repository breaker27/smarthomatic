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

import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;


/**
 * DeviceSelectorPanel shows a table with all devices (according to the existing EEPROM files)
 * and has buttons for adding and deleting devices.
 * @author: Uwe Freese
 */

public class DeviceSelectorPanel extends JPanel
{
	private static final long serialVersionUID = 2679223550063036996L;
	
	private DeviceSelectorTable deviceSelectorTable;

	public DeviceSelectorPanel()
	{
		super();

		deviceSelectorTable = new DeviceSelectorTable();
		
		JScrollPane scrollPane = new JScrollPane();
		
		// heading
		JLabel headingLabel = new JLabel("Device List", JLabel.CENTER);
		headingLabel.setFont(new Font(headingLabel.getFont().getName(), Font.BOLD, 14));
		
		JPanel headingPanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 6, 6));
		headingPanel.add(headingLabel);
		
		// buttons
		JButton buttonAddDevice = new JButton("Add Device");
		buttonAddDevice.setMnemonic('a');
		
		buttonAddDevice.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e)
            {
                onButtonAddDevice();
            }
        });
		
		JButton buttonDeleteDevice = new JButton("Delete Device");
		buttonDeleteDevice.setMnemonic('d');
		
		buttonDeleteDevice.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e)
            {
                onButtonDeleteDevice();
            }
        });
		
		JPanel buttonPanel = new JPanel();
		buttonPanel.add(buttonAddDevice);
		buttonPanel.add(buttonDeleteDevice);

		// place heading and buttons to this panel
        setLayout(new java.awt.BorderLayout());
        add(headingPanel, "North");
        add(scrollPane, "Center");
		add(buttonPanel, "South");
		scrollPane.getViewport().add(deviceSelectorTable);
		scrollPane.getVerticalScrollBar().setUnitIncrement(10);
	}

	protected void onButtonDeleteDevice()
	{
		String device = (String)deviceSelectorTable.getValueAt(deviceSelectorTable.getSelectedRow(), 1);
		
		int res = JOptionPane.showConfirmDialog(SHCEEMain.mySHCEEMain, "Really delete device " + device + "?", "Please confirm", JOptionPane.YES_NO_OPTION);
        
		if (res == JOptionPane.YES_OPTION)
		{
			new File(device + ".e2p").delete();
			
			deviceSelectorTable.updateTable();
			
			if (deviceSelectorTable.getRowCount() > 0)
			{
				deviceSelectorTable.setRowSelectionInterval(0, 0);
			}
        }
	}

	/**
	 * Ask user for new device name, create eeprom file and update the table.
	 */
	protected void onButtonAddDevice()
	{
		String filename = JOptionPane.showInputDialog(SHCEEMain.mySHCEEMain, "Enter name of new device (e2p filename w/o extension)", "Enter name", JOptionPane.PLAIN_MESSAGE);
		
		if (null != filename)
		{
			filename = filename.trim();
			
			if (!filename.toLowerCase().endsWith(".e2p"))
			{
				filename += ".e2p";
			}
			
			if (new File(filename).exists())
			{
				JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, "File " + filename + " already exists.", "Error", JOptionPane.ERROR_MESSAGE);
				return;
			}
			
			try
			{
				Util.createBinaryFile(filename, 1024);
			} catch (IOException e)
			{
				JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, "Could not create file " + filename + ".", "Error", JOptionPane.ERROR_MESSAGE);
				e.printStackTrace();
				return;
			}
			
			deviceSelectorTable.updateTable();
			deviceSelectorTable.selectElement(filename.substring(0, filename.length() - 4));
		}
	}
	
	public void updateCurrentRow()
	{
		deviceSelectorTable.updateTableDataForRow(deviceSelectorTable.getSelectedRow());
		deviceSelectorTable.sort();
	}
}
