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

package shcee;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Hashtable;

import javax.swing.JOptionPane;
import javax.xml.transform.TransformerException;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import com.sun.org.apache.xpath.internal.XPathAPI;

/**
 * This is a very simple generator for EEPROM layout files used by the SHC device firmwares.
 * The positions, length and ranges are saved to a *.h file per DeviceType.
 * This generator assumes that EEPROM blocks have a DeviceType as restriction (which
 * is the case for SHC) and creates one file per DeviceType.
 * @author uwe
 */
public class SourceCodeGeneratorE2P
{
	private static String newline = System.getProperty("line.separator");
	
	public SourceCodeGeneratorE2P() throws TransformerException, IOException
	{	
		String errMsg = Util.conformsToSchema(SHCEEMain.EEPROM_LAYOUT_XML, SHCEEMain.EEPROM_METAMODEL_XSD);
		
		if (null != errMsg)
		{
			JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, SHCEEMain.EEPROM_LAYOUT_XML + " does not conform to " +
					SHCEEMain.EEPROM_METAMODEL_XSD + ".\nError message:\n" + errMsg , "Error", JOptionPane.ERROR_MESSAGE);
			return;
		}
		
		Node xmlRoot;
		
		try
		{
			xmlRoot = Util.readXML(new File(SHCEEMain.EEPROM_LAYOUT_XML));
		}
		catch (Exception e)
		{
			e.printStackTrace();
			JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, SHCEEMain.EEPROM_LAYOUT_XML + " could not be loaded", "Error", JOptionPane.ERROR_MESSAGE);
			return;
		}
		
		xmlRoot = Util.getChildNode(xmlRoot, "E2P");
		
		int offset = 0;
		
		offset += generateBlockHeaderFile(xmlRoot, "Hardware", offset);
		offset += generateBlockHeaderFile(xmlRoot, "Generic", offset);
		
		// dump all device specific blocks
		NodeList blocks = XPathAPI.selectNodeList(xmlRoot, "Block");
	
		for (int b = 0; b < blocks.getLength(); b++)
		{
			Node block = blocks.item(b);
			String blockName = Util.getChildNodeValue(block, "Name");
			
			// skip the common blocks
			if (blockName.equals("Hardware") || blockName.equals("Generic"))
			{
				continue;
			}
	
			// every other block has to have a restriction with a devTypeID
			Node restrictionValueNode = XPathAPI.selectSingleNode(block, "Restriction[RefID='DeviceType']");
			String deviceTypeID = Util.getChildNodeValue(restrictionValueNode, "Value");		
			String deviceTypeName = deviceTypeNameOfDeviceTypeID(xmlRoot, Integer.parseInt(deviceTypeID));
			
			generateBlockHeaderFile(xmlRoot, deviceTypeName, offset);
		}
	}
	

	/**
	 * Retrieve the (human readable) DeviceType name according to the definition in the hardware config block.
	 * @param xmlRoot
	 * @param devTypeID
	 * @return
	 * @throws TransformerException
	 */
	private String deviceTypeNameOfDeviceTypeID(Node xmlRoot, int devTypeID) throws TransformerException
	{
		Node def = XPathAPI.selectSingleNode(xmlRoot, "Block/EnumValue[ID='DeviceType']/Element[Value=" + devTypeID + "]");
		return Util.getChildNodeValue(def, "Name");		
	}
	
	private int generateBlockHeaderFile(Node xmlRoot, String blockName, int offset) throws IOException, TransformerException
	{
		PrintWriter out = new PrintWriter(new FileWriter("../../firmware/src_common/e2p_" + blockName.toLowerCase() + ".h"));
		out.println(SourceCodeGeneratorPacket.genCopyrightNotice());
		out.println("#ifndef _E2P_" + blockName.toUpperCase() + "_H");
		out.println("#define _E2P_" + blockName.toUpperCase() + "_H");
		out.println("");
		out.println("");

		Node hwConfigBlock = XPathAPI.selectSingleNode(xmlRoot, "Block[Name='" + blockName + "']");
		
		StringBuilder fieldDefs = new StringBuilder();
		int length = generateFieldDefs(hwConfigBlock, offset, fieldDefs);

		out.println("// ---------- " + blockName + " ----------");
		out.println("// Start offset (bit): " + offset);
		out.println("// Overall block length: " + length + " bits");
		out.println("");

		out.println(fieldDefs.toString());
		
		out.println("#endif /* _E2P_" + blockName.toUpperCase() + "_H */");
		
		out.close();
		return length;
	}
	
	private int generateFieldDefs(Node blockNode, int startOffset, StringBuilder sb) throws TransformerException
	{
		int offset = startOffset;
		
		NodeList elements = blockNode.getChildNodes();

		for (int e = 0; e < elements.getLength(); e++)
		{
			Node element = elements.item(e);
			
			if (element.getNodeName().equals("EnumValue"))
			{
				String ID1 = Util.getChildNodeValue(element, "ID");
				sb.append("// EnumValue " + ID1 + newline);
				sb.append(newline);
				String ID = ID1.toUpperCase();
				
				NodeList enumElements = XPathAPI.selectNodeList(element, "Element");
				
				sb.append("typedef enum {" + newline);

				for (int ee = 0; ee < enumElements.getLength(); ee++)
				{
					Node enumElement = enumElements.item(ee);
					String value = Util.getChildNodeValue(enumElement, "Value");
					String name = ID + "_" + Util.getChildNodeValue(enumElement, "Name").toUpperCase().replace(' ', '_');
					
					String suffix = ee == enumElements.getLength() - 1 ? "" : ","; 
					
					sb.append("  " + name + " = " + value + suffix + newline);
				}

				sb.append("} " + ID1 + "Enum;" + newline);
				sb.append(newline);
				
				sb.append("#define EEPROM_" + ID + "_BYTE " + (offset / 8) + newline);
				sb.append("#define EEPROM_" + ID + "_BIT " + (offset % 8) + newline);
				sb.append("#define EEPROM_" + ID + "_LENGTH_BITS 8" + newline);
				sb.append(newline);
				offset += 8;
			}
			else if (element.getNodeName().equals("UIntValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				sb.append("// UIntValue " + ID + newline);
				sb.append(newline);
				ID = ID.toUpperCase();
				String bits = Util.getChildNodeValue(element, "Bits");
				String minVal = Util.getChildNodeValue(element, "MinVal");
				String maxVal = Util.getChildNodeValue(element, "MaxVal");
				sb.append("#define EEPROM_" + ID + "_BYTE " + (offset / 8) + newline);
				sb.append("#define EEPROM_" + ID + "_BIT " + (offset % 8) + newline);
				sb.append("#define EEPROM_" + ID + "_LENGTH_BITS " + bits + newline);
				sb.append("#define EEPROM_" + ID + "_MINVAL " + minVal + newline);
				sb.append("#define EEPROM_" + ID + "_MAXVAL " + maxVal + newline);
				sb.append(newline);
				offset += Integer.parseInt(bits);
			}
			else if (element.getNodeName().equals("ByteArray"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				sb.append("// ByteArray " + ID + newline);
				sb.append(newline);
				ID = ID.toUpperCase();
				String bytes = Util.getChildNodeValue(element, "Bytes");
				sb.append("#define EEPROM_" + ID + "_BYTE " + (offset / 8) + newline);
				sb.append("#define EEPROM_" + ID + "_BIT " + (offset % 8) + newline);
				sb.append("#define EEPROM_" + ID + "_LENGTH_BYTES " + bytes + newline);
				sb.append(newline);
				offset += Integer.parseInt(bytes) * 8;
			}
			else if (element.getNodeName().equals("Reserved"))
			{
				String bits = Util.getChildNodeValue(element, "Bits");
				sb.append("// Reserved area with " + bits + " bits" + newline);
				sb.append(newline);
				offset += Integer.parseInt(bits);
			}
		}
		
		return offset - startOffset;
	}
}
