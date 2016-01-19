
Application is decomposed in 3 main parts.

1. Main application window class designed to frame solution into UI of operating system.
2. Viewer widget class -- a reusable component designed to encapsulate
   scene view and controls UI. It can be used later as a part of larger system.
3. 3D Scene widget class encapsulates opengl-related details of implementation.

Camera class holds state and exposes interface to manipulate position and view angles.
Please see structure.png diagram attached.

I've built it with '-rpath=\\\$$ORIGIN/../lib:\\\$$ORIGIN' and put Qt libs and plugins.
So its going to run on debians (with required mesa/libGL.so.1).


Known issue.
------------
"Measuring tool" functionality is not perfect
because 'unproject' routine for ray casting technique is missing something,
which I haven't figured out. Require time to dive into more theory about that.


Scalability considerations.
---------------------------

There're two complexity issues:
1. Spacial complexity: loading and keeping large datasets in RAM.
2. Time complexity: searching for closest point in O(logN) (Measuring tool).
   Current search is trivial linear scan.

Both of these issues can be solved with partitioning large datasets with BSP tree or quadtree aproach,
and dynamically loading/drawing chunks depending on current camera position.
