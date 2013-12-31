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
 * This is a generator for packet format definition files used by the SHC device firmwares.
 * The positions, length and ranges are saved to a *.h file per MessageGroup.
 * @author uwe
 */
public class SourceCodeGeneratorPacket
{
	private static String newline = System.getProperty("line.separator");
	private Hashtable<String, Integer> headerExtOffset = new Hashtable<String, Integer>();
	
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
		
		// generate header, header extension and message group files
		int offsetHeader = generateHeaderFile(xmlRoot);
		Hashtable<Integer, String> messageTypes = getMessageTypes(xmlRoot);

		generateHeaderExtFiles(xmlRoot, offsetHeader, messageTypes);

		generateMessageGroupFiles(xmlRoot, messageTypes);
	}

	/**
	 * Read the possible MessageTypes and store them in a Hashtable.
	 * @return
	 * @throws TransformerException 
	 */
	private Hashtable<Integer, String> getMessageTypes(Node xmlRoot) throws TransformerException
	{
		Hashtable<Integer, String> h = new Hashtable<Integer, String>();
		
		NodeList msgTypeNodes = XPathAPI.selectNodeList(xmlRoot, "/Packet/Header/EnumValue[ID=\"MessageType\"]/Element");
		
		for (int d = 0; d < msgTypeNodes.getLength(); d++)
		{
			Node msgTypeNode = msgTypeNodes.item(d);	
			h.put(Integer.parseInt(Util.getChildNodeValue(msgTypeNode, "Value")), Util.getChildNodeValue(msgTypeNode, "Name"));
		}
		
		return h;
	}
	
	/**
	 * Generate the source code (.h) file for the packet header.
	 * @param xmlRoot
	 * @throws IOException
	 * @throws TransformerException
	 */
	private int generateHeaderFile(Node xmlRoot) throws IOException, TransformerException {
		Node headerNode = Util.getChildNode(xmlRoot, "Header");
		PrintWriter outHeader = new PrintWriter(new FileWriter("../../firmware/src_common/packet_header.h"));
		outHeader.println(genCopyrightNotice());
		outHeader.println("#ifndef _PACKET_HEADER_H");
		outHeader.println("#define _PACKET_HEADER_H");
		outHeader.println("");
		outHeader.println("#include \"util.h\"");
		outHeader.println("#include \"e2p_access.h\"");
		outHeader.println("");
		outHeader.println("// Header size in bits (incl. header extension), set depending on used");
		outHeader.println("// MessageType and used for calculating message data offsets.");
		outHeader.println("uint8_t __HEADEROFFSETBITS;");
		outHeader.println("");
		outHeader.println("// Packet size in bytes including padding, set depending on MessageType,");
		outHeader.println("// MessageGroupID and MessageID and used for CRC32 calculation.");
		outHeader.println("uint8_t __PACKETSIZEBYTES;");
		outHeader.println("");

		StringBuilder funcDefsH = new StringBuilder();
		ArrayList<String> dataFieldsH = new ArrayList<String>();
	
		int offsetHeader = generateDataFieldDefs(headerNode, false, 0, "pkg_header", funcDefsH, dataFieldsH);
		
		outHeader.println(funcDefsH.toString());
		outHeader.println("// overall length: " + offsetHeader + " bits");
		outHeader.println("");

		outHeader.println("// Function to set CRC value after all data fields are set.");
		outHeader.println("static inline void pkg_header_calc_crc32(void)");
		outHeader.println("{");
		outHeader.println("  pkg_header_set_crc32(crc32(bufx + 4, __PACKETSIZEBYTES - 4));");
		outHeader.println("}");
		outHeader.println("");

		outHeader.println("#endif /* _PACKET_HEADER_H */");
		
		outHeader.close();
		return offsetHeader;
	}
	
	/**
	 * Generate one source code (.h) file for every packet header extension.
	 * @param xmlRoot
	 * @param offsetHeader
	 * @throws TransformerException 
	 * @throws IOException 
	 */
	private void generateHeaderExtFiles(Node xmlRoot, int offsetHeader, Hashtable<Integer, String> messageTypes) throws TransformerException, IOException
	{
		// for all DeviceTypes
		NodeList extensionNodes = XPathAPI.selectNodeList(xmlRoot, "HeaderExtension");
		
		for (int d = 0; d < extensionNodes.getLength(); d++)
		{
			int offset = offsetHeader;
			
			Node extensionNode = extensionNodes.item(d);
			
			ArrayList<Integer> possibleMessageTypes = getPossibleMessageTypes(extensionNode);

			for (int p = 0; p < possibleMessageTypes.size(); p++)
			{
				int messageTypeID = possibleMessageTypes.get(p);
				String messageTypeName = messageTypes.get(messageTypeID);
				
				String defineStr = "_PACKET_HEADEREXT_" + messageTypeName.toUpperCase() + "_H";
				boolean containsMessageData = Util.getChildNodeValue(extensionNode, "ContainsMessageData").equals("true");
				
				PrintWriter out = new PrintWriter(new FileWriter("../../firmware/src_common/packet_headerext_" + messageTypeName.toLowerCase() + ".h"));

				out.println(genCopyrightNotice());
				out.println("#ifndef " + defineStr);
				out.println("#define " + defineStr);
				out.println("");
	
				out.println("#include \"packet_header.h\"");
				out.println("");
	
				StringBuilder funcDefsHE = new StringBuilder();
				ArrayList<String> dataFieldsHE = new ArrayList<String>();
			
				int offsetHeaderExt = generateDataFieldDefs(extensionNode, false, offset, "pkg_headerext_" + messageTypeName.toLowerCase(), funcDefsHE, dataFieldsHE);
				
				out.println(funcDefsHE.toString());
				out.println("// overall length: " + offsetHeaderExt + " bits");
				out.println("// message data follows: " + (containsMessageData ? "yes" : "no"));
				out.println("");
				out.println("#endif /* " + defineStr + " */");
				
				// remember size of header + header extension
				headerExtOffset.put(messageTypeName, offsetHeaderExt);
				
				out.close();
			}
		}			
	}
		
	/**
	 * Generate one source code (.h) file for every MessageGroupID.
	 * @param xmlRoot
	 * @param offsetHeader
	 * @param messageTypes
	 * @throws TransformerException 
	 * @throws IOException 
	 */
	private void generateMessageGroupFiles(Node xmlRoot, Hashtable<Integer, String> messageTypes) throws TransformerException, IOException
	{
		// for all DeviceTypes
		NodeList msgGroupNodes = XPathAPI.selectNodeList(xmlRoot, "MessageGroup");
		
		for (int d = 0; d < msgGroupNodes.getLength(); d++)
		{
			Node msgGroupNode = msgGroupNodes.item(d);
			Hashtable<Integer, String> messageIDs = new Hashtable<Integer, String>();
						
			//String devTypeID = Util.getChildNodeValue(msgGroupNode, "Value");

			String messageGroupName = Util.getChildNodeValue(msgGroupNode, "Name").toLowerCase().replace(' ', '_');
			String messageGroupID = Util.getChildNodeValue(msgGroupNode, "MessageGroupID");
			String description = Util.getChildNodeValue(msgGroupNode, "Description");
			
			// String.format("%03d", Integer.parseInt(messageGroupID))
			PrintWriter out = new PrintWriter(new FileWriter("../../firmware/src_common/msggrp_" + messageGroupName + ".h"));

			out.println(genCopyrightNotice());

			out.println("#include \"packet_header.h\"");

			for (Integer i : messageTypes.keySet())
			{
				out.println("#include \"packet_headerext_" + messageTypes.get(i).toLowerCase() + ".h\"");
			}
			
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
				ArrayList<Integer> possibleMessageTypes = getPossibleMessageTypes(msgNode);
				
				String messageName = Util.getChildNodeValue(msgNode, "Name").toLowerCase().replace(' ', '_');
				String messageID = Util.getChildNodeValue(msgNode, "MessageID").toLowerCase().replace(' ', '_');
				String validity = Util.getChildNodeValue(msgNode, "Validity");
				String msgDescription = Util.getChildNodeValue(msgNode, "Description");
				
				messageIDs.put(Integer.parseInt(messageID), messageName);
				
				//String fullMessageName = String.format("%03d", Integer.parseInt(messageGroupID)) + "_" + String.format("%03d", Integer.parseInt(messageID)) + "_" + messageName;
				String fullMessageName =  messageGroupName + "_" + messageName;

				StringBuilder funcDefs = new StringBuilder();

				int offset2 = generateDataFieldDefs(msgNode, true, 0, "msg_" + fullMessageName, funcDefs, dataFields);

				out.println("");
				
				String h2 = "// Message \"" + fullMessageName + "\"";
				out.println(h2);
				out.println("// " + String.format("%-" + (h2.length() - 3) + "s", "").replace(' ', '-'));

				out.println("// MessageGroupID: " + messageGroupID);
				out.println("// MessageID: " + messageID);
				out.println("// Possible MessageTypes: " + createMessageTypeList(messageTypes, possibleMessageTypes));
				out.println("// Validity: " + validity);
				out.println("// Length w/o Header + HeaderExtension: " + offset2 + " bits");
				out.println("// Data fields: " + Util.arrayListToString(dataFields, ", "));
				out.println("// Description: " + msgDescription);
				out.println("");
				
				for (int p = 0; p < possibleMessageTypes.size(); p++)
				{
					int messageTypeID = possibleMessageTypes.get(p);
					String messageTypeName = messageTypes.get(messageTypeID);
					
					out.println("// Function to initialize header for the MessageType \"" + messageTypeName + "\".");
					out.println("static inline void pkg_header_init_" + fullMessageName + "_" + messageTypeName.toLowerCase() + "(void)");
					out.println("{");
					out.println("  memset(&bufx[0], 0, sizeof(bufx));");
					out.println("  pkg_header_set_messagetype(" + messageTypeID + ");");
					
					if (containsMessageID(xmlRoot, messageTypeID))
					{
						out.println("  pkg_headerext_" + messageTypeName.toLowerCase() + "_set_messagegroupid(" + messageGroupID + ");");
						out.println("  pkg_headerext_" + messageTypeName.toLowerCase() + "_set_messageid(" + messageID + ");");
					}
					
					int hdrBits = headerExtOffset.get(messageTypeName);
					int ovrBits = hdrBits + offset2;
					int neededBytes = (ovrBits - 1) / 8 + 1;
					int packetBytes = ((neededBytes - 1) / 16 + 1) * 16;
					
					out.println("  __HEADEROFFSETBITS = " + hdrBits + ";");
					out.println("  __PACKETSIZEBYTES = " + packetBytes + ";");
					out.println("}");
					out.println("");
				
				}	
				
				out.print(funcDefs.toString());
				
			}

			out.close();
		}
	}

	/**
	 * Generate a string describing the possible MessageTypes for a message.
	 * @param possibleMessageTypes
	 * @return
	 */
	private String createMessageTypeList(Hashtable<Integer, String> messageTypes, ArrayList<Integer> possibleMessageTypes)
	{
		String res = "";
		
		for (int i = 0; i < possibleMessageTypes.size(); i++)
		{
			if (i > 0)
			{
				res += ", ";
			}
			
			res += messageTypes.get(possibleMessageTypes.get(i));
		}
		
		return res;
	}

	/**
	 * Return the integer IDs of the possible MessageTypes for the given Message as ArrayList.
	 * @param msgNode
	 * @return
	 * @throws TransformerException
	 */
	private ArrayList<Integer> getPossibleMessageTypes(Node msgNode) throws TransformerException {
		ArrayList<Integer> res = new ArrayList<Integer>();				
		
		NodeList possibleMessageTypeNodes = XPathAPI.selectNodeList(msgNode, "MessageType");
		
		for (int j = 0; j < possibleMessageTypeNodes.getLength(); j++)
		{
			res.add(Integer.parseInt(possibleMessageTypeNodes.item(j).getFirstChild().getNodeValue()));
		}
		
		return res;
	}
	
	/**
	 * Check if the MessageType with the given ID contains message data according to the definition in the XML file.
	 * @param root
	 * @param messageTypeID
	 * @return
	 * @throws TransformerException 
	 */
	private boolean containsMessageData(Node xmlRoot, int messageTypeID) throws TransformerException
	{
		Node hext = XPathAPI.selectSingleNode(xmlRoot, "HeaderExtension[MessageType=" + messageTypeID + "]");
		return Util.getChildNodeValue(hext, "ContainsMessageData").equals("true");		
	}
	
	/**
	 * Check if the header extension of the given MessageType contains (MessageGroupID and) MessageID.
	 * @param root
	 * @param messageTypeID
	 * @return
	 * @throws TransformerException 
	 */
	private boolean containsMessageID(Node xmlRoot, int messageTypeID) throws TransformerException
	{
		Node hext = XPathAPI.selectSingleNode(xmlRoot, "HeaderExtension[MessageType=" + messageTypeID + "]/UIntValue[ID=\"MessageID\"]");
		return hext != null;
	}
	
	/**
	 * Generate accessor functions to set the data fields for the given message.
	 * @param dataNode
	 * @return
	 * @throws TransformerException
	 */
	private int generateDataFieldDefs(Node dataNode, boolean useHeaderOffset, int offset, String functionPrefix, StringBuilder sb, ArrayList<String> dataFields) throws TransformerException
	{	
		NodeList childs = dataNode.getChildNodes();
		
		for (int e = 0; e < childs.getLength(); e++)
		{
			Node element = childs.item(e);
		
			if (element.getNodeName().equals("EnumValue"))
			{
				String ID1 = Util.getChildNodeValue(element, "ID");
				String bits = Util.getChildNodeValue(element, "Bits");

				// enum
				NodeList enumElements = XPathAPI.selectNodeList(element, "Element");
				
				sb.append("// ENUM " + ID1 + newline);
				sb.append("typedef enum {" + newline);
	
				for (int ee = 0; ee < enumElements.getLength(); ee++)
				{
					Node enumElement = enumElements.item(ee);
					String value = Util.getChildNodeValue(enumElement, "Value");
					String name = ID1.toUpperCase() + "_" + Util.getChildNodeValue(enumElement, "Name").toUpperCase().replace(' ', '_');
					
					String suffix = ee == enumElements.getLength() - 1 ? "" : ","; 
					
					sb.append("  " + name + " = " + value + suffix + newline);
				}
	
				sb.append("} " + ID1 + "Enum;" + newline + newline);
				
				// access function
				dataFields.add(ID1);
				sb.append("// Set " + ID1 + " (EnumValue)" + newline);
				String offsetStr = generateOffsetString(useHeaderOffset, offset);
				sb.append("// Offset: " + offsetStr + ", length bits " + bits + newline);			
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID1.toLowerCase() + "(" + ID1 + "Enum val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_UIntValue(" + offsetStr + ", " + bits + ", val, bufx);" + newline);				
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
				String offsetStr = generateOffsetString(useHeaderOffset, offset);
				sb.append("// Offset: " + offsetStr + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(uint32_t val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_UIntValue(" + offsetStr + ", " + bits + ", val, bufx);" + newline);
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
				String offsetStr = generateOffsetString(useHeaderOffset, offset);
				sb.append("// Offset: " + offsetStr + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(int32_t val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_IntValue(" + offsetStr + ", " + bits + ", val, bufx);" + newline);
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
				String offsetStr = generateOffsetString(useHeaderOffset, offset);
				sb.append("// Offset: " + offsetStr + ", length bytes " + bytes + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(array * val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_ByteArray(" + offsetStr + ", " + bytes + ", val, bufx);" + newline);
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
			else if (element.getNodeName().equals("BoolValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				dataFields.add(ID);
				
				sb.append("// Set " + ID + " (BoolValue)" + newline);
				String offsetStr = generateOffsetString(useHeaderOffset, offset);
				sb.append("// Offset: " + offsetStr + ", length bytes 1" + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(uint8_t val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_UIntValue(" + offsetStr + ", " + 1 + ", val, bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += 1;
			}
		}

		return offset;
	}
	
	private String generateOffsetString(boolean useHeaderOffset, int offset)
	{
		if (useHeaderOffset)
		{
			return "((uint16_t)__HEADEROFFSETBITS + " + offset + ") / 8, ((uint16_t)__HEADEROFFSETBITS + " + offset + ") % 8"; 
		}
		else
		{
			return (offset / 8) + ", " + (offset % 8);
		}
		 
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
