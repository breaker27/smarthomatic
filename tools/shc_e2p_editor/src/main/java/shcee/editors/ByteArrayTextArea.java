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

import javax.swing.JTextArea;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.text.PlainDocument;
import shcee.Util;

/**
 * ByteArrayTextArea is a JTextArea with a limit on characters that can be entered
 * and automatic input checking.
 * If the input is not correct, the background is red.
 * The validity can be read with isValid().
 * @author uwe
 */
public class ByteArrayTextArea extends JTextArea
{
	private static final long serialVersionUID = -2796855809983284807L;

	private boolean valid;
	private int bytes;
	private String defaultVal;
	private boolean hexMode = true;
	private TextFieldLimit doc;
	
	public ByteArrayTextArea(int bytes, String defaultVal)
	{
		super();
		
		this.bytes = bytes;
		this.defaultVal = defaultVal;
		
		doc = new TextFieldLimit(bytes * 2);
		setDocument(doc);

		this.setText(Util.fillString('0', bytes * 2));

		doc.addDocumentListener(new DocumentListener()
		{
			@Override
			public void changedUpdate(DocumentEvent e)
			{
				checkInput();
			}

			@Override
			public void insertUpdate(DocumentEvent e)
			{
				checkInput();
			}

			@Override
			public void removeUpdate(DocumentEvent e)
			{
				checkInput();	
			}
	    });
		
		setLineWrap(true);

		checkInput();
	}

	/**
	 * Toggle between hex and string mode.
	 */
	public void toggleMode()
	{
		hexMode = !hexMode;
		
		if (hexMode) // string -> hex
		{
			String s = getText();
			
			doc.setLimit(bytes * 2);
			
			StringBuilder sb = new StringBuilder();
			
			for (int i = 0; i < bytes; i++)
			{
				if (s.length() > i)
				{
					sb.append(Util.byteToHex((byte)s.charAt(i)));
				}
			}
			
			setText(sb.toString());
		}
		else // hex -> string
		{
			String s = getText();
			
			byte[] buffer = new byte[bytes];
			
			for (int i = 0; i < bytes; i++)
			{
				if (s.length() > i * 2 + 1)
				{
					
					buffer[i] = Util.hexToByte(s.charAt(i * 2), s.charAt(i * 2 + 1));
				}
			}
			
			doc.setLimit(bytes);

			setText(new String(buffer));
		}
	}
	
	protected void checkInput()
	{
		valid = (hexMode && (getText().length() == bytes * 2) && Util.isHexString(getText()))
				|| (!hexMode && (getText().length() == bytes));
		
		if (!valid)
		{
			setBackground(Color.RED);
		}
		else if ((null != defaultVal) && (!defaultVal.equals(getText())))
		{
			setBackground(Color.YELLOW);
		}
		else
		{
			setBackground(Color.WHITE);
		}
	}
	
	public boolean dataIsValid()
	{
		return valid;
	}
}
