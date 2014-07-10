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

import java.awt.Color;
import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JPanel;
import javax.swing.text.DefaultCaret;

import org.w3c.dom.Node;

import shcee.Util;

public class IntEditor extends AbstractEditor
{
	private static final long serialVersionUID = -7593115068369714873L;

	private long minVal;
	private long maxVal;
	private Long defaultVal;
	private int bits;
	private UIntTextArea input;
	
	public IntEditor(Node root, Color baseColor, int arrayIndex)
	{
		super(root, baseColor, arrayIndex);
		
		// add label about format
		format = "Int of " + bits + " bits in the range " + minVal + ".." + maxVal;
		
		if (null != defaultVal)
		{
			format += " (default: " + defaultVal + ")";
		}

		addLabel(format);
		
		// add input
		JPanel inputPanel = new JPanel();
		inputPanel.setLayout(new BoxLayout(inputPanel, javax.swing.BoxLayout.X_AXIS));
		inputPanel.setBackground(this.getBackground());

		input = new UIntTextArea(minVal, maxVal, defaultVal);
		inputPanel.add(input);

		if (null != defaultVal)
		{
			JButton buttonDefault = new JButton("Default");
			
			buttonDefault.addActionListener(new ActionListener() {
	            public void actionPerformed(ActionEvent e)
	            {
	                onButtonDefault();
	            }
	        });

			inputPanel.add(Box.createRigidArea(new Dimension(6, 6))); // space between components
			inputPanel.add(buttonDefault);
		}
		
		add(inputPanel);
		
		// add description
		description = Util.getChildNodeValue(root, "Description");
		addLabel(description);
	}

	private void onButtonDefault()
	{
		input.setText("" + defaultVal);
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
		
		if (name.equals("MinVal"))
			minVal = Integer.parseInt(n.getFirstChild().getNodeValue());
		else if (name.equals("MaxVal"))
			maxVal = Integer.parseInt(n.getFirstChild().getNodeValue());
		else if (name.equals("DefaultVal"))
			defaultVal = new Long(Integer.parseInt(n.getFirstChild().getNodeValue()));
		else if (name.equals("Bits"))
			bits = Integer.parseInt(n.getFirstChild().getNodeValue());
	}

	@Override
	public int readFromEepromArray(byte[] eeprom, int offsetBit)
	{
		int data = Util.getIntFromByteArray(eeprom, offsetBit, bits);
		
		// temporarily avoid scrolling to the element when content changes,
		// then set text
		DefaultCaret caret = (DefaultCaret)input.getCaret();
		caret.setUpdatePolicy(DefaultCaret.NEVER_UPDATE);
		input.setText("" + data);
		caret.setUpdatePolicy(DefaultCaret.ALWAYS_UPDATE);

		return bits;
	}

	@Override
	public int writeToEepromArray(byte[] eeprom, int offsetBit)
	{
		int value = Integer.parseInt(input.getText());		
		Util.setIntInByteArray(value, eeprom, offsetBit, bits);
		return bits;
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
