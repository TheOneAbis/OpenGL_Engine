A/D to rotate selected body part

Down/Up keys to select body child/parent

Left/Right keys to cycle through the selected body's siblings

I decided to set up some utilities that would serve me very useful once we get into 3D, so I'm using them for this assignment.
Utilities were compiled into a library I called ABCore.lib. Check below for how to link it into your project.

How to link in ABCore:

Grab the ABCore folder from the MyCourses submission and place it directly inside of your solution directory.

- Under additional include directories, add the path to the ABCore folder: example $(SolutionDir)ABCore
- Under additional library directorites, add the path to the folder under ABCore, either x32 or x64 depending on configuration
	- example $(SolutionDir)ABCore\x64 if project is configured for x64. x32 otherwise.

I kept all my code, including original source, in the ABCore folder so feel free to check it out.

Extra note: Line widths are set at 1 px since I'm not using immediate mode, but actual shaders.

Shaders that account for line width are a bit more difficult to do than I originally thought 
(I'd likely either need geometry shaders, which I don't know how to use yet, or fatten the lines in a postprocess pass), thus I left them at 1.