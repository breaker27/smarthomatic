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
import java.util.ArrayList;

import javax.swing.border.TitledBorder;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import shcee.LabelArea;

public class ArrayEditor extends AbstractEditor
{
	private static final long serialVersionUID = -7593115068369714873L;

	private ArrayList<AbstractEditor> editors;
	private int length;
	
	public ArrayEditor(Node root, Color baseColor, int arrayIndex)
	{
		super(root, baseColor, arrayIndex);
		
		editors = new ArrayList<AbstractEditor>();
			
		// add editors
		NodeList nl = root.getChildNodes();
		Node elementNode = nl.item(3);
		
		for (int i = 0; i < length; i++)
		{
			if (elementNode.getNodeName().equals("UIntValue"))
				addElem(new UIntEditor(elementNode, this.getBackground(), i));
			else if (elementNode.getNodeName().equals("IntValue"))
				addElem(new IntEditor(elementNode, this.getBackground(), i));
			else if (elementNode.getNodeName().equals("BoolValue"))
				addElem(new BoolEditor(elementNode, this.getBackground(), i));
			else if (elementNode.getNodeName().equals("EnumValue"))
				addElem(new EnumEditor(elementNode, this.getBackground(), i));
			else if (elementNode.getNodeName().equals("ByteArray"))
				addElem(new ByteArrayEditor(elementNode, this.getBackground(), i));
		}
		
		if (length > 0)
		{
			// add array length
			format = "Array of " + length + " elements of " + editors.get(0).format;
			LabelArea lengthLabel = new LabelArea(format);
			add(lengthLabel, 0);
			
			description = editors.get(0).description;		
			LabelArea descriptionLabel = new LabelArea(description);
			add(descriptionLabel, 1);
			
			TitledBorder border = (TitledBorder)getBorder();
			border.setTitle(editors.get(0).id + " (Array)");
		}
	}

	private void addElem(AbstractEditor c)
	{
		//add(Box.createRigidArea(new Dimension(10, 10))); // space between components
		add(c);	
		editors.add(c);
	}
	
	@Override
	public boolean dataIsValid()
	{
		for (int i = 0; i < editors.size(); i++)
		{
			if (!editors.get(i).isValid())
			{
				return false;
			}
		}
		
		return true;
	}

	@Override
	public void setDefinitionParameter(Node n)
	{
		if (n.getNodeName().equals("Length"))
			length = Integer.parseInt(n.getFirstChild().getNodeValue());
	}

	@Override
	public int readFromEepromArray(byte[] eeprom, int offsetBit)
	{
		int bits = 0;
		
		for (int i = 0; i < editors.size(); i++)
		{
			bits += editors.get(i).readFromEepromArray(eeprom, offsetBit + bits);
		}
		
		return bits;
	}

	@Override
	public int writeToEepromArray(byte[] eeprom, int offsetBit)
	{
		int bits = 0;
		
		for (int i = 0; i < editors.size(); i++)
		{
			bits += editors.get(i).writeToEepromArray(eeprom, offsetBit + bits);
		}
		
		return bits;
	}

	@Override
	public String getValue(boolean humanReadable)
	{
		return "ARRAY";
	}
}
