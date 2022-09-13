
#include "Input.h"
#include <dirent.h>

#define MouseScreenXlimit 1920
#define MouseScreenYlimit 1080

void* Input::ProcessMouseThread(void* arg)
{
    FILE* fmouse;
    fmouse = fopen("/dev/input/mice", "r");

    if (fmouse != NULL) {
        while (((Input*)arg)->mQuit == false) // so as long as mQuit is FALSE, this will endlessly loop
        {
            signed char b[3];
            fread(b, sizeof(char), 3, fmouse);

            // if we do plan to scale, best make these into floats for greater precision before they are cast down to
            // ints.
            float mousex = (float)b[1];
            float mousey = -(float)b[2];

            ((Input*)arg)->TheMouse.PositionX
                += (mousex / 1.0f); // 1.0 can be replaced by a scale factor (entierly optional)

            if (((Input*)arg)->TheMouse.PositionX > MouseScreenXlimit)
                ((Input*)arg)->TheMouse.PositionX = MouseScreenXlimit;
            if (((Input*)arg)->TheMouse.PositionX < 0) ((Input*)arg)->TheMouse.PositionX = 0;

            ((Input*)arg)->TheMouse.PositionY += (mousey / 1.0f);

            if (((Input*)arg)->TheMouse.PositionY > MouseScreenYlimit)
                ((Input*)arg)->TheMouse.PositionY = MouseScreenYlimit;
            if (((Input*)arg)->TheMouse.PositionY < 0) ((Input*)arg)->TheMouse.PositionY = 0;

            ((Input*)arg)->TheMouse.LeftButton
                = (b[0] & 1) > 0; // using a test( x >0 )  allows it to return and store a bool
            ((Input*)arg)->TheMouse.MidButton   = (b[0] & 4) > 0;
            ((Input*)arg)->TheMouse.RightButton = (b[0] & 2) > 0;
        }
        fclose(fmouse);
    }
    printf("Mouse Thread closing \n");

    pthread_exit(NULL);
}

void* Input::ProcessKeyboardThread(void* arg)
{
    FILE* fp;

    fp = fopen(((Input*)arg)->kbd.c_str(), "r");
    struct input_event ev;

    if (fp != NULL) {
        while (((Input*)arg)->kQuit == false) // kQuit is set to false by the init
        {
            fread(&ev, sizeof(struct input_event), 1, fp);

            if (ev.type == (__u16)EV_KEY) {
                ((Input*)arg)->Keys[ev.code] = ev.value > 0 ? TRUE : FALSE; // never gets here to give me key values
                ((Input*)arg)->KeyPressed    = true;
            } else
                ((Input*)arg)->KeyPressed = false;
        }
        fclose(fp);
    }

    printf("Key Thread closing\n");
    pthread_exit(NULL);
}

bool Input::TestKey(unsigned char WhatKey)
{
    return Keys[WhatKey] == TRUE; // simply return binary true or false
}

bool Input::SimpleTest() { return KeyPressed; }

bool Input::Init()
{
    kQuit      = false;
    mQuit      = false;
    iterations = 0;

    int result;

    // mice don't usually provide any issues
    result = pthread_create(
        &threadMouse, NULL, &ProcessMouseThread, this); // we send the Input class (this) as an argument to allow for
                                                        // easy cast ((Input*)arg)-> type access to the classes data.
    if (result != 0)
        printf("got an error\n");
    else
        printf("mouse thread started\n");

    if (AreYouMyKeyboard() == false)
        printf("Oh Bugger, we can't seen to find the keyboard\n"); // go find an active keyboard

    result = pthread_create(&threadKeyboard, NULL, &ProcessKeyboardThread, this);
    if (result != 0)
        printf("got an error\n");
    else
        printf("Key thread started\n");

    return true;
}

// tests for the keyboard, which can be on different events in Linux
// thanks to my student Petar Dimitrov for this improvement to the keyboard search systems
int Input::AreYouMyKeyboard()
{
    // Some bluetooth keyboards are registered as "event-mouse".
    // If this is your case, then just change this variable to event-mouse.
    // this is a frustrating issue that needs more research.
    std::string pattern = "event-kbd"; //<-change event-kbd to event-mouse if your BT keyboard is a "mouse"
    std::string file    = "";

    DIR* dir;
    struct dirent* ent;

    printf("Checking for active keyboard\nPlease note, you may need to alter line marked in AreYouMyKeyboard() in "
           "Input.cpp if no keyboard response occurs\n");

    if ((dir = opendir("/dev/input/by-path/")) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) // lets scan theough the event files that are available
        {
            fprintf(stdout, "%s\n", ent->d_name); // report what file we are checking
            file = std::string(ent->d_name);

            if (!file.compare(".") || !file.compare("..")) continue;

            if (file.substr(file.length() - pattern.length()) == pattern) // check if we have an event-kbd
            {
                kbd = "/dev/input/by-path/"
                    + file; // yes we seem to ...but be aware of the issue with wireless keyboards, not responding
                fprintf(stdout, "%s\n", kbd.c_str()); // report the chosen event handler

                return true;
            }
        } // while ends
    }

    return false;
}
