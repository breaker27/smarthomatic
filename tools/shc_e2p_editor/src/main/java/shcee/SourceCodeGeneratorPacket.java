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
import java.util.Collections;
import java.util.Dictionary;
import java.util.HashSet;
import java.util.Hashtable;
import java.util.Vector;

import javax.swing.JOptionPane;
import javax.xml.transform.TransformerException;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import com.sun.org.apache.xpath.internal.XPathAPI;

/**
 * This is a very simple generator for packet format definition files used by the SHC device firmwares.
 * The positions, length and ranges are saved to a *.h file per MessageGroup.
 * @author uwe
 */
public class SourceCodeGeneratorPacket
{
	private static String newline = System.getProperty("line.separator");
	
	public SourceCodeGeneratorPacket() throws TransformerException, IOException
	{
		String errMsg = Util.conformsToSchema(SHCEEMain.PACKET_CATALOG_XML, SHCEEMain.PACKET_METAMODEL_XSD);
		
		if (null != errMsg)
		{
			JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, SHCEEMain.PACKET_CATALOG_XML + " does not conform to " +
					SHCEEMain.PACKET_METAMODEL_XSD + ".\nError message:\n" + errMsg , "Error", JOptionPane.ERROR_MESSAGE);
			return;
		}
		
		Node xmlRoot;
		
		try
		{
			xmlRoot = Util.readXML(new File(SHCEEMain.PACKET_CATALOG_XML));
		}
		catch (Exception e)
		{
			e.printStackTrace();
			JOptionPane.showMessageDialog(SHCEEMain.mySHCEEMain, SHCEEMain.PACKET_CATALOG_XML + " could not be loaded", "Error", JOptionPane.ERROR_MESSAGE);
			return;
		}
		
		xmlRoot = Util.getChildNode(xmlRoot, "Packet");
		
		// header
		Node headerNode = Util.getChildNode(xmlRoot, "Header");
		PrintWriter outHeader = new PrintWriter(new FileWriter("../../firmware/src_common/packet_header.h"));
		outHeader.println(genCopyrightNotice());
		outHeader.println("#ifndef _PACKET_HEADER_H");
		outHeader.println("#define _PACKET_HEADER_H");
		outHeader.println("");
		outHeader.println("#include \"util.h\"");
		outHeader.println("#include \"e2p_access.h\"");
		outHeader.println("");

		StringBuilder funcDefsH = new StringBuilder();
		ArrayList<String> dataFieldsH = new ArrayList<String>();
	
		int offsetHeader = generateDataFieldDefs(headerNode, 0, "pkg_header", funcDefsH, dataFieldsH);
		
		outHeader.println(funcDefsH.toString());
		outHeader.println("// overall length: " + offsetHeader + " bits");
		outHeader.println("");
		outHeader.println("#endif /* _PACKET_HEADER_H */");
		
		outHeader.close();
		
		// for all DeviceTypes
		NodeList msgGroupNodes = XPathAPI.selectNodeList(xmlRoot, "//MessageGroup");
		
