package shcee.editors;

import javax.swing.JComboBox;

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
