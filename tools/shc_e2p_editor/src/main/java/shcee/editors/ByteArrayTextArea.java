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
	 * Convert normal text string (e.g. "hello") to a hex string (e.g. "68656C6C6F").
	 * @return
	 */
	public String textToHex(String text)
	{
		StringBuilder sb = new StringBuilder();
		
		for (int i = 0; i < bytes; i++)
		{
			if (text.length() > i)
			{
				sb.append(Util.byteToHex((byte)text.charAt(i)));
			}
		}
		
		return sb.toString();
	}
	
	/**
	 * Set hex or string mode.
	 */
	public void setMode(boolean hexMode)
	{
		this.hexMode = hexMode;
		
		if (hexMode) // string -> hex
		{
			doc.setLimit(bytes * 2);
			setText(textToHex(getText()));
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
			
			setText(new String(buffer));
			doc.setLimit(bytes);
		}
	}
	
	/**
	 * Toggle between hex and string mode.
	 */
	public void toggleMode()
	{
		setMode(!hexMode);
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
	
	public byte[] getBytes()
	{
		byte[] res = new byte[bytes];
		String s;
		
		if (hexMode)
		{
			s = getText();
		}
		else
		{
			s = textToHex(getText());
		}
		
		for (int i = 0; i < bytes; i++)
		{
			res[i] = Util.hexToByte(s.charAt(i * 2), s.charAt(i * 2 + 1));
		}
		
		return res;
	}
}
