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

import java.io.Console;
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
 * This is a generator for packet format definition files used by the SHC device firmwares.
 * The positions, length and ranges are saved to a *.h file per MessageGroup.
 * @author uwe
 */
public class SourceCodeGeneratorPacket
{
	private static String newline = System.getProperty("line.separator");
	private Hashtable<String, Integer> headerExtOffset = new Hashtable<String, Integer>();
	private Hashtable<String, Integer> headerExtFieldOccurrences = new Hashtable<String, Integer>();
	
	public SourceCodeGeneratorPacket() throws Exception
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
		
		generateHeaderExtensionCommonFile(xmlRoot, messageTypes);
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
		outHeader.println("#include <stdbool.h>");
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
		outHeader.println("// Remember the MessageType after receiving a packet to reduce code size");
		outHeader.println("// in common header extension access functions.");
		outHeader.println("uint8_t __MESSAGETYPE;");
		outHeader.println("");

		// MessageGroupID enum
		
		NodeList messageGroupNodes = XPathAPI.selectNodeList(xmlRoot, "/Packet/MessageGroup");
		
		outHeader.println("// ENUM MessageGroupID");
		outHeader.println("typedef enum {");
		
		for (int d = 0; d < messageGroupNodes.getLength(); d++)
		{
			Node msgGroupNode = messageGroupNodes.item(d);	
			int messageGroupID = Integer.parseInt(Util.getChildNodeValue(msgGroupNode, "MessageGroupID"));
			String name = Util.getChildNodeValue(msgGroupNode, "Name").toUpperCase();

			String suffix = d == messageGroupNodes.getLength() - 1 ? "" : ","; 

			outHeader.println("  MESSAGEGROUP_" + name + " = " + messageGroupID + suffix);
		}

		outHeader.println("} MessageGroupIDEnum;");
		outHeader.println("");

		// header fields
		
		StringBuilder funcDefsH = new StringBuilder();
		ArrayList<String> dataFieldsH = new ArrayList<String>();
	
		int offsetHeader = generateDataFieldDefs(headerNode, false, 0, "pkg_header", funcDefsH, dataFieldsH, 0, 0);
		
		outHeader.println(funcDefsH.toString());
		outHeader.println("// overall length: " + offsetHeader + " bits");
		outHeader.println("");

		// additional helper functions
		
		outHeader.println("// Function to check CRC value against calculated one (after reception).");
		outHeader.println("static inline bool pkg_header_check_crc32(uint8_t packet_size_bytes)");
		outHeader.println("{");
		outHeader.println("  return getBuf32(0) == crc32(bufx + 4, packet_size_bytes - 4);");
		outHeader.println("}");
		outHeader.println("");		
		
		outHeader.println("#endif /* _PACKET_HEADER_H */");
		
