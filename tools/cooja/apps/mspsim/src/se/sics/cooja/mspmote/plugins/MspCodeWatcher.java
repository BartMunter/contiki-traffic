/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

package se.sics.cooja.mspmote.plugins;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Observable;
import java.util.Observer;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.Box;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTabbedPane;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import javax.swing.filechooser.FileFilter;
import javax.swing.plaf.basic.BasicComboBoxRenderer;

import org.apache.log4j.Logger;
import org.jdom.Element;

import se.sics.cooja.ClassDescription;
import se.sics.cooja.GUI;
import se.sics.cooja.GUI.RunnableInEDT;
import se.sics.cooja.Mote;
import se.sics.cooja.MotePlugin;
import se.sics.cooja.PluginType;
import se.sics.cooja.Simulation;
import se.sics.cooja.VisPlugin;
import se.sics.cooja.Watchpoint;
import se.sics.cooja.WatchpointMote;
import se.sics.cooja.WatchpointMote.WatchpointListener;
import se.sics.cooja.mspmote.MspMote;
import se.sics.cooja.mspmote.MspMoteType;
import se.sics.mspsim.core.EmulationException;
import se.sics.mspsim.ui.DebugUI;
import se.sics.mspsim.util.DebugInfo;
import se.sics.mspsim.util.ELFDebug;

@ClassDescription("Msp Code Watcher")
@PluginType(PluginType.MOTE_PLUGIN)
public class MspCodeWatcher extends VisPlugin implements MotePlugin {
  private static final long serialVersionUID = -8463196456352243367L;

  private static final int SOURCECODE = 0;
  private static final int BREAKPOINTS = 2;

  private static Logger logger = Logger.getLogger(MspCodeWatcher.class);
  private Simulation simulation;
  private Observer simObserver;

  private File currentCodeFile = null;
  private int currentLineNumber = -1;

  private DebugUI assCodeUI;
  private CodeUI sourceCodeUI;
  private BreakpointsUI breakpointsUI;

  private MspMote mspMote; /* currently the only supported mote */
  private WatchpointMote watchpointMote;
  private WatchpointListener watchpointListener;

  private JComboBox fileComboBox;
  private String[] debugInfoMap = null;
  private File[] sourceFiles;

  private JTabbedPane mainPane;

