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
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;
import java.util.HashMap;

import javax.swing.JComboBox;

import shcee.LabelArea;
import shcee.SHCEEMain;
import shcee.Util;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class EnumEditor extends AbstractEditor
{
	private static final long serialVersionUID = -7583780384335242111L;

	private int bits;
	private ArrayList<Integer> itemIndex2enumValue; // store ENUM values additionally for quick access 
	private HashMap<Integer, Integer> enumValue2itemIndex;
	private JComboBox input;
	
	public EnumEditor(Node root, Color baseColor, int arrayIndex)
	{
		super(root, baseColor, arrayIndex);
		
		// add label about format
		format = "Enum value of " + bits + " bits";
		addLabel(format);
		
		// add input
		add(input);
		
		// add description
		description = Util.getChildNodeValue(root, "Description");
		addLabel(description);
		
		// add listener for reacting on changed DeviceType (in case this editor is the one)
		input.addActionListener (new ActionListener () {
		    public void actionPerformed(ActionEvent e) {
		        onChangeValue();
		    }
		});
	}

	protected void onChangeValue()
	{
		// If the DeviceType is changes, the layout of the ValueEditorPanel has to be changed, because
		// different DeviceTypes have different values to edit.
		if (id.equals("DeviceType"))
		{
			SHCEEMain.mySHCEEMain.valueEditor.updateBlockVisibility();
		}
	}

	@Override
	public boolean dataIsValid()
	{
		// nothing invalid can be selected, so return true
		return true;
	}

	@Override
	public int readFromEepromArray(byte[] eeprom, int offsetBit)
	{
		int enumValue = Util.getUIntFromByteArray(eeprom, offsetBit, bits);
		int index = 0;
		
		if (enumValue2itemIndex.containsKey(enumValue))
		{
			index = enumValue2itemIndex.get(enumValue);
		}

		input.setSelectedIndex(index);
		return bits;
	}

	@Override
	public void setDefinitionParameter(Node n)
	{
		if (null == itemIndex2enumValue)
		{
			itemIndex2enumValue = new ArrayList<Integer>();
			enumValue2itemIndex = new HashMap<Integer, Integer>();
			input = new JComboBox();
		}
		
		String name = n.getNodeName();
		
		if (name.equals("Bits"))
			bits = Integer.parseInt(n.getFirstChild().getNodeValue());
		else if (name.equals("Element"))
		{
			NodeList nl = n.getChildNodes();
			String v = "";
			
			for (int i = 0; i < nl.getLength(); i++)
			{
				Node n2 = nl.item(i);
				String name2 = n2.getNodeName();
				
				if (name2.equals("Value"))
					v = n2.getFirstChild().getNodeValue();
				else if (name2.equals("Name"))
				{
					input.addItem(n2.getFirstChild().getNodeValue() + " (" + v + ")");
					itemIndex2enumValue.add(Integer.parseInt(v));
					enumValue2itemIndex.put(Integer.parseInt(v), input.getItemCount() - 1);
				}
			}
		}
	}

	@Override
	public int writeToEepromArray(byte[] eeprom, int offsetBit)
	{
		int value = itemIndex2enumValue.get(input.getSelectedIndex());		
		Util.setUIntInByteArray(value, eeprom, offsetBit, bits);
		return bits;
	}

	@Override
	public String getValue(boolean humanReadable)
	{
		if (humanReadable)
			return (String)input.getSelectedItem();
		else
			return "" + itemIndex2enumValue.get(input.getSelectedIndex());
	}
	
	/**
	 * Return all integer values possible at this editor.
	 */
	public ArrayList<Integer> getValues()
	{
		return itemIndex2enumValue;
	}
}
