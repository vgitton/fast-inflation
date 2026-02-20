# About the documentation

[TOC]

The overall capabilities of the code are described in the [readme](doxygen/readme.md). 

Note that the documentation refers to the underlying article for the mathematical notation.
It is recommended to be familiar with the article, and in particular, with its supplemental material, to understand the underlying mathematical strcture before reading the documentation.
This does not necessarily apply to the simplest and most high-level functions of the code, of course.

### How to read this documentation?

This documentation is organized in [modules](modules.html).
A module is a group of classes and functions together with explanative text that, conceptually, aim at achieving a specific goal.
Thus, it is recommended to browse the [modules](modules.html) page, and depending on one's interest, to read up about the different modules.

### Recommended starting point

The simplest application to start from is probably user::srb_vis_222_weak, see user::srb_vis_222_weak::run().
This is also the fastest application to run because of the low numbers of variables (associated to inflation events) and constraints.
To reproduce the proof of nonlocality of the Elegant Joint Measurement, please refer to the \ref ejm "EJM module".

