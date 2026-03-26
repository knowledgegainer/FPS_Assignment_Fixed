FPS Assignment (Unreal Engine 5.6)

Overview

This project is a simple FPS-based system built in Unreal Engine 5 using C++.

The main objective was to:

* Spawn enemies from a JSON file( with given location, rotaion and scale)
* Assign properties like health, score, and color dynamically
* Allow the player to shoot and destroy enemies
* Update score using a UI widget

Features

* JSON-based enemy spawning system
* Dynamic enemy properties (Health, Score, Color)
* Shooting system using line trace/projectiles
* Score UI using UMG
* Basic gameplay loop

How It Works

* JSON file contains enemy types and object data
* Types define: Health, Score, Color
* Objects define: Type reference, Transform

The game reads this JSON at runtime and spawns enemies accordingly.

How to Run

1. Open the project in Unreal Engine 5.6
2. Click Play (The correct level is set as default and loads automatically)

Challenges Faced

* Initially faced issues with incorrect field names, missing keys, and data mismatches between JSON and C++ (e.g., using "name" instead of "type", incorrect transform parsing).
* Encountered multiple build failures due to missing modules, incorrect includes

Solutions

* Carefully matched JSON structure with C++ parsing logic
* Used debug messages to verify each step
* Ensured consistent key usage like "type", "transform", "location"


Notes

* Some UI elements from the template are still present
* This project focuses on functionality over polish due to time constraints
