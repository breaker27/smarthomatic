/*
* This file is part of smarthomatic, http://www.smarthomatic.org.
* Copyright (c) 2014 Uwe Freese
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

import java.util.Dictionary;
import java.util.Hashtable;

import javax.xml.transform.TransformerException;

/**
 * This class remembers enum definitions to allow detecting "incompatible" ones.
 * The concept for enum definitions in generated source code is that they can be
 * defined more than once (also in the XML), as long as the definitions are identical.
 * This concept was chosen to aviod the necessity to support dependencies/references
 * between enums in XML and source code files.
 * Enum definitions in the generated source code files are surrounded by #ifdef guards
 * so the first definition encountered during compilation is taken. This is ok
 * as long as every further definitions are identical anyways. 
 * @author Uwe
 */
public class EnumDuplicateChecker
{
	private static Hashtable<String, Hashtable<String, String>> enums;
	
	/**
	 * Return if the given enum value definition is "ok" for the generated source code.
	 * This means it is either unique (new), or the defined value is the same as defined before.
	 */
	public static void checkEnumDefinition(String enumName, Hashtable<String, String> enumValues)
			throws TransformerException
	{
		if (enums == null)
		{
			enums = new Hashtable<String, Hashtable<String, String>>();
		}
		
		if (!enums.containsKey(enumName))
		{
			enums.put(enumName, enumValues);
		}
		else // check against previous definition
		{
			Hashtable<String, String> enumValuesOld = enums.get(enumName);
			
			// exit if previous definition was different of non existent
			for (String key : enumValues.keySet())
			{
				if (!enumValuesOld.containsKey(key))
				{
					System.out.println("WARNING: Previously defined enum " + enumName +
							" did not contain the value " + key +
							". This could lead to problems depending on which header files are included.");
				}
				else if (!enumValuesOld.get(key).equals(enumValues.get(key)))
				{
					throw new TransformerException("Previously defined enum " + enumName +
							" defined value " + key + "=" + enumValuesOld.get(key) +
							" whereas new definition defines value " + key + "=" + enumValues.get(key) +
							". Either make value definitons identical or name them different!");
				}
			}
			
			// exit if previous definition was different of non existent
			for (String key : enumValuesOld.keySet())
			{
				if (!enumValues.containsKey(key))
				{
					System.out.println("WARNING: New defined enum " + enumName +
							" does not contain the value " + key +
							", which was defined in an enum with the same name before. " +
							"This could lead to problems depending on which header files are included.");
				}
				else if (!enumValues.get(key).equals(enumValuesOld.get(key)))
				{
					throw new TransformerException("Previously defined enum " + enumName +
							" defined value " + key + "=" + enumValuesOld.get(key) +
							" whereas new definition defines value " + key + "=" + enumValues.get(key) +
							". Either make value definitons identical or name them different!");
				}
			}
		}
	}
}
