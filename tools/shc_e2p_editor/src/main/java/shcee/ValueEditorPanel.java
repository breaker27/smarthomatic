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

import java.awt.Color;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.SwingUtilities;

import shcee.editors.Block;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

/**
 * The ValueEditorPanel consists of a ScollPane, which contains a ValueEditorBlockContainer,
 * which finally holds the EEPROM block panels. It can be updates with data from an EEPROM file.
 * To store the dta back in an EEPROM file, the content can be written back
 * to a byte array.
 * 
 * @author uwe
 */
public class ValueEditorPanel extends JPanel
{
	private static final long serialVersionUID = -2750864278619672384L;

	private static int[] blockColors = { 0xFFD0D0, 0xD0FFD0, 0xD0D0FF, 0xFFFFA0, 0xA0FFFF, 0xFFA0FF };
	private int colorIndex = 0;
	
	private String filename = "";
	
	private JPanel blockContainer;
	private ArrayList<Block> blocks;
	private JScrollPane scrollPane;
	
	private Node xmlRoot;
	private int length = 0;
	
	public ValueEditorPanel()
	{
		super();
		
		blockContainer = new ValueEditorBlockContainer(); 
		blocks = new ArrayList<Block>();
		
		scrollPane = new JScrollPane(blockContainer,
	            JScrollPane.VERTICAL_SCROLLBAR_ALWAYS,
	            JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
				
		// create heading
		JLabel headingLabel = new JLabel("Value Editor", JLabel.CENTER);
		headingLabel.setFont(new Font(headingLabel.getFont().getName(), Font.BOLD, 14));
		
		JPanel headingPanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 6, 6));
		headingPanel.add(headingLabel);

		// create buttons
		JButton buttonSave = new JButton("Save");
		buttonSave.setMnemonic('s');
		
