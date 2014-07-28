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

package shcee.editors;

import javax.swing.JComboBox;

/**
 * ColorComboBox is a JComboBox whose background color can be set.
 * @author uwe
 */
public class ColorComboBox extends JComboBox
{
	private static final long serialVersionUID = 5499448989078147156L;

	public ColorComboBox()
	{
		ComboBoxRenderer renderer = new ComboBoxRenderer();  
	    setRenderer(renderer);
	}
}

class ComboBoxRenderer extends javax.swing.plaf.basic.BasicComboBoxRenderer  
{
	private static final long serialVersionUID = -5650534877807420091L;

	public ComboBoxRenderer()  
	{  
		super();
		setOpaque(true);
	}  
  
  /* To define custom backgrounds inside the list...
  public Component getListCellRendererComponent(JList list, Object value, int index, boolean isSelected, boolean cellHasFocus)  
  {  
	  setText(value.toString());  
	  if (...) setBackground(...);
	  return this;
  } */ 
}
