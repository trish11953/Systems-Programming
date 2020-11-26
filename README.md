# ScatterGather Filesystem
You are to write a basic device driver that will sit between a virtual application and virtualized hardware devices. The application makes use of the abstraction you will provide called the ScatterGather driver. The functionality for this will be developed over the rest of the semester. This first assignment will deal with one aspect; the creation and processing of messages called packets.

The ScatterGather service is a software simulation of an online storage system such as OneDrive or Box. It has a basic protocol by which you communicate with it through a series of messages sent across a network. You will communicate with this service and submit and retrieve data in form of blocks. You will provide all the code to implement the protocol with remote nodes to implement a simple flat filesystem (no directories).

The sg_sim application is provided to you and will call your device driver functions with the basic UNIX file operations (open, read, ...). You are to write the device driver code to implement the file OS operations. Your code will communicate with virtual devices by creating and receiving packets (structured data).

The implementation for the application (called the simulator) and the virtual Internet service is given to you. Numerous utility functions are also provided to help debug and test your program, as well as create readable output. For each assignment, a sample output will be provided. Students that make use of all of these tools (which take a bit of time up front to figure out) will find that the later assignments will become much easier to complete.

