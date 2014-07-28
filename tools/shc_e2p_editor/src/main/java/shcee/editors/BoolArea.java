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

package shcee.editors;

import java.awt.Color;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;

import javax.swing.JCheckBox;

/**
 * BoolArea is a JCheckBox which colors its background depending on the
 * selected value in comparison to the default value.
 * @author uwe
 */
public class BoolArea extends JCheckBox
{
	private static final long serialVersionUID = 4158223774920676647L;
	
	private boolean valid;
	private Boolean defaultVal;
	
	public BoolArea(Boolean defaultVal)
	{
		super();
		
		this.defaultVal = defaultVal;
		this.setText("true / active / on");

		this.addItemListener(new ItemListener() {

			@Override
			public void itemStateChanged(ItemEvent arg0) {
				checkInput();
			}
            
        });
		
		checkInput();
	}

	protected void checkInput()
	{
		valid = true;
		
		if ((null != defaultVal) && (defaultVal != this.isSelected()))
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
