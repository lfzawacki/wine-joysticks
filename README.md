wine-joysticks
==============

Joystick utilities for Wine. GSoC 2012 project for Wine.

For more info look at: <http://lfzawacki.heroku.com/wine/published>

Use make to build, you may need to install the mingw cross compiler and edit the makefile for the wine includes path.

## Tools

### Command line joystick tester
#### Status: Usable

	* To test run joystick.exe with -h for help
	* Use this to test list joystick, test if they work, test axis mapping, search for dinput traces...
	* The script joystick_drivers uses this to print the linux interface where each joystick is being read.

### Command line force feedback tester
#### Status: initial support

	* To test run forcefeedback.exe
	* Just looks for FF capable devices and rumbles one of them

### GUI
#### Status: To be done