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

import shcee.LabelArea;

import org.w3c.dom.Node;

public class ReservedBits extends AbstractEditor
{
	private static final long serialVersionUID = -2935168121301126627L;

	private int bits;
	
	public ReservedBits(Node root)
	{
		super(root);
		
		// add label about format
		LabelArea formatLabel = new LabelArea("Reserved area of " + bits + " bits ("
				+ (bits / 8) + "  bytes)");
		add(formatLabel);
	}

	@Override
	public boolean dataIsValid()
	{
		return true;
	}

	@Override
	public void setDefinitionParameter(Node n)
	{
		String name = n.getNodeName();
		
		if (name.equals("Bits"))
			bits = Integer.parseInt(n.getFirstChild().getNodeValue());
	}

	@Override
	public int readFromEepromArray(byte[] eeprom, int offsetBit)
	{
		// There's no data to save in the reserved area.
		return bits;
	}

	@Override
	public int writeToEepromArray(byte[] eeprom, int offsetBit)
	{
		// Nothing to write here. Just skip an amount of bits.
		return bits;
	}

	@Override
	public String getValue(boolean humanReadable)
	{
		return "RES";
	}
}
