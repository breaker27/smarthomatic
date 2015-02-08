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
		out.println("#include \"e2p_access.h\"");
		out.println("");

		Node hwConfigBlock = XPathAPI.selectSingleNode(xmlRoot, "Block[Name='" + blockName + "']");
		
		StringBuilder fieldDefs = new StringBuilder();
		int length = generateFieldDefs(hwConfigBlock, offset, "e2p_" + blockName.toLowerCase(), fieldDefs, 0);

		String h = "// E2P Block \"" + blockName + "\"";
		out.println(h);
		out.println("// " + String.format("%-" + (h.length() - 3) + "s", "").replace(' ', '='));
		
		out.println("// Start offset (bit): " + offset);
		out.println("// Overall block length: " + length + " bits");
		out.println("");

		out.println(fieldDefs.toString());
		
		out.println("#endif /* _E2P_" + blockName.toUpperCase() + "_H */");
		
		out.close();
		return length;
	}
	
	/**
	 * 
	 * @param blockNode
	 * @param startOffset
	 * @param functionPrefix
	 * @param sb
	 * @param arrayCount is > 0 if an Array element is detected and this function is called recursive for the inner element type.
	 * @return
	 * @throws TransformerException
	 */
	private int generateFieldDefs(Node blockNode, int startOffset, String functionPrefix, StringBuilder sb, int arrayLength) throws TransformerException
	{
		int offset = startOffset;
		
		NodeList elements = blockNode.getChildNodes();
		
		boolean isArray = arrayLength > 0;
		
		String arrayNameSuffix = isArray ? "[" + arrayLength + "]" : "";
		String funcParam = isArray ? "uint8_t index, " : "";	
		String funcParam2 = isArray ? "uint8_t index" : "void";	

		for (int e = 0; e < elements.getLength(); e++)
		{
			Node element = elements.item(e);
			
			String description = Util.getChildNodeValue(element, "Description", true);
			
			if (element.getNodeName().equals("EnumValue"))
			{
				String ID1 = Util.getChildNodeValue(element, "ID");
				int bits = Integer.parseInt(Util.getChildNodeValue(element, "Bits"));
				String ID = ID1.toUpperCase();
				int cTypeBits = calcCTypeBits(bits);

				sb.append("// " + ID1 + " (EnumValue" + arrayNameSuffix + ")" + newline);
				
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);
				
				NodeList enumElements = XPathAPI.selectNodeList(element, "Element");
				
				sb.append("#ifndef _ENUM_" + ID1 + newline);
				sb.append("#define _ENUM_" + ID1 + newline);
				
				sb.append("typedef enum {" + newline);

				Hashtable<String, String> enumDefs = new Hashtable<String, String>();
				
				for (int ee = 0; ee < enumElements.getLength(); ee++)
				{
					Node enumElement = enumElements.item(ee);
					String value = Util.getChildNodeValue(enumElement, "Value");
					String name = ID + "_" + Util.getChildNodeValue(enumElement, "Name").toUpperCase().replace(' ', '_');
					
					String suffix = ee == enumElements.getLength() - 1 ? "" : ","; 
					
					sb.append("  " + name + " = " + value + suffix + newline);
					
					enumDefs.put(name, value);
				}

				sb.append("} " + ID1 + "Enum;" + newline);
				sb.append("#endif /* _ENUM_" + ID1 + " */" + newline + newline);
				
				EnumDuplicateChecker.checkEnumDefinition(ID1, enumDefs);
				
				String accessStr = Util.calcAccessStr("", offset, bits, isArray);
				
				// SET
				
				sb.append("// Set " + ID1 + " (EnumValue)" + newline);
				sb.append("// Offset: " + offset + ", length bits " + bits + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID1.toLowerCase() + "(" + funcParam + ID1 + "Enum val)" + newline);
				sb.append("{" + newline);
				sb.append("  eeprom_write_UIntValue(" + accessStr + ", " + bits + ", val);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET
				
				int maxVal = (1 << cTypeBits) - 1;
				
				sb.append("// Get " + ID1 + " (EnumValue)" + newline);
				sb.append("// Offset: " + offset + ", length bits " + bits + newline);
				
				sb.append("static inline " + ID1 + "Enum " + functionPrefix + "_get_" + ID1.toLowerCase() + "(" + funcParam2 + ")" + newline);
				sb.append("{" + newline);
				sb.append("  return eeprom_read_UIntValue" + cTypeBits + "(" + accessStr + ", " + bits + ", 0, " + maxVal + ");" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += isArray ? bits * arrayLength : bits;
			}
			else if (element.getNodeName().equals("UIntValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				int bits = Integer.parseInt(Util.getChildNodeValue(element, "Bits"));
				String minVal = Util.getChildNodeValue(element, "MinVal");
				String maxVal = Util.getChildNodeValue(element, "MaxVal");
				int cTypeBits = calcCTypeBits(bits);
				
				sb.append("// " + ID + " (UIntValue" + arrayNameSuffix + ")" + newline);
				
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);
				
				String accessStr = Util.calcAccessStr("", offset, bits, isArray);

				// SET
				
				sb.append("// Set " + ID + " (UIntValue)" + newline);
				sb.append("// Offset: " + offset + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(" + funcParam + "uint" + cTypeBits + "_t val)" + newline);
				sb.append("{" + newline);
				sb.append("  eeprom_write_UIntValue(" + accessStr + ", " + bits + ", val);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET
				
				sb.append("// Get " + ID + " (UIntValue)" + newline);
				sb.append("// Offset: " + offset + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				// TODO: Return minimal type uint8_t, ...
				sb.append("static inline uint" + cTypeBits + "_t " + functionPrefix + "_get_" + ID.toLowerCase() + "(" + funcParam2 + ")" + newline);
				sb.append("{" + newline);
				sb.append("  return eeprom_read_UIntValue" + cTypeBits + "(" + accessStr + ", " + bits + ", " + minVal + ", " + maxVal + ");" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += isArray ? bits * arrayLength : bits;
			}
			else if (element.getNodeName().equals("IntValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				int bits = Integer.parseInt(Util.getChildNodeValue(element, "Bits"));
				String minVal = Util.getChildNodeValue(element, "MinVal");
				String maxVal = Util.getChildNodeValue(element, "MaxVal");
				int cTypeBits = calcCTypeBits(bits);
				
				sb.append("// " + ID + " (IntValue" + arrayNameSuffix + ")" + newline);
				
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);
				
				String accessStr = Util.calcAccessStr("", offset, bits, isArray);

				// SET
				
				sb.append("// Set " + ID + " (IntValue)" + newline);
				sb.append("// Offset: " + offset + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(" + funcParam + "int" + cTypeBits + "_t val)" + newline);
				sb.append("{" + newline);
				sb.append("  eeprom_write_IntValue(" + accessStr + ", " + bits + ", val);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET
				
				sb.append("// Get " + ID + " (IntValue)" + newline);
				sb.append("// Offset: " + offset + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				// TODO: Return minimal type uint8_t, ...
				sb.append("static inline int" + cTypeBits + "_t " + functionPrefix + "_get_" + ID.toLowerCase() + "(" + funcParam2 + ")" + newline);
				sb.append("{" + newline);
				sb.append("  return eeprom_read_IntValue32(" + accessStr + ", " + bits + ", " + minVal + ", " + maxVal + ");" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += isArray ? bits * arrayLength : bits;
			}
			else if (element.getNodeName().equals("ByteArray"))
			{
				if (offset % 8 != 0)
					throw new TransformerException("ByteArray does not start at byte border! Please fix layout.");
				
				String ID = Util.getChildNodeValue(element, "ID");
				
				sb.append("// " + ID + " (ByteArray" + arrayNameSuffix + ")" + newline);
				
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);
				
				int bytes = Integer.parseInt(Util.getChildNodeValue(element, "Bytes"));
				int bits = bytes * 8;
				
				String accessStr = Util.calcAccessStr("", offset, bytes * 8, isArray);
				
				// SET
				
				sb.append("// Set " + ID + " (ByteArray)" + newline);
				sb.append("// Offset: " + offset + ", length bits " + bits + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(" + funcParam + "void *src)" + newline);
				sb.append("{" + newline);
				sb.append("  eeprom_write_block(src, (uint8_t *)((" + accessStr + ") / 8), " + bytes + ");" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET
				
				sb.append("// Get " + ID + " (ByteArray)" + newline);
				sb.append("// Offset: " + offset + ", length bits " + bits + newline);
				
				// TODO: Return minimal type uint8_t, ...
				sb.append("static inline void " + functionPrefix + "_get_" + ID.toLowerCase() + "(" + funcParam + "void *dst)" + newline);
				sb.append("{" + newline);
				sb.append("  eeprom_read_block(dst, (uint8_t *)((" + accessStr + ") / 8), " + bytes + ");" + newline);
				sb.append("}" + newline);
				sb.append(newline);

				offset += isArray ? bytes * 8 * arrayLength : bytes * 8;
			}
			else if (element.getNodeName().equals("BoolValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				
				sb.append("// " + ID + " (BoolValue" + arrayNameSuffix + ")" + newline);
				
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);
				
				String accessStr = Util.calcAccessStr("", offset, 8, isArray);
				
				// SET
				
				sb.append("// Set " + ID + " (BoolValue)" + newline);
				sb.append("// Offset: " + offset + ", length bits 8" + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(" + funcParam + "bool val)" + newline);
				sb.append("{" + newline);
				sb.append("  eeprom_write_UIntValue(" + accessStr + ", 8, val ? 1 : 0);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET
				
				sb.append("// Get " + ID + " (BoolValue)" + newline);
				sb.append("// Offset: " + offset + ", length bits 8" + newline);
				
				// TODO: Return minimal type uint8_t, ...
				sb.append("static inline bool " + functionPrefix + "_get_" + ID.toLowerCase() + "(" + funcParam2 + ")" + newline);
				sb.append("{" + newline);
				sb.append("  return eeprom_read_UIntValue8(" + accessStr + ", 8, 0, 1) == 1;" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				if (offset % 8 != 0)
				{
					System.err.println("Warning: BoolValue " + ID + " not a byte boundary! e2p leayout should be corrected.");
				}
				
				offset += isArray ? 8 * arrayLength : 8;
			}
			else if (element.getNodeName().equals("Reserved"))
			{
				if (isArray)
					throw new TransformerException("Arrays are not supported for Reserved elements!");
				
				String bits = Util.getChildNodeValue(element, "Bits");
				sb.append("// Reserved area with " + bits + " bits" + newline);
				sb.append("// Offset: " + offset + newline);
				sb.append(newline);
				offset += Integer.parseInt(bits);
			}
			else if (element.getNodeName().equals("Array"))
			{
				int length = Integer.parseInt(Util.getChildNodeValue(element, "Length"));
				
				offset += generateFieldDefs(element, offset, functionPrefix, sb, length);
			}
		}
		
		return offset - startOffset;
	}

	/**
	 * Get 8, 16 or 32 as used in c types, depending on the needed bits.
	 * @param bits
	 * @return
	 */
	private int calcCTypeBits(int bits) {
		int cTypeBits = (((bits - 1) / 8) + 1) * 8;
		if (cTypeBits == 24)
		{
			cTypeBits = 32;
		}
		return cTypeBits;
	}
}