  /**
   * Mini-debugger for MSP Motes.
   * Visualizes instructions, source code and allows a user to manipulate breakpoints.
   *
   * @param mote MSP Mote
   * @param simulationToVisualize Simulation
   * @param gui Simulator
   */
  public MspCodeWatcher(Mote mote, Simulation simulationToVisualize, GUI gui) {
    super("Msp Code Watcher - " + mote, gui);
    simulation = simulationToVisualize;
    this.mspMote = (MspMote) mote;
    this.watchpointMote = (WatchpointMote) mote;

    getContentPane().setLayout(new BorderLayout());

    /* Create source file list */
    fileComboBox = new JComboBox();
    fileComboBox.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        sourceFileSelectionChanged();
      }
    });
    fileComboBox.setRenderer(new BasicComboBoxRenderer() {
      private static final long serialVersionUID = -2135703608960229528L;
      public Component getListCellRendererComponent(JList list, Object value,
          int index, boolean isSelected, boolean cellHasFocus) {
        if (isSelected) {
          setBackground(list.getSelectionBackground());
          setForeground(list.getSelectionForeground());
          if (index > 0) {
            list.setToolTipText(sourceFiles[index-1].getPath());
          }
        } else {
          setBackground(list.getBackground());
          setForeground(list.getForeground());
        }
        setFont(list.getFont());
        setText((value == null) ? "" : value.toString());
        return this;
      }
    });
    updateFileComboBox();

    /* Browse code control (north) */
    Box sourceCodeControl = Box.createHorizontalBox();
    sourceCodeControl.add(new JButton(stepAction));
    sourceCodeControl.add(Box.createHorizontalStrut(10));
    sourceCodeControl.add(new JLabel("Current location: "));
    sourceCodeControl.add(new JButton(currentFileAction));
    sourceCodeControl.add(Box.createHorizontalGlue());
    sourceCodeControl.add(new JLabel("Source files: "));
    sourceCodeControl.add(fileComboBox);
    sourceCodeControl.add(Box.createHorizontalStrut(5));
    sourceCodeControl.add(new JButton(mapAction));
    sourceCodeControl.add(Box.createHorizontalStrut(10));

    /* Execution control panel (south of source code panel) */

    /* Layout */
    mainPane = new JTabbedPane();

    sourceCodeUI = new CodeUI(watchpointMote);
    JPanel sourceCodePanel = new JPanel(new BorderLayout());
    sourceCodePanel.add(BorderLayout.CENTER, sourceCodeUI);
    sourceCodePanel.add(BorderLayout.SOUTH, sourceCodeControl);
    mainPane.addTab("Source code", null, sourceCodePanel, null); /* SOURCECODE */

    assCodeUI = new DebugUI(this.mspMote.getCPU(), true);
    for (Component c: assCodeUI.getComponents()) {
      c.setBackground(Color.WHITE);
    }
    mainPane.addTab("Instructions", null, assCodeUI, null);

    breakpointsUI = new BreakpointsUI(mspMote, this);
    mainPane.addTab("Breakpoints", null, breakpointsUI, "Right-click source code to add"); /* BREAKPOINTS */

    add(BorderLayout.CENTER, mainPane);

    /* Listen for breakpoint changes */
    watchpointMote.addWatchpointListener(watchpointListener = new WatchpointListener() {
      public void watchpointTriggered(final Watchpoint watchpoint) {
        SwingUtilities.invokeLater(new Runnable() {
          public void run() {
            logger.info("Watchpoint triggered: " + watchpoint);
            if (simulation.isRunning()) {
              return;
            }
            breakpointsUI.selectBreakpoint(watchpoint);
            sourceCodeUI.updateBreakpoints();
            showCurrentPC();
          }
        });
      }
      public void watchpointsChanged() {
        sourceCodeUI.updateBreakpoints();
      }
    });

    /* Observe when simulation starts/stops */
    simulation.addObserver(simObserver = new Observer() {
      public void update(Observable obs, Object obj) {
        if (!simulation.isRunning()) {
          stepAction.setEnabled(true);
          showCurrentPC();
        } else {
          stepAction.setEnabled(false);
        }
      }
    });

    setSize(750, 500);
    showCurrentPC();
  }

  private void updateFileComboBox() {
    sourceFiles = getSourceFiles(mspMote, debugInfoMap);
    fileComboBox.removeAllItems();
    fileComboBox.addItem("[view sourcefile]");
    for (File f: sourceFiles) {
      fileComboBox.addItem(f.getName());
    }
    fileComboBox.setSelectedIndex(0);
  }

  public void displaySourceFile(final File file, final int line, final boolean markCurrent) {
    SwingUtilities.invokeLater(new Runnable() {
      public void run() {
        mainPane.setSelectedIndex(SOURCECODE); /* code */
        sourceCodeUI.displayNewCode(file, line, markCurrent);
      }});
  }

  private void sourceFileSelectionChanged() {
    int index = fileComboBox.getSelectedIndex();
    if (index <= 0) {
      return;
    }

    File selectedFile = sourceFiles[index-1];
    displaySourceFile(selectedFile, -1, false);
  }

  private void showCurrentPC() {
    /* Instructions */
    assCodeUI.updateRegs();
    assCodeUI.repaint();

    /* Source */
    updateCurrentSourceCodeFile();
    File file = currentCodeFile;
    Integer line = currentLineNumber;
    if (file == null || line == null) {
      currentFileAction.setEnabled(false);
      currentFileAction.putValue(Action.NAME, "[unknown]");
      currentFileAction.putValue(Action.SHORT_DESCRIPTION, null);
      return;
    }
    currentFileAction.setEnabled(true);
    currentFileAction.putValue(Action.NAME, file.getName() + ":" + line);
    currentFileAction.putValue(Action.SHORT_DESCRIPTION, file.getAbsolutePath() + ":" + line);
    fileComboBox.setSelectedItem(file.getName());

    displaySourceFile(file, line, true);
  }

  public void closePlugin() {
    watchpointMote.removeWatchpointListener(watchpointListener);
    watchpointListener = null;

    simulation.deleteObserver(simObserver);
    simObserver = null;

    /* TODO XXX Unregister breakpoints? */
  }

  private void updateCurrentSourceCodeFile() {
    currentCodeFile = null;

    try {
      ELFDebug debug = ((MspMoteType)mspMote.getType()).getELF().getDebug();
      if (debug == null) {
        return;
      }
      int pc = mspMote.getCPU().getPC();
      DebugInfo debugInfo = debug.getDebugInfo(pc);
      if (debugInfo == null) {
        if (pc != 0) {
          logger.warn("No sourcecode info at " + String.format("0x%04x", mspMote.getCPU().getPC()));
        }
        return;
      }

      String path = new File(debugInfo.getPath(), debugInfo.getFile()).getPath();
      if (path == null) {
        return;
      }
      path = path.replace('\\', '/');

      /* Debug info to source file map */
      if (debugInfoMap != null && debugInfoMap.length == 2) {
        if (path.startsWith(debugInfoMap[0])) {
          path = debugInfoMap[1] + path.substring(debugInfoMap[0].length());
        }
      }

      /* Nasty Cygwin-Windows fix */
      if (path.length() > 10 && path.startsWith("/cygdrive/")) {
        char driveCharacter = path.charAt(10);
        path = driveCharacter + ":/" + path.substring(11);
      }

      currentCodeFile = new File(path).getCanonicalFile();
      currentLineNumber = debugInfo.getLine();
    } catch (Exception e) {
      logger.fatal("Exception: " + e.getMessage(), e);
      currentCodeFile = null;
      currentLineNumber = -1;
    }
  }

  private void tryMapDebugInfo() {
    final String[] debugFiles;
    try {

      ELFDebug debug = ((MspMoteType)mspMote.getType()).getELF().getDebug();
      debugFiles = debug != null ? debug.getSourceFiles() : null;
      if (debugFiles == null) {
        logger.fatal("Error: No debug information is available");
        return;
      }
    } catch (IOException e1) {
      logger.fatal("Error: " + e1.getMessage(), e1);
      return;
    }
    String[] map = new RunnableInEDT<String[]>() {
      public String[] work() {
        /* Select which source file to use */
        int counter = 0, n;
        File correspondingFile = null;
        while (true) {
          n = JOptionPane.showOptionDialog(GUI.getTopParentContainer(),
              "Choose which source file to manually locate.\n\n" +
              "Some source files may not exist, as debug info is also inherited from the toolchain.\n" +
              "\"Next File\" proceeds to the next source file in the debug info.\n\n" +
              debugFiles[counter] + " (" + (counter+1) + '/' + debugFiles.length + ')',
              "Select source file to locate", JOptionPane.YES_NO_CANCEL_OPTION,
              JOptionPane.QUESTION_MESSAGE, null,
              new String[] { "Next File", "Locate File", "Cancel"}, "Next File");
          if (n == JOptionPane.CANCEL_OPTION || n == JOptionPane.CLOSED_OPTION) {
            return null;
          }
          if (n == JOptionPane.NO_OPTION) {
            /* Locate file */
            final String filename = new File(debugFiles[counter]).getName();
            JFileChooser fc = new JFileChooser();
            fc.setFileFilter(new FileFilter() {
              public boolean accept(File file) {
                if (file.isDirectory()) { return true; }
                if (file.getName().equals(filename)) {
                  return true;
                }
                return false;
              }
              public String getDescription() {
                return "Source file " + filename;
              }
            });
            fc.setCurrentDirectory(new File(GUI.getExternalToolsSetting("PATH_CONTIKI", ".")));
            int returnVal = fc.showOpenDialog(GUI.getTopParentContainer());
            if (returnVal == JFileChooser.APPROVE_OPTION) {
              correspondingFile = fc.getSelectedFile();
              break;
            }
          }

          if (n == JOptionPane.YES_OPTION) {
            /* Next file */
            counter = (counter+1) % debugFiles.length;
          }
        }

        /* Match files */
        try {
          String canonDebug = debugFiles[counter];
          String canonSelected = correspondingFile.getCanonicalFile().getPath().replace('\\', '/');

          int offset = 0;
          while (canonDebug.regionMatches(
              true,
              canonDebug.length()-offset,
              canonSelected, canonSelected.length()-offset,
              offset)) {
            offset++;
            if (offset >= canonDebug.length() ||
                offset >= canonSelected.length())
              break;
          }
          offset--;
          String replace = canonDebug.substring(0, canonDebug.length() - offset);
          String replacement = canonSelected.substring(0, canonSelected.length() - offset);

          {
            JTextField replaceInput = new JTextField(replace);
            replaceInput.setEditable(true);
            JTextField replacementInput = new JTextField(replacement);
            replacementInput.setEditable(true);

            Box box = Box.createVerticalBox();
            box.add(new JLabel("Debug info file:"));
            box.add(new JLabel(canonDebug));
            box.add(new JLabel("Selected file:"));
            box.add(new JLabel(canonSelected));
            box.add(Box.createVerticalStrut(20));
            box.add(new JLabel("Replacing:"));
            box.add(replaceInput);
            box.add(new JLabel("with:"));
            box.add(replacementInput);

            JOptionPane optionPane = new JOptionPane();
            optionPane.setMessage(box);
            optionPane.setMessageType(JOptionPane.INFORMATION_MESSAGE);
            optionPane.setOptions(new String[] { "OK" });
            optionPane.setInitialValue("OK");
            JDialog dialog = optionPane.createDialog(
                GUI.getTopParentContainer(),
            "Mapping debug info to real sources");
            dialog.setVisible(true);

            replace = replaceInput.getText();
            replacement = replacementInput.getText();
          }

          replace = replace.replace('\\', '/');
          replacement = replacement.replace('\\', '/');
          return new String[] { replace, replacement };
        } catch (IOException e) {
          logger.fatal("Error: " + e.getMessage(), e);
          return null;
        }
      }
    }.invokeAndWait();

    if (map != null) {
      debugInfoMap = map;
      updateFileComboBox();
    }
  }

  private static File[] getSourceFiles(MspMote mote, String[] map) {
    final String[] sourceFiles;
    try {
      ELFDebug debug = ((MspMoteType)mote.getType()).getELF().getDebug();
      sourceFiles = debug != null ? debug.getSourceFiles() : null;
      if (sourceFiles == null) {
        logger.fatal("Error: No debug information is available");
        return new File[0];
      }
    } catch (IOException e1) {
      logger.fatal("Error: " + e1.getMessage(), e1);
      return null;
    }
    File contikiSource = mote.getType().getContikiSourceFile();
    if (contikiSource != null) {
      try {
        contikiSource = contikiSource.getCanonicalFile();
      } catch (IOException e1) {
      }
    }

    /* Verify that files exist */
    ArrayList<File> existing = new ArrayList<File>();
    for (String sourceFile: sourceFiles) {

      /* Debug info to source file map */
      sourceFile = sourceFile.replace('\\', '/');
      if (map != null && map.length == 2) {
        if (sourceFile.startsWith(map[0])) {
          sourceFile = map[1] + sourceFile.substring(map[0].length());
        }
      }

      /* Nasty Cygwin-Windows fix */
      if (sourceFile.length() > 10 && sourceFile.contains("/cygdrive/")) {
        char driveCharacter = sourceFile.charAt(10);
        sourceFile = driveCharacter + ":/" + sourceFile.substring(11);
      }

      File file = new File(sourceFile);
      try {
        file = file.getCanonicalFile();
      } catch (IOException e1) {
      }
      if (!GUI.isVisualizedInApplet()) {
        if (file.exists() && file.isFile()) {
          existing.add(file);
        } else {
          /*logger.warn("Can't locate source file, skipping: " + file.getPath());*/
        }
      } else {
        /* Accept all files without existence check */
        existing.add(file);
      }
    }

    /* If no files were found, suggest map function */
    if (sourceFiles.length > 0 && existing.isEmpty() && GUI.isVisualized()) {
      new RunnableInEDT<Boolean>() {
        public Boolean work() {
          JOptionPane.showMessageDialog(
              GUI.getTopParentContainer(),
              "The firmware debug info specifies " + sourceFiles.length + " source files.\n" +
              "However, Msp Code Watcher could not find any of these files.\n" +
              "Make sure the source files were not moved after the firmware compilation.\n" +
              "\n" +
              "If you want to manually locate the sources, click \"Map\" button.",
              "No source files found",
              JOptionPane.WARNING_MESSAGE);
          return true;
        }
      }.invokeAndWait();
    }

    /* Sort alphabetically */
    ArrayList<File> sorted = new ArrayList<File>();
    for (File file: existing) {
      int index = 0;
      for (index=0; index < sorted.size(); index++) {
        if (file.getName().compareToIgnoreCase(sorted.get(index).getName()) < 0) {
          break;
        }
      }
      sorted.add(index, file);
    }

    /* Add Contiki source first */
    if (contikiSource != null && contikiSource.exists()) {
      sorted.add(0, contikiSource);
    }

    return sorted.toArray(new File[0]);
  }

  public Collection<Element> getConfigXML() {
    ArrayList<Element> config = new ArrayList<Element>();
    Element element;

    element = new Element("tab");
    element.addContent("" + mainPane.getSelectedIndex());
    config.add(element);

    return config;
  }

  public boolean setConfigXML(Collection<Element> configXML, boolean visAvailable) {
    for (Element element : configXML) {
      if (element.getName().equals("tab")) {
        mainPane.setSelectedIndex(Integer.parseInt(element.getText()));
      }
    }
    return true;
  }

  private AbstractAction currentFileAction = new AbstractAction() {
    private static final long serialVersionUID = -3218306989816724883L;

    public void actionPerformed(ActionEvent e) {
      if (currentCodeFile == null) {
        return;
      }
      displaySourceFile(currentCodeFile, currentLineNumber, true);
    }
  };

  private AbstractAction mapAction = new AbstractAction("Locate sources") {
    private static final long serialVersionUID = -3929432830596292495L;

    public void actionPerformed(ActionEvent e) {
      tryMapDebugInfo();
    }
  };

  private AbstractAction stepAction = new AbstractAction("Step instruction") {
    private static final long serialVersionUID = 3520750710757816575L;
    public void actionPerformed(ActionEvent e) {
      try {
        mspMote.getCPU().stepInstructions(1);
      } catch (EmulationException ex) {
        logger.fatal("Error: ", ex);
      }
      showCurrentPC();
    }
  };

  public Mote getMote() {
    return mspMote;
  }

}
