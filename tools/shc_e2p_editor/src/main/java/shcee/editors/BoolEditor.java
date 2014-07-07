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

package shcee.editors;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JPanel;

import org.w3c.dom.Node;

import shcee.Util;

public class BoolEditor extends AbstractEditor
{
	private static final long serialVersionUID = -7593115068369714873L;

	private int bits;
	private Boolean defaultVal;	
	private BoolArea input;
	
	public BoolEditor(Node root, Color baseColor, int arrayIndex)
	{
		super(root, baseColor, arrayIndex);
		
		bits = 8;
		
		// add label about format
		format = "Boolean of 8 bits";
		
		if (null != defaultVal)
		{
			format += " (default: " + defaultVal + ")";
		}

		addLabel(format);
		
		// add input
		JPanel inputPanel = new JPanel();
		inputPanel.setLayout(new BorderLayout());
		inputPanel.setBackground(this.getBackground());

		input = new BoolArea(defaultVal);
		inputPanel.add(input, BorderLayout.WEST);

		if (null != defaultVal)
		{
			JButton buttonDefault = new JButton("Default");
			
			buttonDefault.addActionListener(new ActionListener() {
	            public void actionPerformed(ActionEvent e)
	            {
	                onButtonDefault();
	            }
	        });

			Component glue = Box.createRigidArea(new Dimension(6, 6));
			glue.setPreferredSize(new Dimension(1000, 6));
			inputPanel.add(glue, BorderLayout.CENTER); // space between components
			inputPanel.add(buttonDefault, BorderLayout.EAST);
		}
		
		add(inputPanel);
		
		// add description
		description = Util.getChildNodeValue(root, "Description");
		addLabel(description);
	}

	private void onButtonDefault()
	{
		input.setSelected(defaultVal);
	}
	
	@Override
	public boolean dataIsValid()
	{
		return input.dataIsValid();
	}

	@Override
	public void setDefinitionParameter(Node n)
	{
		String name = n.getNodeName();
		
		if (name.equals("DefaultVal"))
			defaultVal = n.getFirstChild().getNodeValue().equals("true");
	}

	@Override
	public int readFromEepromArray(byte[] eeprom, int offsetBit)
	{
		int data = Util.getIntFromByteArray(eeprom, offsetBit, bits);

		input.setSelected(data != 0);

		return 8;
	}

	@Override
	public int writeToEepromArray(byte[] eeprom, int offsetBit)
	{
		int value = input.isSelected() ? 1 : 0;		
		Util.setIntInByteArray(value, eeprom, offsetBit, bits);
		return 8;
	}

	@Override
	public String getValue(boolean humanReadable)
	{
		String s = input.getText();
		
		if (Util.isInteger(s)) 
			return "" + Integer.parseInt(s);
		else
			return "0";
	}
}
