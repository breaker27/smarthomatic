/*
* This file is part of smarthomatic, http://www.smarthomatic.com.
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

import javax.swing.text.DefaultCaret;

import shcee.LabelArea;
import shcee.Util;

import org.w3c.dom.Node;

public class ByteArrayEditor extends AbstractEditor
{
	private static final long serialVersionUID = -4736742201423869251L;

	private int bytes;
	private ByteArrayTextArea input;
	
	public ByteArrayEditor(Node root)
	{
		super(root);
		
		// add label about format
		LabelArea formatLabel = new LabelArea("ByteArray of " + bytes + " bytes (use HEX format as input)");
		add(formatLabel);
		
		// add input
		input = new ByteArrayTextArea(bytes);
		add(input);
		
		// add description
		String blockDescription = Util.getChildNodeValue(root, "Description");
		LabelArea descriptionLabel = new LabelArea(blockDescription);
		add(descriptionLabel);
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
		
		if (name.equals("Bytes"))
			bytes = Integer.parseInt(n.getFirstChild().getNodeValue());
	}

	@Override
	public int readFromEepromArray(byte[] eeprom, int offsetBit)
	{
		String s = "";
		
		for (int i = 0; i < bytes; i++)
		{
			byte b = (byte)Util.getUIntFromByteArray(eeprom, offsetBit + i * 8, 8);
			s += Util.byteToHex(b);
		}
		
		// temporarily avoid scrolling to the element when content changes,
		// then set text
		DefaultCaret caret = (DefaultCaret)input.getCaret();
		caret.setUpdatePolicy(DefaultCaret.NEVER_UPDATE);
		input.setText(s);
		caret.setUpdatePolicy(DefaultCaret.ALWAYS_UPDATE);
		
		return bytes * 8;
	}

	@Override
	public int writeToEepromArray(byte[] eeprom, int offsetBit)
	{
		String s = input.getText();
		
		for (int i = 0; i < bytes; i++)
		{
			byte b = Util.hexToByte(s.charAt(i * 2), s.charAt(i * 2 + 1));

			Util.setUIntInByteArray(b, eeprom, offsetBit, 8);
			offsetBit += 8;
		}
		
		return bytes * 8;
	}

	@Override
	public String getValue(boolean humanReadable)
	{
		return input.getText();
	}
}
