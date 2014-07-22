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

import java.awt.Color;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

import javax.xml.XMLConstants;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.stream.StreamSource;
import javax.xml.validation.Schema;
import javax.xml.validation.SchemaFactory;
import javax.xml.validation.Validator;

import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

/**
 * Several static conveniance methods to read and write from text
 * and xml files and do some xml processing.
 * @author uwe
 */
public class Util {

	public static Node readXML(File XMLinput) throws ParserConfigurationException, SAXException, IOException {
		DocumentBuilderFactory docBuilderFactory = DocumentBuilderFactory.newInstance();
		DocumentBuilder docBuilder;
		docBuilder = docBuilderFactory.newDocumentBuilder();
		org.w3c.dom.Document doc = docBuilder.parse (XMLinput);
		doc.getDocumentElement().normalize();
		return doc;
	}

	public static void executeProg(String cmdLine) throws IOException {	
		Runtime r = Runtime.getRuntime();
		r.exec(cmdLine);
	}
	
	/**
	 * Check if the given xml file conforms to the given xsd file.
	 * @param xmlFile
	 * @param xsdFile
	 * @return Null if ok, or the error message if it does not conform to the schema
	 *         (to help the user fix it).
	 */
	static String conformsToSchema(String xmlFile, String xsdFile)
	{
		InputStream xmlFis;
		
		try
		{
			xmlFis = new FileInputStream(xmlFile);
			InputStream xsdFis = new FileInputStream(xsdFile);
			return conformsToSchema(xmlFis, xsdFis);		
		}
		catch (FileNotFoundException e)
		{
			return e.toString();
		}
	}

	/**
	 * Check if the given xml content conforms to the given xsd content.
	 * @param xml
	 * @param xsd
	 * @return Null if ok, or the error message if it does not conform to the schema
	 *         (to help the user fix it).
	 */
	static String conformsToSchema(InputStream xml, InputStream xsd)
	{		
	    try
	    {
	        SchemaFactory factory = 
	            SchemaFactory.newInstance(XMLConstants.W3C_XML_SCHEMA_NS_URI);
	        Schema schema = factory.newSchema(new StreamSource(xsd));
	        Validator validator = schema.newValidator();
	        validator.validate(new StreamSource(xml));
	        return null;
	    }
	    catch(Exception ex)
	    {
	        return ex.toString();
	    }
	}
	
	/**
	 * Read a text file and return the content as array of bytes.
	 * from http://stackoverflow.com/questions/858980/file-to-byte-in-java
	 * @param fileName
	 * @return
	 */
	public static byte[] readFileToByteArray(String filename) throws IOException {
		File file = new File(filename);
		
	    ByteArrayOutputStream ous = null;
	    InputStream ios = null;
	    
	    try
	    {
	        byte[] buffer = new byte[4096];
	        ous = new ByteArrayOutputStream();
	        ios = new FileInputStream(file);
	        int read = 0;
	        while ((read = ios.read(buffer)) != -1 )
	        {
	            ous.write(buffer, 0, read);
	        }
	    } finally { 
	        try {
	             if (ous != null) 
	                 ous.close();
	        } catch ( IOException e) {
	        }

	        try {
	             if (ios != null) 
	                  ios.close();
	        } catch (IOException e) {
	        }
	    }
	    
	    return ous.toByteArray();
	}
	
	/**
	 * Instead of using XPathAPI.selectSingleNode(root, "name"));
	 * use getChildNode(root, "name"); This is much quicker!
	 * @param root
	 * @param string
	 */
	public static Node getChildNode(Node root, String childName) {
		NodeList nl = root.getChildNodes();
		for (int i = 0; i < nl.getLength(); i++) {
			if (nl.item(i).getNodeName().equals(childName)) {
				return nl.item(i);
			}
		}
		System.err.println("Childnode " + childName + " not found!");
		return null;	
	}	
	
	/**
	 * Instead of using XPathAPI.selectSingleNode(root, "name"));
	 * use getChildNodeValue(root, "name"); This is much quicker!
	 * @param root
	 * @param string
	 */
	public static String getChildNodeValue(Node root, String childName, boolean emptyStringOnMissingChild) {
		NodeList nl = root.getChildNodes();
		
		for (int i = 0; i < nl.getLength(); i++) {
			if (nl.item(i).getNodeName().equals(childName)) {
				Node textNode = nl.item(i).getFirstChild();
				
				if (textNode == null)
					return "";
				else
					return textNode.getNodeValue();
			}
		}
		
		if (emptyStringOnMissingChild)
		{
			return "";
		}
		else
		{
			System.err.println("Childnode " + childName + " not found!");
			return null;
		}
	}

