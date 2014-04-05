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

import javax.swing.text.DefaultCaret;

import org.w3c.dom.Node;

import shcee.Util;

public class BoolEditor extends AbstractEditor
{
	private static final long serialVersionUID = -7593115068369714873L;

	private long minVal;
	private long maxVal;
	private int bits;
	private UIntTextArea input;
	
	public BoolEditor(Node root, Color baseColor, int arrayIndex)
	{
		super(root, baseColor, arrayIndex);
		
		minVal = 0;
		maxVal = 1;
		bits = 8;
		
		// add label about format
		format = "Boolean of 8 bits";
		addLabel(format);
		
		// add input
		input = new UIntTextArea(minVal, maxVal);
		add(input);
		
		// add description
		description = Util.getChildNodeValue(root, "Description");
		addLabel(description);
	}

	@Override
	public boolean dataIsValid()
	{
		return input.dataIsValid();
	}

	@Override
	public void setDefinitionParameter(Node n)
	{
		// no parameters
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

		return 8;
	}

	@Override
	public int writeToEepromArray(byte[] eeprom, int offsetBit)
	{
		int value = Integer.parseInt(input.getText());		
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
