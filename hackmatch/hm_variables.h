#pragma once

// GUI
MACRO_CONFIG_FLOAT(dpi, 100.f)               // In %, used for changing evertything related to GUI's size
MACRO_CONFIG_FLOAT(animSpeed, 0.08f)
MACRO_CONFIG_FLOAT(sliderThickness, 1.f)     // Note: default = 1.f, smaller < 1.f, bigger > 1.f
MACRO_CONFIG_FLOAT(widgetOffset, 8.f)        // Note: Used for offseting widgets inside child windows
MACRO_CONFIG_FLOAT(childWindowOffset, 10.f)  // Note: Offsets child windows so that windows inside the main one, are not crammed next to each other
MACRO_CONFIG_FLOAT(widgetSpacing, 2.f);      // Note: Used for spacing between widgets(i.e. toggle button, slider, etc.)
MACRO_CONFIG_FLOAT(titleBarSize, 20.f);      // Note: Used for title bar height, optimal size is 20.f

// ESP
MACRO_CONFIG_FLOAT(shadingFactor, 0.5f);     // Note: Used for ESP shading