	public static String getChildNodeValue(Node root, String childName)
	{
		return getChildNodeValue(root, childName, false);	
	}
	
	/**
	 * Return a value that tells if the program is run under Windows.
	 * If it is not Windows, Linux is assumed.
	 * @return
	 */
	public static boolean runsOnWindows()
	{
		return System.getProperty("os.name").toUpperCase().contains("WINDOWS");
	}

	/**
	 * Run program after creating a tmp.cmd (Windows) or by using the bash (Linux).
	 * Currently, only Windows is supported!!
	 * TODO: Support Linux by calling bash in a way that the user can see the output.
	 * @param cmdLine
	 * @throws IOException 
	 */
	public static void execute(String cmdLine) throws IOException
	{
		if (runsOnWindows())
		{
			Util.writeFile("flash_tmp.cmd",
					"@echo off\r\n" + 
					cmdLine + "\r\n" +
					"pause\r\n" +
					"exit\r\n");
			Runtime.getRuntime().exec("cmd /c start flash_tmp.cmd");
		}
		else
		{
			cmdLine = cmdLine.replaceAll(" {2,}", " ");
			String[] cmdArray = cmdLine.split(" ");
			Runtime.getRuntime().exec(cmdArray);
		}
	}

	/**
	 * Calculate a brighter or darker version of the originalColor.
	 * @param originalColor
	 * @param brightness The brightness to use in %. 1 = unchanged.
	 */
	public static Color blendColor(Color clOne, Color clTwo, double amount) {
	    float fAmount = (float)amount;
		float fInverse = 1.0f - fAmount;

	    // I had to look up getting color components in java.  Google is good :)
	    float afOne[] = new float[3];
	    clOne.getColorComponents(afOne);
	    float afTwo[] = new float[3]; 
	    clTwo.getColorComponents(afTwo);    

	    float afResult[] = new float[3];
	    afResult[0] = afOne[0] * fAmount + afTwo[0] * fInverse;
	    afResult[1] = afOne[1] * fAmount + afTwo[1] * fInverse;
	    afResult[2] = afOne[2] * fAmount + afTwo[2] * fInverse;

	    return new Color (afResult[0], afResult[1], afResult[2]);
	}
	
	/**
	 * Read a number of bits from a byteArray and return it as ("unsigned") int value.
	 * @param byteArray The array with the bytes to read from.
	 * @param startBit The position of the first bit to read from.
	 *        This needs not to be at byte boundaries.
	 * @param lengthBits The length of the value in bits.
	 * @return
	 */
	public static int getUIntFromByteArray(byte[] byteArray, int startBit, int lengthBits)
	{
		int res = 0;
		
		for (int i = 0; i < lengthBits; i++)
		{
			int byteOffset = (startBit + i) / 8;
			int bitOffset = 7 - ((startBit + i) - byteOffset * 8);
			int dstBitOffset = lengthBits - 1 - i;
			
			int bit = (byteArray[byteOffset] >> bitOffset) & 1;
			
			res += bit << dstBitOffset;
		}
		
		return res;
	}
	
	/**
	 * Read a number of bits from a byteArray and return it as int value.
	 * @param byteArray The array with the bytes to read from.
	 * @param startBit The position of the first bit to read from.
	 *        This needs not to be at byte boundaries.
	 * @param lengthBits The length of the value in bits.
	 * @return
	 */
	public static int getIntFromByteArray(byte[] byteArray, int startBit, int lengthBits)
	{
		long x = getUIntFromByteArray(byteArray, startBit, lengthBits);

		// Decode 2's complement (if you have a nicer version, let us know...).
		String bits = Long.toBinaryString(x);
		
		while (bits.length() < 32)
			bits = "0" + bits;
		
		boolean positive = bits.charAt(32 - lengthBits) == '0';
		
		if (positive)
		{
			return (int)x;
		}
		else
		{
			x ^= 0xFFFFFFFF;
			x = x & ((1 << lengthBits) - 1); // filter data bits without sign bit
			x += 1;
			return (int)-x;
		}
	}
	
