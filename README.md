# linux_drm_test 

A simple project for kernel mode setting and retrieving a framebuffer for drawing in Linux. The entire process from learning from scratch to creating and optimizing the project took just over 24 hours.

## Motivation  
* Learn about the graphics ecosystem in Linux.
* Learn about graphics software and practices (e.g. Cairo).
* Gain an understanding how other programs might work, so I can use this experience as a starting point to create more things (see below).

## Process  

### First Source    
Searching for a learning source, I stumbled upon [this guide](http://betteros.org/tut/graphics1.php#dumb) at [betterOS.org](http://betterOS.org). I followed the guide which went through the process of fetching graphics card resources (e.g. connectors), as well as the process of creating and mapping a framebuffer through kernel mode setting. The guide led me to a simple understanding of the process of getting a frame buffer to draw to.   

### Improvement  
How I improved on what I learned from BetterOS's guide.

#### Fixed Vs. Dynamic Memory    
The BetterOS guide mentioned some of the fallbacks of their rudimentary implementation. For example, the card resource IDs are inserted into fixed, static memory. This poses a problem for cards with varrying resource counts, connector mode counts, etc. My laptop's DisplayPort connector, for example, had more than the pre-allocated 10 display modes. So, I replaced the fixed memory allocations with malloc/free calls.

#### Project Structure    
I moved mostly everything out of the single "main" file, moving most of the application structure/logic into different header/source files. I used CMake to structure and link my project.

#### Buffer Return    
The BetterOS tutorial project didn't exit gracefully. It told the CRT Controller to start displaying a framebuffer, but didn't make sure to save or restore whatever buffer the previous process (presumably the tty) was using. I implmented a restore_buffer function which tells the CRT Controller to restore it's previous configuration from before I took control of rendering.

### Troubles    
The troubles I faced in making the project.

#### Lack of Learning Material  
The BetterOS guide was the best comprehensive guide I could find on the topic. Improving meant I needed to research more into the process-- since the BetterOS guide served as an introduction only-- while some learning resources were scarce. I used the drm-memory(7) man-page extensively. I also needed to use the documentation/comments available in the drm.h header file. However, these two sources were incomplete. I also needed to find whatever I could from [kernel.org's](https://kernel.org) guides on the topic. However, altogether there were still some holes I had to fill in through trial and error, especially since there was an error in BetterOS's guide. 

#### Debugging Without Knowledge    
I had to learn about the debugging process used when coding in the C language, particularly within Linux. It wasn't so bad to start getting information out of errors, but the problem was that I was lost on the subject and didn't know what to do with the information. For example, BetterOS's tutorial contains an error which seems decievingly trivial.  

The DRM_IOCTL_MODE_GETCONNECTOR system call process is done in a pattern of two. You make the call, giving the CONNECTOR_ID as an argument, and the kernel gives you how many display modes, encoders, etc., are available. The problem with the BetterOS guide was that it was allocating 64 bit integer arrays for the resource IDs, which would be filled in with the DRM_IOCTL_MODE_GETRESOURCES call. I had to scour the source libdrm source code to find out what was wrong. Unfortunately, the resource IDs are 32 bit unsigned integers. So, the GETRESOURCES call would fill in the first cell-and-a-half of my array. In my GETCONNECTOR call, I'd skip over the second half of the first cell, because the ioctl call would only read the first 32 bits. The next call would read my third connector. And I'd try to read the third cell of my array, which would be empty. The outcome of this error was that my first connector, my laptop screen, worked fine. My second connector, my HDMI port didn't work at all. And my third connector, my DisplayPort connector, worked as well. Although, between the debugging, I had to work through some segmentation faults, which was really misleading.

## Current Project State    
For the most part, I've removed immediately apparent bugs. The project runs under two systems for which I can test the project. (two of my laptops). I got to the point where I can draw to the buffer using Cairo. Although, right now I've only set it to draw a magenta square in the corner of the screen, as I've just finished working out the kinks. 

## Potential Improvements    
* The current implemntation makes some hardware assumptions, mainly the systems graphics card situation. The program opens `/dev/dri/card0`, while many systems will have `/dev/dri/card1`, etc. One such situation is for systems with an APU and a GPU. So, include a method to detect which card to use and/or add command-line options and configuration files.
* More robust error-handling. One situation I've yet to account for is hot-plugging displays as I'm querying connector information. 
* More robust rendering pipline, including page flipping.

## Future direction    
I plan to use this project as a base to create my own login manager, which means I'll have to start learning input/interrupt handling.
