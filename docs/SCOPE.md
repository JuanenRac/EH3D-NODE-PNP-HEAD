# EH3D-NODE-PNP-HEAD: scope

Status: **simulator**. EH3D-NODE-PNP-HEAD is the logic of a node for the control of the head of a pick-and-place machine: the vacuum pump and the valve of each nozzle with a pressure sensor, the puff of air that lets a part go, the Z axis and the rotation of each nozzle, the two lights, and the interlocks that keep a part from being dropped or a nozzle from being turned where it could hit something. It is written in C++17 for the ESP32-S3 and is tried today only on a computer, against a simulator. It has not been built into firmware, flashed or connected to anything.

## Purpose

The node that will stand between the pick-and-place program of the suite (EH3D-PHOENIX-PNP), through the server, and the real head of a machine. The program plans and describes; the node carries out one small, checked action at a time and refuses what is unsafe.

It is for the assembly of boards and of their components. It is not for taking up objects, for arms or for robots.

## What it does

* Opens the valve of one nozzle, waits for the vacuum and tells a part held from a pick that found nothing; tells a part that was lost while held, and holds a fault for it.
* Releases a part with a short puff of air, only when the nozzle is low enough; turns a nozzle only when it is high enough, and does not lower a nozzle that is turning.
* Moves the Z axis and the rotation of each nozzle inside the limits of the settings, at the speeds of the settings.
* Runs the pump only while a valve is open and never longer than a set time, and stops it and closes the valves for a pump that runs too long.
* Reads one pressure sensor per nozzle through a filter, turns it into kPa with a zero and a scale, calls a sensor dead when it does not change or reads what cannot be real, and calibrates the zero and the scale when the settings allow.
* Sets two lights (the camera looking up and the one looking down) in percent.
* Answers over HTTP with the words of the other nodes of the suite: a status, one nozzle, pick, place, blow, Z, rotate, light, stop, acknowledge and calibrate, all behind a token.
* Goes to a safe state by itself when the server is not heard: the pump stops, the valves close, the nozzles go up and the lights go out.
* Comes with a simulator of the head (a pump and valves that make a vacuum, parts that seal a nozzle or not, sensors with noise) so that all of this is tried on a computer with a fixed seed.

## What it will not do

* No decision about the limits of a machine, its interlocks or its emergency stop (see below).
* No handling of objects, no arms and no robots: this is the head of a machine that places components on boards.
* No camera: the camera and the ring of light that look at the part belong to the camera node of the head; this node only has the two lights of the head.
* No driver for a pump, a valve, a sensor or a motor yet: that is the firmware, which is not written.

## What it never decides

The stop, the limits of the axes, the interlocks and the emergency stop of the machine belong to the machine and its controller. The interlocks and the safe state of this node are a second line behind those, they only ever make a move refused or slower, and they never replace them. A change that would relax one after a trouble is a defect.

## Where it sits

* **EH3D-PHOENIX-PNP** plans the jobs and describes the nodes; its register already waits for this node.
* **EH3D-SERVER** has a plugin for pick-and-place machines that today simulates them; this node is what will stand behind it.
* **EH3D-COMMON** holds the firmware base (the plain helpers of every node) that this repository takes ten files from.
* **EH3D-NODE-PNP-FEEDER** is the other half: the node of the feeders.
