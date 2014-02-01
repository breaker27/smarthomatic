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
 * UIntTextArea is a JTextArea with a limit on characters that can be entered
 * and automatic input checking.
 * If the input is not correct, the background is red.
 * The validity can be read with isValid().
 * @author uwe
 */
public class UIntTextArea extends JTextArea
{
	private static final long serialVersionUID = -2796855809983284807L;

	private boolean valid;
	private long minVal;
	private long maxVal;
	
	public UIntTextArea(long minVal, long maxVal)
	{
		super();
		
		this.minVal = minVal;
		this.maxVal = maxVal;
		
		PlainDocument d = new TextFieldLimit((int)Math.log10(maxVal) + 1);
		setDocument(d);
		
		this.setText("" + minVal);
		
		d.addDocumentListener(new DocumentListener()
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
		
		valid = false;
	}

	protected void checkInput()
	{
		valid = (Util.isIntegerBetween(getText(), minVal, maxVal));
		
		if (valid)
		{
			setBackground(Color.WHITE);
		}
		else
		{
			setBackground(Color.RED);
		}
	}
	
	public boolean dataIsValid()
	{
		return valid;
	}
}