		outHeader.close();
		return offsetHeader;
	}
	
	/**
	 * Generate the source code (.h) file for the packet header, part 2:
	 * Append additional functions used to interpret packets after reception.
	 * @param xmlRoot
	 * @throws Exception 
	 */
	private void generateHeaderExtensionCommonFile(Node xmlRoot, Hashtable<Integer, String> messageTypes) throws Exception {
		PrintWriter outHeader = new PrintWriter(new FileWriter("../../firmware/src_common/packet_headerext_common.h"));

		outHeader.println(genCopyrightNotice());
		outHeader.println("#ifndef _PACKET_HEADEREXT_COMMON_H");
		outHeader.println("#define _PACKET_HEADEREXT_COMMON_H");
		outHeader.println("");
		outHeader.println("#include <stdbool.h>");
		outHeader.println("#include \"util.h\"");
		outHeader.println("#include \"e2p_access.h\"");
		outHeader.println("");

		for (Integer i : messageTypes.keySet())
		{
			outHeader.println("#include \"packet_headerext_" + messageTypes.get(i).toLowerCase() + ".h\"");
		}
		
		outHeader.println("");
		outHeader.println("// This file contains functions to access fields common to several message");
		outHeader.println("// types. It allows the user to access the fields without explicitly");
		outHeader.println("// specifying the message type in the function call.");
		outHeader.println("// WARNING: If you access a field not contained in the received MessageType,");
		outHeader.println("// you get a wrong value 0!");
		outHeader.println("");

		// "adjust offset" function
		
		outHeader.println("// Initialize the header offset variable, used to correctly interpret");
		outHeader.println("// contents of the header extension and the message data after reception.");
		outHeader.println("static void pkg_header_adjust_offset(void) __attribute__ ((unused));");
		outHeader.println("static void pkg_header_adjust_offset(void)");
		outHeader.println("{");
		outHeader.println("  __MESSAGETYPE = pkg_header_get_messagetype();");
		outHeader.println("");
		outHeader.println("  switch (__MESSAGETYPE)");
		outHeader.println("  {");

		for (Integer messageTypeID : messageTypes.keySet())
		{
			String messageTypeName = messageTypes.get(messageTypeID);
			
			outHeader.println("    case MESSAGETYPE_" + messageTypeName.toUpperCase() + ":");
			outHeader.println("      __HEADEROFFSETBITS = " + headerExtOffset.get(messageTypeName) + ";");
			outHeader.println("      break;");
		}
		
		outHeader.println("  }");
		outHeader.println("}");
		outHeader.println("");
		
		// common access function for header extension fields
		
		for (String name : headerExtFieldOccurrences.keySet())
		{
			String shcType = headerExtensionFieldType(xmlRoot, name);
			String cType = fieldTypeToCType(shcType);
			
			// SET
			
			outHeader.println("// Set " + name + " (" + shcType + ")");
			outHeader.println("// Same function for all MessageTypes!");
			outHeader.println("static void pkg_headerext_common_set_" + name.toLowerCase() + "(" + cType + " val) __attribute__ ((unused));");
			outHeader.println("static void pkg_headerext_common_set_" + name.toLowerCase() + "(" + cType + " val)");
			outHeader.println("{");
			outHeader.println("  switch (__MESSAGETYPE)");
			outHeader.println("  {");
			
			for (Integer messageTypeID : messageTypes.keySet())
			{
				String messageTypeName = messageTypes.get(messageTypeID);
				
				if (headerExtensionContainsField(xmlRoot, messageTypeID, name))
				{
					outHeader.println("    case MESSAGETYPE_" + messageTypeName.toUpperCase() + ":");
					outHeader.println("      pkg_headerext_" + messageTypeName.toLowerCase() + "_set_" + name.toLowerCase() + "(val);");
					outHeader.println("      break;");
				}
			}
			
			outHeader.println("    default:");
			outHeader.println("      break;");

			outHeader.println("  }");
			outHeader.println("}");
			outHeader.println("");
			
			// GET
			
			outHeader.println("// Get " + name + " (" + shcType + ")");
			outHeader.println("// Same function for all MessageTypes!");
			outHeader.println("static " + cType + " pkg_headerext_common_get_" + name.toLowerCase() + "(void) __attribute__ ((unused));");
			outHeader.println("static " + cType + " pkg_headerext_common_get_" + name.toLowerCase() + "(void)");
			outHeader.println("{");
			outHeader.println("  switch (__MESSAGETYPE)");
			outHeader.println("  {");
			
			for (Integer messageTypeID : messageTypes.keySet())
			{
				String messageTypeName = messageTypes.get(messageTypeID);
				
				if (headerExtensionContainsField(xmlRoot, messageTypeID, name))
				{
					outHeader.println("    case MESSAGETYPE_" + messageTypeName.toUpperCase() + ":");
					outHeader.println("      return pkg_headerext_" + messageTypeName.toLowerCase() + "_get_" + name.toLowerCase() + "();");
					outHeader.println("      break;");
				}
			}
			
			outHeader.println("    default:");
			outHeader.println("      return 0;"); // FIXME: Better return MAXVAL - 1
			outHeader.println("      break;");

			outHeader.println("  }");
			outHeader.println("}");
			outHeader.println("");			
		}
		
		outHeader.println("#endif /* _PACKET_HEADEREXT_COMMON_H */");
		
		outHeader.close();
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
			
				int offsetHeaderExt = generateDataFieldDefs(extensionNode, false, offset, "pkg_headerext_" + messageTypeName.toLowerCase(), funcDefsHE, dataFieldsHE, 0, 0);
				
				out.println(funcDefsHE.toString());
				out.println("// overall length: " + offsetHeaderExt + " bits");
				out.println("// message data follows: " + (containsMessageData ? "yes" : "no"));
				out.println("");
				out.println("#endif /* " + defineStr + " */");
				
				// remember size of header + header extension
				headerExtOffset.put(messageTypeName, offsetHeaderExt);
				
				// count fields in the header extension
				for (String name : dataFieldsHE)
				{
					if (headerExtFieldOccurrences.containsKey(name))
					{
						headerExtFieldOccurrences.put(name, headerExtFieldOccurrences.get(name) + 1);
					}
					else
					{
						headerExtFieldOccurrences.put(name, 1);
					}
				}
				
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

			String defineStr = "_MSGGRP_" + messageGroupName.toUpperCase() + "_H";

			out.println(genCopyrightNotice());
			out.println("#ifndef " + defineStr);
			out.println("#define " + defineStr);
			out.println("");
			
			out.println("#include \"packet_header.h\"");
			out.println("#include \"packet_headerext_common.h\"");
			
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
			
			// MessageID enum
			
			NodeList messageNodes = XPathAPI.selectNodeList(msgGroupNode, "Message");
			
			out.println("// ENUM for MessageIDs of this MessageGroup");
			out.println("typedef enum {");
			
			for (int n = 0; n < messageNodes.getLength(); n++)
			{
				Node messageNode = messageNodes.item(n);	
				int messageID = Integer.parseInt(Util.getChildNodeValue(messageNode, "MessageID"));
				String name = Util.getChildNodeValue(messageNode, "Name").toUpperCase();

				String suffix = n == messageNodes.getLength() - 1 ? "" : ","; 

				out.println("  MESSAGEID_" + messageGroupName.toUpperCase() + "_" + name + " = " + messageID + suffix);
			}

			out.println("} " + messageGroupName.toUpperCase() + "_MessageIDEnum;");
			out.println("");
			
			// for all messages
			
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

				int offset2 = generateDataFieldDefs(msgNode, true, 0, "msg_" + fullMessageName, funcDefs, dataFields, 0, 0);

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
				if (!msgDescription.equals(""))
				{
					out.println("// Description: " + msgDescription);
				}
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
					
					if (headerExtensionContainsField(xmlRoot, messageTypeID, "MessageID"))
					{
						out.println("  pkg_headerext_" + messageTypeName.toLowerCase() + "_set_messagegroupid(" + messageGroupID + ");");
						out.println("  pkg_headerext_" + messageTypeName.toLowerCase() + "_set_messageid(" + messageID + ");");
					}
					
					int hdrBits = headerExtOffset.get(messageTypeName);
					int ovrBits = containsMessageData(xmlRoot, messageTypeID) ? hdrBits + offset2 : hdrBits;
					int neededBytes = (ovrBits - 1) / 8 + 1;
					int packetBytes = ((neededBytes - 1) / 16 + 1) * 16;
					
					out.println("  __HEADEROFFSETBITS = " + hdrBits + ";");
					out.println("  __PACKETSIZEBYTES = " + packetBytes + ";");
					out.println("  __MESSAGETYPE = " + messageTypeID + ";");
					out.println("}");
					out.println("");
				
				}	
				
				out.print(funcDefs.toString());
				
			}

			out.println("#endif /* " + defineStr + " */");
			
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
	 * Check if the header extension of the given MessageType contains (MessageGroupID and) MessageID.
	 * @param root
	 * @param messageTypeID
	 * @return
	 * @throws TransformerException 
	 */
	private boolean headerExtensionContainsField(Node xmlRoot, int messageTypeID, String fieldName) throws TransformerException
	{
		Node field = XPathAPI.selectSingleNode(xmlRoot, "HeaderExtension[MessageType=" + messageTypeID + "]/*[ID=\"" + fieldName + "\"]");
		return field != null;
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
	 * Convert the (SHC) field type (like "UIntValue") to a C type (like "uint32_t"). 
	 * @param type
	 * @return
	 * @throws TransformerException
	 */
	private String fieldTypeToCType(String shcType) throws Exception
	{
		if (shcType.equals("UIntValue"))
			return "uint32_t";
		else if (shcType.equals("BoolValue"))
			return "bool";
		else
			throw new Exception("Unsupported common header extension field encountered.");
	}
	
	/**
	 * Get the (SHC) type of the given header extension field.
	 * @param xmlRoot
	 * @param name
	 * @return
	 * @throws TransformerException
	 */
	private String headerExtensionFieldType(Node xmlRoot, String name) throws TransformerException
	{
		Node field = XPathAPI.selectSingleNode(xmlRoot, "HeaderExtension/*[ID=\"" + name + "\"]");
		return field.getNodeName();
	}
	
	/**
	 * Generate access functions to set the data fields for the given message.
	 * @param dataNode
	 * @param arrayStructGapBits: The number of bits between two values whose
	 *        access function is generated. E.g. if you have an array consisting
	 *        of the parts A (5 bits) and B (3 bits) and the access functions
	 *        for A are generated, the arrayStructGapBits are 3.
	 * @return
	 * @throws TransformerException
	 */
	private int generateDataFieldDefs(Node dataNode, boolean useHeaderOffset, int offset, String functionPrefix, StringBuilder sb, ArrayList<String> dataFields, int arrayLength, int structLengthBits) throws TransformerException
	{
		boolean isArray = arrayLength > 0;		
		
		String arrayNameSuffix = isArray ? "[" + arrayLength + "]" : "";
		String funcParam = isArray ? "uint8_t index, " : "";	
		String funcParam2 = isArray ? "uint8_t index" : "void";	

		NodeList childs = dataNode.getChildNodes();
		
		for (int e = 0; e < childs.getLength(); e++)
		{
			Node element = childs.item(e);
		
			String description = Util.getChildNodeValue(element, "Description", true);
			
			if (element.getNodeName().equals("EnumValue"))
			{
				String ID1 = Util.getChildNodeValue(element, "ID");
				dataFields.add(ID1);
				int bits = Integer.parseInt(Util.getChildNodeValue(element, "Bits"));
				if (!isArray)
					structLengthBits = bits;

				// enum
				NodeList enumElements = XPathAPI.selectNodeList(element, "Element");
				
				sb.append("// " + ID1 + " (EnumValue" + arrayNameSuffix + ")" + newline);
				
				if (isArray && (structLengthBits != bits))
				{
					sb.append("// This sub-element with " + bits + " bits is part of an element with " + structLengthBits + " bits in a structured array." + newline);
				}
					
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);
				
				sb.append("#ifndef _ENUM_" + ID1 + newline);
				sb.append("#define _ENUM_" + ID1 + newline);
				
				sb.append("typedef enum {" + newline);
	
				Hashtable<String, String> enumDefs = new Hashtable<String, String>();
				
				for (int ee = 0; ee < enumElements.getLength(); ee++)
				{
					Node enumElement = enumElements.item(ee);
					String value = Util.getChildNodeValue(enumElement, "Value");
					String name = ID1.toUpperCase() + "_" + Util.getChildNodeValue(enumElement, "Name").toUpperCase().replace(' ', '_');
					
					String suffix = ee == enumElements.getLength() - 1 ? "" : ","; 
					
					sb.append("  " + name + " = " + value + suffix + newline);
					
					enumDefs.put(name, value);
				}
	
				sb.append("} " + ID1 + "Enum;" + newline);
				sb.append("#endif /* _ENUM_" + ID1 + " */" + newline + newline);
				
				EnumDuplicateChecker.checkEnumDefinition(ID1, enumDefs);
				
				String offsetStr = calcAccessStr(useHeaderOffset, offset, structLengthBits, isArray);

				// SET
				
				sb.append("// Set " + ID1 + " (EnumValue)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bits " + bits + newline);			
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID1.toLowerCase() + "(" + funcParam + ID1 + "Enum val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_UIntValue(" + offsetStr + ", " + bits + ", val, bufx);" + newline);				
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET
				
				sb.append("// Get " + ID1 + " (EnumValue)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bits " + bits + newline);			
				
				sb.append("static inline " + ID1 + "Enum " + functionPrefix + "_get_" + ID1.toLowerCase() + "(" + funcParam2 + ")" + newline);
				sb.append("{" + newline);
				sb.append("  return array_read_UIntValue32(" + offsetStr + ", " + bits + ", 0, " + ((1 << bits) - 1) + ", bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += bits;
			}
			else if (element.getNodeName().equals("UIntValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				dataFields.add(ID);
				
				int bits = Integer.parseInt(Util.getChildNodeValue(element, "Bits"));
				if (!isArray)
					structLengthBits = bits;
				String minVal = Util.getChildNodeValue(element, "MinVal");
				String maxVal = Util.getChildNodeValue(element, "MaxVal");
				
				sb.append("// " + ID + " (UIntValue" + arrayNameSuffix + ")" + newline);
					
				if (isArray && (structLengthBits != bits))
				{
					sb.append("// This sub-element with " + bits + " bits is part of an element with " + structLengthBits + " bits in a structured array." + newline);
				}
					
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);
				
				String offsetStr = calcAccessStr(useHeaderOffset, offset, structLengthBits, isArray);

				// SET
				
				sb.append("// Set " + ID + " (UIntValue)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(" + funcParam + "uint32_t val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_UIntValue(" + offsetStr + ", " + bits + ", val, bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET
				
				sb.append("// Get " + ID + " (UIntValue)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				// TODO: Return minimal type uint8_t, ...
				sb.append("static inline uint32_t " + functionPrefix + "_get_" + ID.toLowerCase() + "(" + funcParam2 + ")" + newline);
				sb.append("{" + newline);
				sb.append("  return array_read_UIntValue32(" + offsetStr + ", " + bits + ", " + minVal + ", " + maxVal + ", bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += bits;

			}
			else if (element.getNodeName().equals("IntValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				dataFields.add(ID);
				
				int bits = Integer.parseInt(Util.getChildNodeValue(element, "Bits"));
				if (!isArray)
					structLengthBits = bits;
				String minVal = Util.getChildNodeValue(element, "MinVal");
				String maxVal = Util.getChildNodeValue(element, "MaxVal");
				
				sb.append("// " + ID + " (IntValue" + arrayNameSuffix + ")" + newline);
				
				if (isArray && (structLengthBits != bits))
				{
					sb.append("// This sub-element with " + bits + " bits is part of an element with " + structLengthBits + " bits in a structured array." + newline);
				}
					
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);
				
				String offsetStr = calcAccessStr(useHeaderOffset, offset, structLengthBits, isArray);

				// SET
				
				sb.append("// Set " + ID + " (IntValue)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(" + funcParam + "int32_t val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_IntValue(" + offsetStr + ", " + bits + ", val, bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET
				
				sb.append("// Get " + ID + " (IntValue)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bits " + bits + ", min val " + minVal + ", max val " + maxVal + newline);
				
				// TODO: Return minimal type uint8_t, ...
				sb.append("static inline int32_t " + functionPrefix + "_get_" + ID.toLowerCase() + "(" + funcParam2 + ")" + newline);
				sb.append("{" + newline);
				sb.append("  return array_read_IntValue32(" + offsetStr + ", " + bits + ", " + minVal + ", " + maxVal + ", bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += bits;
			}
			else if (element.getNodeName().equals("ByteArray"))
			{
				if (isArray)
					throw new TransformerException("Arrays are not supported for ByteArray elements!");
				
				String ID = Util.getChildNodeValue(element, "ID");
				String bytes = Util.getChildNodeValue(element, "Bytes");
				if (!isArray)
					structLengthBits = Integer.parseInt(bytes) * 8;
				dataFields.add(ID);
				
				sb.append("// " + ID + " (ByteArray" + arrayNameSuffix + ")" + newline);
				
				if (isArray && (structLengthBits != Integer.parseInt(bytes) * 8))
				{
					sb.append("// This sub-element with " + bytes  + " bytes is part of an element with " + structLengthBits + " bits in a structured array." + newline);
				}
					
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);

				String offsetStr = calcAccessStr(useHeaderOffset, offset, structLengthBits, isArray);

				// TODO: These functions would not work if generated.
				// Implement them and fix the generator here if byte arrays in
				// a message is needed.
				
				// SET
				
				sb.append("// Set " + ID + " (ByteArray)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bytes " + bytes + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(array * val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_ByteArray(" + offsetStr + ", " + bytes + ", val, bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET

				sb.append("// Get " + ID + " (ByteArray)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bytes " + bytes + newline);
				
				sb.append("static inline void " + functionPrefix + "_get_" + ID.toLowerCase() + "(" + funcParam + "void *dst)" + newline);
				sb.append("{" + newline);
				sb.append("  array_read_ByteArray(dst, offsetStr, " + bytes + ");" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				offset += Integer.parseInt(bytes) * 8;
			}
			else if (element.getNodeName().equals("BoolValue"))
			{
				String ID = Util.getChildNodeValue(element, "ID");
				dataFields.add(ID);
				int bits = 1;
				if (!isArray)
					structLengthBits = bits;
				
				sb.append("// " + ID + " (BoolValue" + arrayNameSuffix + ")" + newline);
				
				if (isArray && (structLengthBits != bits))
				{
					sb.append("// This sub-element with " + bits + " bits is part of an element with " + structLengthBits + " bits in a structured array." + newline);
				}
					
				if (!description.equals(""))
				{
					sb.append("// Description: " + description + newline);
				}
				
				sb.append(newline);

				String offsetStr = calcAccessStr(useHeaderOffset, offset, structLengthBits, isArray);

				// SET
				
				sb.append("// Set " + ID + " (BoolValue)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bits 1" + newline);
				
				sb.append("static inline void " + functionPrefix + "_set_" + ID.toLowerCase() + "(" + funcParam + "bool val)" + newline);
				sb.append("{" + newline);
				sb.append("  array_write_UIntValue(" + offsetStr + ", " + 1 + ", val ? 1 : 0, bufx);" + newline);
				sb.append("}" + newline);
				sb.append(newline);
				
				// GET
				
				sb.append("// Get " + ID + " (BoolValue)" + newline);
				sb.append("// Offset: " + offsetStr + ", length bits 1" + newline);
				
				// TODO: Return minimal type uint8_t, ...
				sb.append("static inline bool " + functionPrefix + "_get_" + ID.toLowerCase() + "(" + funcParam2 + ")" + newline);
				sb.append("{" + newline);
				sb.append("  return array_read_UIntValue8(" + offsetStr + ", 1, 0, 1, bufx) == 1;" + newline);
				sb.append("}" + newline);
				sb.append(newline);
								
				offset += bits;
			}
			else if (element.getNodeName().equals("Reserved"))
			{
				String bits = Util.getChildNodeValue(element, "Bits");
				sb.append("// Reserved area with " + bits + " bits" + newline);
				sb.append(newline);
				offset += Integer.parseInt(bits);
			}
			else if (element.getNodeName().equals("Array"))
			{
				int length = Integer.parseInt(Util.getChildNodeValue(element, "Length"));
				int ovrStructLengthBits = getOvrArrayStructLen(element);
				
				generateDataFieldDefs(element, useHeaderOffset, offset, functionPrefix, sb, dataFields, length, ovrStructLengthBits);
				
				// The offset is incremented by one element in the previous "generateDataFieldDefs" call.
				// The rest of the array size (arrayLength - 1) has to be added now. 
				offset += ovrStructLengthBits * length; 
			}
		}

		return offset;
	}
	
	/**
	 * For the given array node, go through all sub-elements and calculate the
	 * overall number of bits one set of elements takes.
	 * @return
	 */
	private int getOvrArrayStructLen(Node dataNode)
	{
		int len = 0;
		NodeList childs = dataNode.getChildNodes();
		
		for (int e = 0; e < childs.getLength(); e++)
		{
			Node element = childs.item(e);
			String nodeName = element.getNodeName();
			
			if (nodeName.equals("EnumValue")
					|| nodeName.equals("UIntValue")
					|| nodeName.equals("IntValue"))
			{
				len += Integer.parseInt(Util.getChildNodeValue(element, "Bits"));
			}
			else if (nodeName.equals("ByteArray"))
			{
				len += Integer.parseInt(Util.getChildNodeValue(element, "Bytes")) * 8;
			}
			else if (nodeName.equals("BoolValue"))
			{
				len += 1;
			}
		}
		
		return len;
	}
	
	/**
	 * Return a string used in generated e2p and packet data access functions to represent the byte and bit position.
	 * @param useHeaderOffset  Tells if the additional "__HEADEROFFSETBITS" is to be used.
	 * @param offset           The bit offset of the data value.
	 * @param bits             The number of bits per value (relevant for arrays).
	 * @param isArray          The information if an array is accessed by using a variable "index".
	 * @return A string like "68 + (uint16_t)index * 1, 0"
	 */
	private String calcAccessStr(boolean useHeaderOffset, int offset, int bits, boolean isArray)
	{
		String additionalOffsetPrefix = useHeaderOffset ? "(uint16_t)__HEADEROFFSETBITS + " : ""; 

		return Util.calcAccessStr(additionalOffsetPrefix, offset, bits, isArray);
	}
	
	public static String genCopyrightNotice()
	{
		return "/*" + newline +
				"* This file is part of smarthomatic, http://www.smarthomatic.org." + newline +
				"* Copyright (c) 2013..2014 Uwe Freese" + newline +
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
