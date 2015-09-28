﻿#summary Top level MicroGear page

# Introduction #

MicroGear is an open-source embedded aerial robotics application written in C/C++. MicroGear is designed to be robust, highly configurable, open, interfaceable, and feature rich. Along with high levels of capability, MicroGear focuses on robustness, high performance, and minimal memory and disk footprints.

MicroGear is originally derived from the Xbow "MNAV" open-source autopilot. It has since incorporated a number of source code modules and design and structural concepts from the FlightGear project. The design focus of MicroGear is to offer a rich set of high functioning features and tools, rather than to write a minimilistic bare metal application for a specific processor and a specific application.

MicroGear has powered many hours of autonomous flight in a variety of airframes. It is written with a strong focus on safety and robustness.


# Status #

As of Nov 13, 2007 the first public release is available. This release is capable of sustained route following and reliable altitude hold. The license terms are GPL.


# Images #

![![](http://microgear.googlecode.com/svn/wiki/MicroGear.attach/ground-station1-small.jpg)](http://microgear.googlecode.com/svn/wiki/MicroGear.attach/ground-station1.jpg)

![![](http://microgear.googlecode.com/svn/wiki/MicroGear.attach/racetrack-small.png)](http://microgear.googlecode.com/svn/wiki/MicroGear.attach/racetrack.png)

![![](http://microgear.googlecode.com/svn/wiki/MicroGear.attach/tm1-small.jpg)](http://microgear.googlecode.com/svn/wiki/MicroGear.attach/tm1.jpg)

![![](http://microgear.googlecode.com/svn/wiki/MicroGear.attach/tm2-small.jpg)](http://microgear.googlecode.com/svn/wiki/MicroGear.attach/tm2.jpg)


# Features #

Here is a quick summary of the current feature set:

  * Altitude hold, pitch angle, and rate of climb hold modes.
  * Wing leveler and arbitrary bank angle hold modes.
  * Speed hold by manipulating elevator or throttle.
  * Altitude hold by manipulating throttle or elevator.
  * Waypoint following with "unlimited" route lengths. (Limited only by size of available RAM and flash storage.)
  * Includes many design concepts and structures from the FlightGear project.
  * Derives some code from MNAV v1.4 code (see http://www.sf.net/projects/micronav)
  * The code has been organized to into related functionality groups so it can be navigated and understood more quickly.
  * The code is written in C/C++ and compiles on a variety of linux and posix systems.
  * Includes automake/autoconf configurations to automate and streamline the build process.
  * Transmitter inputs and autopilot control surface outputs can be logged.
  * A console link has been implemented that sends data over the console line in a robust manner with markers for the start of a packet and strong check summing. This enables communication over longer range (but slower) radio modems.
  * Primary target is the GumStix, but the code should run on any linux platform.
  * New FlightGear style property state tree incorporated into the code structure.
  * XML config file parser (interfaces to the property state tree.)
  * Autopilot configurations are defined in XML files and can be quickly changed on the fly without recompiling any code.