		buttonSave.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e)
            {
                onButtonSave();
            }
        });
		
		JButton buttonFlash = new JButton("Flash");
		buttonFlash.setMnemonic('f');
		
		buttonFlash.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e)
            {
                onButtonFlash();
            }
        });
		
		JButton buttonAbout = new JButton("About");
		buttonAbout.setMnemonic('a');
		
		buttonAbout.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e)
            {
                onButtonAbout();
            }
        });

		// add buttons to panel
		JPanel innerButtonPanel = new JPanel();
		innerButtonPanel.add(buttonSave);
		innerButtonPanel.add(buttonFlash);

		JPanel aboutButtonPanel = new JPanel();
		aboutButtonPanel.add(buttonAbout);

		JPanel buttonPanel = new JPanel();
		buttonPanel.setLayout(new java.awt.BorderLayout());
		buttonPanel.add(innerButtonPanel, "Center");
		buttonPanel.add(aboutButtonPanel, "East");
		
		setLayout(new java.awt.BorderLayout());
		add(headingPanel, "North");
		add(scrollPane, "Center");
        add(buttonPanel, "South");
		
		initAccordingEepromLayout();
	}
	
	protected void onButtonAbout()
	{
		JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain,
				"<html><body><b>smarthomatic EEPROM Editor</b><br/>" +
				"http://www.smarthomatic.org<br/>" +
				"Copyright (c) 2013 Uwe Freese<br/>" +
				"Licensed under GLP 3 or later.</body></html>", "About", JOptionPane.INFORMATION_MESSAGE);
	}

	protected void onButtonFlash()
	{
		String oldCmd = "avrdude.exe -p #DEVICE# -U eeprom:w:#FILENAME#:r";
		
		Object cmdLine = JOptionPane.showInputDialog(SHCEEMain.mySHCEEMain,
				"<html><body><b>Enter command for flashing e2p file</b><br/><br/>" +
				"#DEVICE# will be replaced by m328p or m168 according to e2p size<br/>" +
				"#FILENAME# will be replaced by the e2p filename<br/><br/>" +
				"Default is: avrdude.exe -p #DEVICE# -U eeprom:w:#FILENAME#:r</body></html>", "Flash e2p file to device", JOptionPane.PLAIN_MESSAGE, null, null, oldCmd);
		
		if (null != cmdLine)
		{
			String cmdLineS = (String)cmdLine;
			
			if (length > 4096)
			{			
				cmdLineS = cmdLineS.replace("#DEVICE#", "m328p");
			}
			else
			{
				cmdLineS = cmdLineS.replace("#DEVICE#", "m168");	
			}
			
			cmdLineS = cmdLineS.replace("#FILENAME#", "\"" + filename + "\"");
			
			try
			{
				// TODO: Support linux by calling bash in a way that the user can see the output.
				// The user could choose to run bash by prefixing the command line to enter with
				// e.g. "LINUX", so we can live with the simple prompt consisting only
				// of one line of text.
				Util.execute(cmdLineS, false);
			} catch (IOException e)
			{
				JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, "Could not execute program.", "Error", JOptionPane.ERROR_MESSAGE);
				e.printStackTrace();
			}
		}
	}

	protected void onButtonSave()
	{
		if (dataIsValid())
		{
			writeToEepromFile();
			SHCEEMain.mySHCEEMain.deviceSelector.updateCurrentRow();
		}
		else
		{
			JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, "Some data is invalid. Fix it before saving.", "Invalid Data", JOptionPane.ERROR_MESSAGE);
		}
	}

	/**
	 * Read the e2p_layout.xml file and create the editor GUI according to it.
	 */
	private void initAccordingEepromLayout()
	{
		String errMsg = Util.conformsToSchema(SHCEEMain.EEPROM_LAYOUT_XML, SHCEEMain.EEPROM_METAMODEL_XSD);
		
		if (null != errMsg)
		{
			JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, SHCEEMain.EEPROM_LAYOUT_XML + " does not conform to " +
					SHCEEMain.EEPROM_METAMODEL_XSD + ".\nError message:\n" + errMsg , "Error", JOptionPane.ERROR_MESSAGE);
			return;
		}
		
		try
		{
			xmlRoot = Util.readXML(new File(SHCEEMain.EEPROM_LAYOUT_XML));
		}
		catch (Exception e)
		{
			e.printStackTrace();
			JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, SHCEEMain.EEPROM_LAYOUT_XML + " could not be loaded", "Error", JOptionPane.ERROR_MESSAGE);
			return;
		}
		
		xmlRoot = Util.getChildNode(xmlRoot, "E2P");
		
		NodeList nl = xmlRoot.getChildNodes();
		
		for (int i = 0; i < nl.getLength(); i++)
		{
			Node n = nl.item(i);
			
			if (n.getNodeName().equals("Block"))
				addBlock(n);
			// other elements can't occur according to the current metamodel
			// no check here
		}
	}

	private void addBlock(Node n)
	{
		Block b = new Block(new Color(blockColors[colorIndex]), n);
		colorIndex = (colorIndex + 1) % blockColors.length;
		blockContainer.add(b);
		blocks.add(b);
	}

	/**
	 * Show the content of the given EEPROM file for editing.
	 * @param valueAt
	 */
	public void loadE2pFile(String filename)
	{
		final Rectangle rect = scrollPane.getVisibleRect();

		// Only update on change. Sometimes several table row select events are fired
		// with the same filename.
		if (!this.filename.equals(filename))
		{
			try
			{
				byte[] data = Util.readFileToByteArray(filename);
				readFromEepromArray(data);
			}
			catch (Exception e)
			{
				e.printStackTrace();
				JOptionPane.showMessageDialog(null, "The file " + filename
						+ " could not be read into a byte array.", "Cannot read file",
						JOptionPane.ERROR_MESSAGE);
			}
			
			this.filename = filename;
		}

	   SwingUtilities.invokeLater(new Runnable() {
	      public void run() {
	         scrollPane.scrollRectToVisible(rect);
	      }
	   });
	}

	/**
	 * Feed the bytes from the EEPROM file to the editor blocks one after another
	 * with an updated bit offset.
	 * The block objects are responsible for further feeding the bits and bytes
	 * to their contained editor components.
	 * @param data
	 */
	private void readFromEepromArray(byte[] eeprom)
	{
		int offset = 0;

		// Switch all blocks of so no blocks are displayed that are not used for the device if anything goes wrong reading the bytes from the file (e.g. file too short).
		for (Block b : blocks)
		{
			b.setVisible(false);
		}
		
		for (Block b : blocks)
		{
			boolean match = true;
			
			// If a block has a condition set, check it and override
			// using this block if it is not matched.
			if (b.restrictionRefID != null)
			{
				String val = findValue(b.restrictionRefID, false);

				match = val.equals(b.restrictionValue);
			}

			b.setVisible(match);
			
			if (match)
			{
				int bitsProcessed = b.readFromEepromArray(eeprom, offset);
				offset += bitsProcessed;
			}
		}
		
		length = offset;
	}
	
	/**
	 * Let all editors save their content to a byte array and write it to a file.
	 * @param data
	 */
	private void writeToEepromFile()
	{
		byte[] eeprom = new byte[1024]; // assume size of 1024 bytes first and truncate later
		int offset = 0;

		for (Block b : blocks)
		{
			if (b.isVisible())
			{
				int bitsProcessed = b.writeToEepromArray(eeprom, offset);
				offset += bitsProcessed;
			}
		}
		
		// cut size to 512 or 1024 bytes automatically
		if (offset <= 512 * 8)
		{
			byte[] eeprom2 = new byte[512];
			System.arraycopy(eeprom, 0, eeprom2, 0, 512);
			eeprom = eeprom2;
		}
		
		try
		{
			Util.writeFile(filename, eeprom);
		} catch (IOException e)
		{
			JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, "Could not write file " + filename + ".", "Error", JOptionPane.ERROR_MESSAGE);
			e.printStackTrace();
		}
	}

	/**
	 * Go through all blocks and editors searching for a value with the
	 * given name and return a string representation.
	 * @param id The ID of the value (according to the e2p_layout.xml).
	 * @param humanReadable If TRUE, return a better readable string
	 *        representation only used for the GUI.
	 * @return The value as String, or null if not found.
	 */
	public String findValue(String id, boolean humanReadable)
	{	
		for (Block b : blocks)
		{
			String val = b.findValue(id, humanReadable);
			
			if (null != val)
				return val;
		}
		
		return null;
	}

	/**
	 * Check if the data in all contained blocks is valid.
	 * Ignore blocks that are not visible (not relevant).
	 * @return
	 */
	private boolean dataIsValid()
	{
		for (Block b : blocks)
		{
			if (b.isVisible() && !b.dataIsValid())
				return false;
		}
		
		return true;
	}
	
	/**
	 * Assume that blocks are restricted by the DeviceType only
	 * (as true for smarthomatic) and switch the blocks on/off
	 * using the given deviceTypeID.
	 * This is for use for the Firmware source code creation process ONLY!
	 * @param deviceTypeID
	 */
	public void enableEditorsForDeviceType(int deviceTypeID)
	{
		byte[] dummy = new byte[1024];
		int offset = 0;
		
		for (Block b : blocks)
		{
			boolean match = true;
			
			// If a block has a condition set, check it and override
			// using this block if it is not matched.
			if ((b.restrictionRefID != null) && b.restrictionRefID.equals("DeviceType"))
			{
				match = b.restrictionValue.equals("" + deviceTypeID);
			}

			b.setVisible(match);
			
			if (match)
			{
				int bitsProcessed = b.readFromEepromArray(dummy, offset);
				offset += bitsProcessed;
			}
		}
	}
}
