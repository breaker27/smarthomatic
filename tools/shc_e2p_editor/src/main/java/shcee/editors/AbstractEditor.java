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
import java.awt.LayoutManager;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.JPanel;
import javax.swing.border.TitledBorder;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import shcee.LabelArea;
import shcee.Util;

/**
 * Abstract superclass for GUI editor elements that are used to edit config values.
 * Each object is initialized with two parts:
 * - the format / name etc. from the EEPROM definition (XML)
 * - the data from the binary EEPROM file (optional)
 * After editing, the data can be written back to the binary EEPROM file.
 * @author uwe
 */
public abstract class AbstractEditor extends JPanel
{
	private static final long serialVersionUID = -1309313960513732321L;

	public String id;
	public String description = null;
	public String format = null;
	protected TitledBorder border;
	private int arrayIndex;

	/**
	 * Set common definition parameters acording XML subtree. Set up the layout,
	 * color, border etc. of the panel.
	 * @param n The XML node that contains the information about this EEPROM value.
	 */
	public AbstractEditor(Node root, Color baseColor, int arrayIndex)
	{
		super();
		
		this.arrayIndex = arrayIndex;
		
		LayoutManager layout = new BoxLayout(this, javax.swing.BoxLayout.Y_AXIS);
		setLayout(layout);

		NodeList nl = root.getChildNodes();
		
		for (int i = 0; i < nl.getLength(); i++) {
			Node n = nl.item(i);
			String name = n.getNodeName();
			
			if (name.equals("ID"))
				id = n.getFirstChild().getNodeValue();
			else setDefinitionParameter(n);
		}
		
		if (null == id)
		{
			id = root.getNodeName();
		}
		
		String borderTitle =  arrayIndex == -1 ? id : id + "[" + arrayIndex + "]";
		
		// set border with title
		border = BorderFactory.createTitledBorder(borderTitle);
		border.setTitleFont(Block.getTitledBorderFont());
		setBorder(border);

		ComponentListener resizeListener = new ComponentAdapter(){
		    public void componentResized(ComponentEvent e) {
		    	onResize();
		    }
		};

		setBackground(Util.blendColor(baseColor, Color.white, 0.5));
		
		addComponentListener(resizeListener);
	}
	
	/**
	 * Add a text label, but only when this editor is not showing an array element.
	 */
	public void addLabel(String desc)
	{
		boolean isArray = arrayIndex != -1;
		
		if (!isArray)
		{
			LabelArea myLabel = new LabelArea(desc);
			add(myLabel);
		}
	}
	
	private void onResize()
	{
		invalidate(); // let especially the LabelArea object resize properly
	}

	/**
	 * Request the concrete subclass for initialization with the
	 * given definition parameter.
	 * @param name
	 * @param value
	 */
	public abstract void setDefinitionParameter(Node n);
	
	/**
	 * Read the actual data from a byte array (EEPROM file / EEPROM content)
	 * at the given position.
	 * @param eeprom
	 * @param offsetBit
	 * @return The number of bits read.
	 */
	public abstract int readFromEepromArray(byte[] eeprom, int offsetBit);
	
	/**
	 * Append the data of this config value to the EEPROM byte array
	 * at the given position and return the number of bits that were written.
	 * @param stream
	 * @param offsetBit
	 * @return
	 */
	public abstract int writeToEepromArray(byte[] eeprom, int offsetBit);
	
	/**
	 * Tell if the entered data is valid and can be saved to an EEPROM file.
	 * @return
	 */
	public abstract boolean dataIsValid();
	
	/**
	 * Get a String representation for the value. This is used for
	 * showing the value in the device list table and for the block
	 * restrictions.
	 * @param humanReadable If TRUE, return a better readable string
	 *        representation only used for the GUI.
	 * @return
	 */
	public abstract String getValue(boolean humanReadable);
}