		for (int d = 0; d < msgGroupNodes.getLength(); d++)
		{
			int offset = offsetHeader;
			
			Node msgGroupNode = msgGroupNodes.item(d);
			Hashtable<Integer, String> messageIDs = new Hashtable<Integer, String>();
						
			String devTypeID = Util.getChildNodeValue(msgGroupNode, "Value");

			String messageGroupName = Util.getChildNodeValue(msgGroupNode, "Name").toLowerCase().replace(' ', '_');
			String messageGroupID = Util.getChildNodeValue(msgGroupNode, "MessageGroupID");
			String description = Util.getChildNodeValue(msgGroupNode, "Description");
			
			// String.format("%03d", Integer.parseInt(messageGroupID))
			PrintWriter out = new PrintWriter(new FileWriter("../../firmware/src_common/msggrp_" + messageGroupName + ".h"));

			out.println(genCopyrightNotice());

			out.println("#include \"packet_header.h\"");
			out.println("#include \"e2p_access.h\"");
			out.println("");

			String h = "// Message Group \"" + messageGroupName + "\"";
			out.println(h);
			out.println("// " + String.format("%-" + (h.length() - 3) + "s", "").replace(' ', '='));

			out.println("// MessageGroupID: " + messageGroupID);

			if (!description.equals(""))
			{
				out.println("// Description: " + description);
			}

			out.println("");
			
			// for all blocks
			NodeList msg = XPathAPI.selectNodeList(msgGroupNode, "Message");

			for (int b = 0; b < msg.getLength(); b++)
			{
				Node msgNode = msg.item(b);

				ArrayList<String> dataFields = new ArrayList<String>();				
				
				String messageType = Util.getChildNodeValue(msgNode, "MessageType");
				
				String messageName = Util.getChildNodeValue(msgNode, "Name").toLowerCase().replace(' ', '_');
				String messageID = Util.getChildNodeValue(msgNode, "MessageID").toLowerCase().replace(' ', '_');
				
				messageIDs.put(Integer.parseInt(messageID), messageName);
				
				//String fullMessageName = String.format("%03d", Integer.parseInt(messageGroupID)) + "_" + String.format("%03d", Integer.parseInt(messageID)) + "_" + messageName;
				String fullMessageName =  messageGroupName + "_" + messageName;

				StringBuilder funcDefs = new StringBuilder();

				int offset2 = generateDataFieldDefs(msgNode, offset, "msg_" + fullMessageName, funcDefs, dataFields);
				int neededBytes = (offset2 - 1) / 8 + 1;

				out.println("");
				
				String h2 = "// Message \"" + fullMessageName + "\"";
				out.println(h2);
				out.println("// " + String.format("%-" + (h2.length() - 3) + "s", "").replace(' ', '-'));

				out.println("// MessageGroupID: " + messageGroupID);
				out.println("// MessageID: " + messageID);
				out.println("// MessageType: " + messageType);
				out.println("// Data fields: " + Util.arrayListToString(dataFields, ", "));
				out.println("// length: " + offset2 + " bits (needs " + neededBytes + " bytes)");
				out.println("");
				
				out.println("// Function to initialize header for the message.");
				out.println("static inline void pkg_header_init_" + fullMessageName + "(void)");
				out.println("{");
				out.println("  memset(&bufx[0], 0, sizeof(bufx));");
				out.println("  pkg_header_set_messagegroupid(" + messageGroupID + ");");
				out.println("  pkg_header_set_messageid(" + messageID + ");");
				out.println("  pkg_header_set_messagetype(" + messageType + ");");
				out.println("}");
				out.println("");
					
				int ovrPacketSize = ((neededBytes - 1 ) / 16 + 1) * 16; // because of AES encryption, packets are 16 or 32,... bytes long
				out.println("// Function to set CRC value after all data fields are set.");
				out.println("static inline void pkg_header_crc32_" + fullMessageName + "(void)");
				out.println("{");
				out.println("  pkg_header_set_crc32(crc32(bufx + 4, " + (ovrPacketSize - 4) + "));");
				out.println("}");
				out.println("");

				out.print(funcDefs.toString());
				
			}

			out.close();
		}
	}
		
	/**
	 * Generate accessor functions to set the data fields for the given message.
	 * @param dataNode
	 * @return
	 * @throws TransformerException
	 */
	private int generateDataFieldDefs(Node dataNode, int offset, String functionPrefix, StringBuilder sb, ArrayList<String> dataFields) throws TransformerException
	{	
		NodeList childs = dataNode.getChildNodes();
				
		for (int e = 0; e < childs.getLength(); e++)
		{
			Node element = childs.item(e);
		
			if (element.getNodeName().equals("EnumValue"))
			{
				String ID1 = Util.getChildNodeValue(element, "ID");
				String bits = Util.getChildNodeValue(element, "Bits");

				dataFields.add(ID1);
				sb.append("// Set " + ID1 + " (EnumValue)" + newline);
				sb.append("// byte " + (offset / 8) + ", bit " + (offset % 8) + ", length bits " + bits + newline);			
				sb.append(newline);
				
				NodeList enumElements = XPathAPI.selectNodeList(element, "Element");
				
				sb.append("typedef enum {" + newline);
	
				for (int ee = 0; ee < enumElements.getLength(); ee++)
				{
					Node enumElement = enumElements.item(ee);
					String value = Util.getChildNodeValue(enumElement, "Value");
					String name = ID1.toUpperCase() + "_" + Util.getChildNodeValue(enumElement, "Name").toUpperCase().replace(' ', '_');
					
					String suffix = ee == enumElements.getLength() - 1 ? "" : ","; 
					
					sb.append("  " + name + " = " + value + suffix + newline);
				}
	
				sb.append("} " + ID1 + "Enum;" + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID1.toLowerCase() + "(" + ID1 + "Enum val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_UIntValue(" + (offset / 8) + ", " + (offset % 8) + ", " + bits + ", val, bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += Integer.parseInt(bits);
			}
			else if (element.getNodeName().equals("UIntValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				dataFields.add(ID);
				
				String bits = Util.getChildNodeValue(element, "Bits");
				String minVal = Util.getChildNodeValue(element, "MinVal");
				String maxVal = Util.getChildNodeValue(element, "MaxVal");
				
				sb.append("// Set " + ID + " (UIntValue)" + newline);
				sb.append("// byte " + (offset / 8) + ", bit " + (offset % 8) +
						", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(uint32_t val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_UIntValue(" + (offset / 8) + ", " + (offset % 8) + ", " + bits + ", val, bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += Integer.parseInt(bits);
			}
			else if (element.getNodeName().equals("IntValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				dataFields.add(ID);
				
				String bits = Util.getChildNodeValue(element, "Bits");
				String minVal = Util.getChildNodeValue(element, "MinVal");
				String maxVal = Util.getChildNodeValue(element, "MaxVal");
				
				sb.append("// Set " + ID + " (IntValue)" + newline);
				sb.append("// byte " + (offset / 8) + ", bit " + (offset % 8) +
						", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(int32_t val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_IntValue(" + (offset / 8) + ", " + (offset % 8) + ", " + bits + ", val, bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += Integer.parseInt(bits);
			}
			else if (element.getNodeName().equals("ByteArray"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				String bytes = Util.getChildNodeValue(element, "Bytes");
				dataFields.add(ID);
				
				sb.append("// Set " + ID + " (ByteArray)" + newline);
				sb.append("// byte " + (offset / 8) + ", bit " + (offset % 8) +
						", length bytes " + bytes + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(array * val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_ByteArray(" + (offset / 8) + ", " + (offset % 8) + ", " + bytes + ", val, bufx);" + newline);
				sb.append("}" + newline);
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

		return offset;
	}
	
	private String genCopyrightNotice()
	{
		return "/*" + newline +
				"* This file is part of smarthomatic, http://www.smarthomatic.org." + newline +
				"* Copyright (c) 2013 Uwe Freese" + newline +
				"*" + newline +
				"* smarthomatic is free software: you can redistribute it and/or modify it" + newline +
				"* under the terms of the GNU General Public License as published by the" + newline +
				"* Free Software Foundation, either version 3 of the License, or (at your" + newline +
				"* option) any later version." + newline +
				"*" + newline +
				"* smarthomatic is distributed in the hope that it will be useful, but" + newline +
				"* WITHOUT ANY WARRANTY; without even the implied warranty of" + newline +
				"* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General" + newline +
				"* Public License for more details." + newline +
				"*" + newline +
				"* You should have received a copy of the GNU General Public License along" + newline +
				"* with smarthomatic. If not, see <http://www.gnu.org/licenses/>." + newline +
				"*" + newline +
				"* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" + newline +
				"* ! WARNING: This file is generated by the SHC EEPROM editor and should !" + newline +
				"* ! never be modified manually.                                         !" + newline +
				"* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" + newline +
				"*/" + newline;
	}
}
