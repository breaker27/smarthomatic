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

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Properties;

import javax.swing.JFrame;
import javax.swing.JSplitPane;
import javax.swing.UIManager;
import javax.xml.transform.TransformerException;

/**
 * mySHCEEMain is the Main class for the SHC EEPROM Editor application.
 * @author: Uwe Freese
 */
public class SHCEEMain extends JFrame {
	
	private static final long serialVersionUID = 1760274466931877539L;
	public static SHCEEMain mySHCEEMain = null;
	
	public static final String EEPROM_METAMODEL_XSD = "e2p_metamodel.xsd";
	public static final String EEPROM_LAYOUT_XML = "e2p_layout.xml";

	public static final String PACKET_METAMODEL_XSD = "packet_metamodel.xsd";
	public static final String PACKET_CATALOG_XML = "packet_layout.xml";

	private static String CFG_FILENAME = "shc_e2p_editor.cfg";

	// Default width and height is used at least until the size is save to a config file.
	public static int propWidth = 900;
	public static int propHeight = 600;
	public static int propDividerLocation = 400;
	public static String propFlashCmd = "";

	public static String version;
	
	private int oldWidth;
	
	public ValueEditorPanel valueEditor;
	public DeviceSelectorPanel deviceSelector;
	private JSplitPane jSplitPane;

	public SHCEEMain()
	{		
		super();
		mySHCEEMain = this;
	
		loadProperties();
		readVersion();
		
		initializeMainFrame();
		
		valueEditor = new ValueEditorPanel();
		deviceSelector = new DeviceSelectorPanel();
		
		jSplitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
		jSplitPane.add(deviceSelector);
		jSplitPane.add(valueEditor);
		
		add(jSplitPane);
		
		ComponentListener resizeListener = new ComponentAdapter(){
		    public void componentResized(ComponentEvent e) {
		    	onResize();
		    }
		};

		addComponentListener(resizeListener);
		
		// Add window listener to detect when window is closed.
		this.addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
				saveProperties();
            }
		});
				
		jSplitPane.setDividerLocation(propDividerLocation);
	}

	/**
	 * Load parameters from a file.
	 */
	private void loadProperties()
	{
		Properties prop = new Properties();
		InputStream input = null;
	 
		try
		{
			input = new FileInputStream(CFG_FILENAME);
			prop.load(input);
			propFlashCmd = prop.getProperty("flash_cmd");
			propWidth = Integer.parseInt(prop.getProperty("width", "" + propWidth));
			propHeight = Integer.parseInt(prop.getProperty("height", "" + propHeight));
			propDividerLocation = Integer.parseInt(prop.getProperty("divider_location", "" + propDividerLocation));
		}
		catch (FileNotFoundException ex)
		{
			// OK - cfg may not exist
		}
		catch (IOException ex)
		{
			ex.printStackTrace();
		}
		finally
		{
			if (input != null)
			{
				try
				{
					input.close();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
		}
	}
	
	/**
	 * Persist parameters to a file.
	 */
	private void saveProperties()
	{
		Properties prop = new Properties();
		OutputStream output = null;

		try
		{
			output = new FileOutputStream(CFG_FILENAME);
			prop.setProperty("flash_cmd", propFlashCmd);
			prop.setProperty("width", "" + this.getWidth());
			prop.setProperty("height", "" + this.getHeight());
			prop.setProperty("divider_location", "" + jSplitPane.getDividerLocation());
			prop.store(output, null);	 
		}
		catch (IOException io)
		{
			io.printStackTrace();
		}
		finally
		{
			if (output != null)
			{
				try
				{
					output.close();
				}
				catch (IOException e)
				{
					e.printStackTrace();
				}
			}
	 
		}
	}

	private void readVersion()
	{
		try
		{
			Properties p = new Properties();
			InputStream is = getClass().getResourceAsStream("/META-INF/maven/org.smarthomatic/shcee/pom.properties");
	        
			if (is != null)
			{
				p.load(is);
				version = p.getProperty("version", "");
			}
		} catch (Exception e)
		{ }

		if (version == null)
		{
			version = "(unofficial developer version)";
		}
	}
	
	private void onResize()
	{
		jSplitPane.setDividerLocation((int)((long)jSplitPane.getDividerLocation() * getWidth() / oldWidth));	
		oldWidth = getWidth();
	}
	
	private void initializeMainFrame()
	{
		setName("SHCEE");
		setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
		setTitle("smarthomatic EEPROM Editor");
		
		Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		setSize(propWidth, propHeight);
		setLocation((screenSize.width - propWidth) / 2, (screenSize.height - propHeight) / 2);
		oldWidth = propWidth;
	}

	/**
	 * Starts the application.
	 * @param args an array of command-line arguments
	 */
	public static void main(java.lang.String[] args)
	{
		if ((args.length == 1) && args[0].equals("/gen"))
		{	
			try
			{
				new SourceCodeGeneratorE2P();
			} catch (TransformerException e1)
			{
				e1.printStackTrace();
			} catch (IOException e1)
			{
				e1.printStackTrace();
			}
			
			try
			{
				new SourceCodeGeneratorPacket();
			} catch (Exception e1)
			{
				e1.printStackTrace();
			}
		}
		else
		{
			/* Set native look and feel */
			try
			{
				UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
			
			new SHCEEMain();
			mySHCEEMain.setVisible(true);
		}
	}

	/**
	 * Show the content of the given EEPROM file on the right side for editing.
	 * @param valueAt
	 */
	public void loadE2pFile(String filename)
	{
		valueEditor.loadE2pFile(filename);
	}
}
