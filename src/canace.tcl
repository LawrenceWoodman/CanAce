# GUI creation and handling
#
# Copyright (C) 2012 Lawrence Woodman
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
package require xdgbasedir

proc HandleEmuKey {key} {
  switch -- $key {
    F12 {
      ResetAce
      ClearStatusBar
    }
  }
}

# toggle: on or off
# Uses 'xset' so X dependent.
proc KeyAutoRepeat {toggle} {
  if {[string tolower $toggle] eq "on"} {
    exec xset r on
  } else {
    exec xset r off
  }
}

proc ROMLocation {} {
  set searchDirs [concat [XDG::DATA_HOME canace] [XDG::DATA_DIRS canace]]

  foreach dir $searchDirs {
    set romLocation [file join $dir ace.rom]
    if {[file exists $romLocation]} {
      return $romLocation;
    }
  }
  return {};
}

proc SetupACE {} {
  if {![LoadROM [ROMLocation]]} {
    error "Error: Couldn't load the ACE ROM"
  }
}

proc Quit {} {
  KeyAutoRepeat on
  DestroyWindow
}

proc CreateWindow {} {
  wm title . {CanAce}
  wm protocol . WM_DELETE_WINDOW {Quit}
  wm resizable . 0 0
}

proc CreateScreen {} {
  set border [expr {2 * 20 * [GetScale]}]
  set width [expr {([GetScale] * 256) + $border}]
  set height [expr {([GetScale] * 192) + $border}]
  frame .screen -width $width -height $height -background ""
  grid .screen
}

proc CreateStatusBar {} {
  ttk::label .status -textvariable StatusMsg -relief sunken -padding 1m
  grid .status -sticky ew
}

proc ClearStatusBar {} {
  global StatusMsg
  set StatusMsg ""
}

proc GetTapFile {} {
  tk_getOpenFile -filetypes {
    {{Tape file} {.tap .Tap .TAP}}
    {{All files} *}
  } -title "Select a tape image"
}

proc GetSpoolFile {} {
  tk_getOpenFile -filetypes {
    {{Spool file} {.spool .Spool .SPOOL}}
    {{All files} *}
  } -title "Select a text file to spool"
}

proc DisplayAbout {} {
  tk_messageBox -message "CanAce v[GetCanAceVersion]" -type ok
}

proc CreateMenu {} {
  option add *Menu.tearOff 0
  menu .mbar
  . configure -menu .mbar

  .mbar add cascade -label File -menu .mbar.file -underline 0
  .mbar add cascade -label Help -menu .mbar.help -underline 0

  # KeyClear is used below to reset the key modifiers because otherwise the
  # KeyPress command will register an Alt key being pressed to access the menu,
  # but will never see it released.  This would mean all following keys would
  # be ignored by KeyPress.
  menu .mbar.file -postcommand {KeyClear}
  menu .mbar.help -postcommand {KeyClear}

  .mbar.file add command -label "Attach a Tape" -underline 0 -command {
    set filename [GetTapFile]

    if {$filename != ""} {
      TapeAttach $filename
    }
    focus .screen
  }

  .mbar.file add command -label "Spool from File" -underline 0 -command {
    set filename [GetSpoolFile]

    if {$filename != ""} {
      Spool $filename
    }
    focus .screen
  }

  .mbar.file add command -label "Reset" -underline 0 -command {
    ResetAce
    ClearStatusBar
  }

  .mbar.file add command -label "Quit" -underline 0 -command {Quit}

  .mbar.help add command -label "About" -underline 0 -command {DisplayAbout}

}

proc BindEvents {} {
  bind . <KeyPress> {
    KeyPress %K
    HandleEmuKey %K
  }

  bind . <KeyRelease> {KeyRelease %K}
  bind . <FocusIn> {KeyAutoRepeat off}
  bind . <FocusOut> {KeyAutoRepeat on}
}

CreateWindow
CreateMenu
CreateScreen
CreateStatusBar

BindEvents

SetupACE