	/**
	 * Write an ("unsigned") int value to a byte array at the given position.
	 * @param byteArray The array with the bytes to write to.
	 * @param startBit The position of the first bit to write to.
	 *        This needs not to be at byte boundaries.
	 * @param lengthBits The length of the value in bits.
	 * @return
	 */
	public static void setUIntInByteArray(int value, byte[] byteArray, int startBit, int lengthBits)
	{
		for (int i = 0; i < lengthBits; i++)
		{
			int dstByteOffset = (startBit + i) / 8;
			int dstBitOffset = 7 - ((startBit + i) - dstByteOffset * 8);
			int srcBitOffset = lengthBits - 1 - i;
				
			int bit = (int)((value >> srcBitOffset) & 1);
			int bitMask = 1 << dstBitOffset;
			int bitMaskNeg = 255 - bitMask;
			
			byteArray[dstByteOffset] = (byte)((byteArray[dstByteOffset] & bitMaskNeg) | (bit << dstBitOffset));
		}
	}
	
	/**
	 * Write an (signed) int value to a byte array at the given position.
	 * @param byteArray The array with the bytes to write to.
	 * @param startBit The position of the first bit to write to.
	 *        This needs not to be at byte boundaries.
	 * @param lengthBits The length of the value in bits.
	 * @return
	 */
	public static void setIntInByteArray(int value, byte[] byteArray, int startBit, int lengthBits)
	{
		// Encode 2's complement.
		int uint = (((value >> 31) & 1) << (lengthBits - 1)) | (value & ((1 << (lengthBits - 1)) - 1));
		
		setUIntInByteArray(uint, byteArray, startBit, lengthBits);
	}
	
	final protected static char[] hexArray = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	
	public static String byteToHex(byte b)
	{
		int i = (b + 256) % 256;
		
		return "" + hexArray[i >>> 4] + hexArray[i & 0x0F];
	}

	public static String fillString(char fillChar, int count)
	{  
		char[] chars = new char[count];  
		java.util.Arrays.fill(chars, fillChar);  
		return new String(chars);  
	}  
	
	/**
	 * Create a file with the given length (in bytes). The content will
	 * be 0-bytes.
	 * @param filename The name of the file to create.
	 * @param bytes The length of the file in bytes.
	 * @throws IOException 
	 */
	public static void createBinaryFile(String filename, int bytes) throws IOException
	{
		FileOutputStream fos = new FileOutputStream(filename);
		fos.write(new byte[bytes]);
		fos.close();
	}

	public static void writeFile(String filename, byte[] byteArray) throws IOException
	{
		FileOutputStream fos = new FileOutputStream(filename);
		fos.write(byteArray);
		fos.close();
	}

	public static void writeFile(String filename, String text) throws IOException
	{
		FileWriter outFile = new FileWriter(filename);
		outFile.write(text);
		outFile.close();
	}

	public static boolean isInteger(String s)
	{
	    try { 
	        Integer.parseInt(s); 
	    } catch(NumberFormatException e) { 
	        return false; 
	    }

	    return true;
	}

	public static boolean isIntegerBetween(String s, long minVal, long maxVal)
	{
		if (!isInteger(s))
		{
			return false;
		}
		else
		{
			int i = Integer.parseInt(s);
			return (i >= minVal) && (i <= maxVal);
		}
	}

	/**
	 * Check if the given string is a hex string, i.e. every character
	 * is in the range 0..9, a..f, A..F.
	 * @param text
	 * @return
	 */
	public static boolean isHexString(String text)
	{
		return (text.length() % 2 == 0) && text.replaceAll("[0-9]|[a-f]|[A-F]", "").equals("");
	}

	/**
	 * Transform two hex characters (e.g. "A0") to a byte value.
	 * @param cHigh
	 * @param cLow
	 * @return
	 */
	public static byte hexToByte(Character cHigh, Character cLow)
	{
		return (byte)((Character.digit(cHigh, 16) << 4) + Character.digit(cLow, 16));
	}
	
	public static String arrayListToString(ArrayList<String> al, String delimiter)
	{
		StringBuilder out = new StringBuilder();
		boolean first = true;
		
		for (Object o : al)
		{
			if (first)
			{
				first = false;
			}
			else
			{
				  out.append(delimiter);
			}
			out.append(o.toString());
		}
		
		return out.toString();
	}
	
	/**
	 * Return a string used in functions to represent the bit position for e2p access.
	 * @param offset  The bit offset of the data value.
	 * @param bits    The number of bits per value (relevant for arrays).
	 * @param isArray The information if an array is accessed by using a variable "index".
	 * @return A string like "68 + (uint16_t)index * 1"
	 */
	public static String calcAccessStr(String additionalOffsetPrefix, int offset, int bits, boolean isArray)
	{
		String res;
		
		if (additionalOffsetPrefix.equals(""))
		{
			res = "" + offset;
		}
		else
		{
			res = "" + additionalOffsetPrefix + offset;
		}
		
		if (isArray)
		{
			res += " + (uint16_t)index * " + bits;
		}

		return res;
	}
}
