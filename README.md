## Introduction
Amazon web service gives businesses an opportunity to create sophisticated applications and provide a range of tools to use.  
Another application, APIs, are also useful to provide data in a way that is simple to understand and view. 
By combining these two components (using a weather API), we can create a program that provides users information about weather 
and allow their inputs to impact the outputs.

## Implementation
This project heavily relies on external information to function because it will produce real time weather that was collected from an API.
We will need to send this data to our CC3200 launchpad so our display will have accurate information.Another thing that will be implemented is the 
serial peripheral interface which is an interface that is used to communicate short distances between a slave and a master. 
Our CC3200 launchpad will be the master that is connected to the slave, Adafruit OLED so it can send data and produce a display on our OLED. 
We can trigger different responses by having the master send different bit messages to the slave.

We used something called I2C and it is a serial communication protocol that transfers data bit by bit on a single wire. 
It uses features present in SPI and UARTs and we are now capable of having multiple slaves to one master. I2C messages have a start (switches from high to low voltage) 
and stop (switches from low to high voltage) condition and frames in between contain other information such as a read/write bit and an acknowledge/no-acknowledge bit. 

The way I2C is implemented in my project is by using its sensors to poll data for its acceleration values and transferring that data to a graphical icon on the OLED.
The last part of our lab includes the AWS IoT. AWS IoT is a cloud platform that allows the users to connect devices to the cloud applications. 
It is a communication protocol that is designed to handle intermittent connections, minimize code footprint on devices, and reduce bandwidth requirements. 
It has become more reliable and useful over the years as technology advances. This application also allows users to analyze the data found and continue to 
send that data to another device.

## Results

