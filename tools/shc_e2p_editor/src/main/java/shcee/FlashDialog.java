/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2014 Uwe Freese
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

import java.awt.Component;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Properties;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextField;

public class FlashDialog extends JDialog
{
	private static final long serialVersionUID = -5437332123602239488L;
	
	private static String DEFAULT_CMD_WINDOWS = "avrdude.exe -p #DEVICE# -U eeprom:w:#FILENAME#:r";
	private static String DEFAULT_CMD_LINUX = "xterm -hold -e avrdude -p #DEVICE# -U eeprom:w:#FILENAME#:r";
	private static String CFG_FILENAME = "shc_e2p_editor.cfg";
	
	private JTextField textField;
	private int e2pLength;
	private String e2pFilename;
	
	public FlashDialog(int e2pLength, String e2pFilename)
	{
		this.e2pLength = e2pLength;
		this.e2pFilename = e2pFilename;
		
		this.setTitle("Flash e2p file to device");
		
		addComponents();

		loadProperties();
		
		setModal(true);
		pack();
		setLocationRelativeTo(null);
		setResizable(false);
		
		textField.requestFocusInWindow();

		// Add window listener to detect when window is closed.
		// Make sure the listener is added before setVisible is called!
		this.addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
				saveProperties();
            }
		});

		setVisible(true);
	}
	
	private void addComponents()
	{
		JPanel panel = new JPanel();  
		panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));  
		panel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
		
		JLabel l1 = new JLabel("Enter command for flashing e2p file");
		l1.setFont(new Font(l1.getFont().getName(), Font.BOLD, l1.getFont().getSize()));
		l1.setAlignmentX(Component.LEFT_ALIGNMENT);
		panel.add(l1);
		
		panel.add(Box.createRigidArea(new Dimension(0, 10)));
		panel.add(new JLabel("#DEVICE# will be replaced by m328p or m168p according to e2p size"));
		panel.add(new JLabel("#FILENAME# will be replaced by the e2p filename"));
		panel.add(Box.createRigidArea(new Dimension(0, 10)));
		panel.add(new JLabel("Default is: " + (Util.runsOnWindows() ? DEFAULT_CMD_WINDOWS : DEFAULT_CMD_LINUX)));
		panel.add(Box.createRigidArea(new Dimension(0, 10)));

		textField = new JTextField(Util.runsOnWindows() ? DEFAULT_CMD_WINDOWS : DEFAULT_CMD_LINUX);
		textField.setAlignmentX(Component.LEFT_ALIGNMENT);
		panel.add(textField);

		panel.add(Box.createRigidArea(new Dimension(0, 10)));
		
		JButton buttonFlash = new JButton("Flash");
		JButton buttonRestoreDefaults = new JButton("Restore Defaults");
		JButton buttonCancel = new JButton("Cancel");
		
		buttonFlash.setMnemonic('f');
		
		buttonFlash.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e)
            {
                onButtonFlash();
            }
        });
		
		buttonRestoreDefaults.setMnemonic('d');
		
		buttonRestoreDefaults.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e)
            {
                onButtonRestoreDefaults();
            }
        });
		
		buttonCancel.setMnemonic('c');
		
		buttonCancel.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e)
            {
                onButtonCancel();
            }
        });
		
		JPanel buttonPane = new JPanel();
		buttonPane.setLayout(new BoxLayout(buttonPane, BoxLayout.LINE_AXIS));
		//buttonPane.setBorder(BorderFactory.createEmptyBorder(0, 10, 10, 10));
		buttonPane.add(Box.createHorizontalGlue());
		buttonPane.add(buttonFlash);
		buttonPane.add(Box.createRigidArea(new Dimension(10, 0)));
		buttonPane.add(buttonRestoreDefaults);
		buttonPane.add(Box.createRigidArea(new Dimension(10, 0)));
		buttonPane.add(buttonCancel);
		buttonPane.setAlignmentX(Component.LEFT_ALIGNMENT);
		
		panel.add(buttonPane);
		
		this.add(panel);

		getRootPane().setDefaultButton(buttonFlash);
	}
	
	private void onButtonFlash()
	{
		String cmdLineS = textField.getText();
		
		if (e2pLength > 4096)
		{			
			cmdLineS = cmdLineS.replace("#DEVICE#", "m328p");
		}
		else
		{
			cmdLineS = cmdLineS.replace("#DEVICE#", "m168p");	
		}
		
		cmdLineS = cmdLineS.replace("#FILENAME#", "\"" + e2pFilename + "\"");
		
		try
		{
			Util.execute(cmdLineS);
		}
		catch (IOException e)
		{
			JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, "Could not execute program.", "Error", JOptionPane.ERROR_MESSAGE);
			e.printStackTrace();
		}
	}
	
	private void onButtonRestoreDefaults()
	{
		textField.setText(Util.runsOnWindows() ? DEFAULT_CMD_WINDOWS : DEFAULT_CMD_LINUX);
	}

	private void onButtonCancel()
	{
		saveProperties();
		setVisible(false);
	}

	/**
	 * Load parameters from a file.
	 */
	private void loadProperties()
	{
		Properties prop = new Properties();
		InputStream input = null;
	 
		try
		{
			input = new FileInputStream(CFG_FILENAME);
			prop.load(input);
			textField.setText(prop.getProperty("flash_cmd"));
		}
		catch (FileNotFoundException ex)
		{
			// OK - cfg may not exist
		}
		catch (IOException ex)
		{
			ex.printStackTrace();
		}
		finally
		{
			if (input != null)
			{
				try
				{
					input.close();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
		}
	}
	
	/**
	 * Persist parameters to a file.
	 */
	private void saveProperties()
	{
		Properties prop = new Properties();
		OutputStream output = null;

		try
		{
			output = new FileOutputStream(CFG_FILENAME);
			prop.setProperty("flash_cmd", textField.getText());
			prop.store(output, null);	 
		}
		catch (IOException io)
		{
			io.printStackTrace();
		}
		finally
		{
			if (output != null)
			{
				try
				{
					output.close();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
	 
		}
	}
}
