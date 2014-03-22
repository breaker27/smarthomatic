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
import java.awt.Font;
import java.awt.LayoutManager;
import java.util.ArrayList;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JPanel;
import javax.swing.UIManager;
import javax.swing.border.TitledBorder;

import shcee.LabelArea;
import shcee.Util;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class Block extends JPanel
{
	private static final long serialVersionUID = -4893436425708148328L;

	private ArrayList<AbstractEditor> editors;
	public String restrictionRefID = null;
	public String restrictionValue = null;
	
	private static Font titledBorderFont;
	
	public Block(Color bgColor, Node root)
	{
		super();

		editors = new ArrayList<AbstractEditor>();
		
		setBackground(bgColor);
		
		// set layout
		LayoutManager layout = new BoxLayout(this, javax.swing.BoxLayout.Y_AXIS);
		setLayout(layout);
		
		// set border with title
		String blockName = Util.getChildNodeValue(root, "Name");
		TitledBorder border = BorderFactory.createTitledBorder(blockName);
		border.setTitleFont(getTitledBorderFont());
		setBorder(border);

		// add description
		String blockDescription = Util.getChildNodeValue(root, "Description");
		LabelArea descriptionLabel = new LabelArea(blockDescription);
		add(descriptionLabel);
		
		// add editors
		NodeList nl = root.getChildNodes();
		
		for (int i = 0; i < nl.getLength(); i++)
		{
			Node n = nl.item(i);
			
			if (n.getNodeName().equals("Restriction"))
				addRestriction(n);
			else if (n.getNodeName().equals("UIntValue"))
				addElem(new UIntEditor(n, this.getBackground(), -1));
			else if (n.getNodeName().equals("IntValue"))
				addElem(new IntEditor(n, this.getBackground(), -1));
			else if (n.getNodeName().equals("BoolValue"))
				addElem(new BoolEditor(n, this.getBackground(), -1));
			else if (n.getNodeName().equals("EnumValue"))
				addElem(new EnumEditor(n, this.getBackground(), -1));
			else if (n.getNodeName().equals("ByteArray"))
				addElem(new ByteArrayEditor(n, this.getBackground(), -1));
			else if (n.getNodeName().equals("Reserved"))
				addElem(new ReservedBits(n, this.getBackground(), -1));
			else if (n.getNodeName().equals("Array"))
				addElem(new ArrayEditor(n, this.getBackground(), -1));
		}
	}	
	
	/**
	 * (Create and) return a font for use at titled borders in editor elements.
	 * Use this static function to only create one of these objects. 
	 */
	public static Font getTitledBorderFont()
	{
		if (titledBorderFont == null)
		{
			titledBorderFont = UIManager.getDefaults().getFont("TitledBorder.font").deriveFont(Font.BOLD);
		}
		
		return titledBorderFont;
	}
	
	/**
	 * Read the restriction RefID and Value from the subnodes
	 * and remember them.
	 * @param n
	 */
	private void addRestriction(Node n)
	{
		restrictionRefID = Util.getChildNodeValue(n, "RefID");
		restrictionValue = Util.getChildNodeValue(n, "Value");
	}

	private void addElem(AbstractEditor c)
	{
		add(Box.createRigidArea(new Dimension(10, 10))); // space between components
		add(c);
		editors.add(c);
	}
	
	public int readFromEepromArray(byte[] eeprom, int offsetBit)
	{
		int myOffset = 0;
		
		for (AbstractEditor ed : editors)
		{
			int usedBits = ed.readFromEepromArray(eeprom, offsetBit + myOffset);
			myOffset += usedBits; 
		}

		return myOffset;
	}
	
	public int writeToEepromArray(byte[] eeprom, int offsetBit)
	{
		int myOffset = 0;
		
		for (AbstractEditor ed : editors)
		{
			int usedBits = ed.writeToEepromArray(eeprom, offsetBit + myOffset);
			myOffset += usedBits; 
		}

		return myOffset;
	}
	
	/**
	 * Go through all editors searching for one with the given name and
	 * return the string representation of the value.
	 * @param id The ID of the value (according to the e2p_layout.xml).
	 * @param humanReadable If TRUE, return a better readable string
	 *        representation only used for the GUI.
	 * @return The value as String, or null if not found.
	 */
	public String findValue(String id, boolean humanReadable)
	{
		for (AbstractEditor ed : editors)
		{
			if (ed.id.equals(id))
				return ed.getValue(humanReadable);
		}
		
		return null;
	}
	
	/**
	 * Check if the data in all contained editors is valid.
	 * @return
	 */
	public boolean dataIsValid()
	{
		for (AbstractEditor ed : editors)
		{
			if (!ed.dataIsValid())
				return false;
		}
		
		return true;
	}
}
