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

package shcee;

import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.LayoutManager;

import javax.swing.BoxLayout;
import javax.swing.JPanel;

public class ValueEditorBlockContainer extends JPanel
{
	private static final long serialVersionUID = 3625756713568969823L;

	private JPanel spacer;
	
	public ValueEditorBlockContainer()
	{
		super();
		
		LayoutManager layout = new BoxLayout(this, javax.swing.BoxLayout.Y_AXIS);
		setLayout(layout);
		
		// Add a spacer to the end of the panel so that the block panels don't blow
		// up in height if the scrollpane is big.
		spacer = new JPanel();
		spacer.setPreferredSize(new Dimension(1000,100));
		//spacer.setMinimumSize(new Dimension(1,1));
		add(spacer);
	}
	
	@Override
	public Dimension getPreferredSize()
	{
		Container myScrollPane = this.getParent();
		
		int sH = myScrollPane.getHeight();
		
		// TODO: (Only) if the blocks have a smaller height than the scroll pane,
		// add a spacer element.
		
		//int height = this.getHeight();

		int pheight = super.getPreferredSize().height;
		
		if (sH > pheight)
		{
			//spacer.setPreferredSize(new Dimension(10000,10000));
		}
		else
		{
			//spacer.setPreferredSize(new Dimension(1,1));
		}
		
	    return new Dimension(10, pheight);
	}

	/**
	 * Add a component not at the end, but just before the spacer JPanel,
	 * which is always at the end in the ValueEditorBlockContainer to fill
	 * up the rest of the free space in the JScollPane.
	 */
	@Override
	public Component add(Component comp)
	{
		return super.add(comp, this.getComponentCount() - 1);
	}	
}
