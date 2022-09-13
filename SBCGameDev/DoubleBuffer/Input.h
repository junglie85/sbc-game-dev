
#pragma once
#include <iostream>
#include <linux/input.h>
#include <pthread.h>
#include <stdio.h>

//*******************************************************************************************
// Mega thanks to Grumpy old Git Developer Gareth Lewis for his help in putting this together
//*******************************************************************************************

// Keys[] sets the relevent index to TRUE when pressed and false when released, index names can be found in
// <linux/input.h> and follow the usual KEY_A...KEY_Z format mouse keeps track of the motion of the mouse not the
// absolute position, which needs to be known beforehand if you plan to use the mouse for direct control. wherever the
// GUI mouse is at startup is considered 0,0 and all movement is offset from there. If no GUI it assumes 0,0 is start.
// User must draw their own mouse!

class Input {
#define TRUE 1
#define FALSE 0

public:
    typedef struct // A very simple structure to hold the mouse info
    {
        int PositionX; // contains the relative position from the start point (take care to not confuse it with the GUI
                       // mouse position)
        int PositionY;
        unsigned char LeftButton; // TRUE when pressed FALSE otherwise
        unsigned char MidButton;
        unsigned char RightButton;

    } MouseData;

    char Keys[512]; // maximum possible keys is a little less than this, but best to be careful in case of future
                    // expansion
    MouseData TheMouse;

    pthread_t threadKeyboard;
    pthread_t threadMouse; // handles for the threads

    bool mQuit; // set true to kill the mouse thread
    bool kQuit; // set true to kill the key thread

    int iterations;
    bool KeyPressed;

    //****************************************************************************************************
    // Intended to be a thread which processes the mouse events and stores  Mouse info in TheMouse struct.
    //****************************************************************************************************

    static void* ProcessMouseThread(void* arg);

    //*********************************************************************************
    // This thread processes the keys, and stores TRUE/FALSE values in the Keys[] array.
    //*********************************************************************************
    static void* ProcessKeyboardThread(void* arg);

    // small accessor function to test a specific key
    bool TestKey(unsigned char WhatKey);
    bool SimpleTest();

    /************************
    must call init before use
    ************************/
    bool Init();
    //*****************************************************************************
    // this will test for our keyboards
    //*****************************************************************************
    int AreYouMyKeyboard();
    std::string kbd; // this will be the event filename for the keyboard discovered in the init/AreYouMyKeyboard test
};
