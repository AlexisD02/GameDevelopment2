//--------------------------------------------------------------------------------------
// Key/mouse input functions
//--------------------------------------------------------------------------------------
// Not encapsulated into a class to keep familiar TL-Engine-like usage

#include "Input.h"
#include <array>


/*-----------------------------------------------------------------------------------------
    Globals
-----------------------------------------------------------------------------------------*/

// Current state of all keys (and mouse buttons)
std::array<KeyState, NumKeyCodes> gKeyStates;

// Current position of mouse
Vector2i gMousePosition = {0, 0};



/*-----------------------------------------------------------------------------------------
    Initialisation
-----------------------------------------------------------------------------------------*/

void InitInput()
{
    // Initialise input data
    for (auto& keyState : gKeyStates)  keyState = NotPressed;
    gMousePosition = { 0, 0 };
}


/*-----------------------------------------------------------------------------------------
    Events
-----------------------------------------------------------------------------------------*/

// Event called to indicate that a key or mouse button has been pressed down
void KeyDownEvent(KeyCode Key)
{
    if (gKeyStates[Key] == NotPressed)
    {
        gKeyStates[Key] = Pressed;
    }
    else
    {
        gKeyStates[Key] = Held;
    }
}

// Event called to indicate that a key or mouse button has been lifted up
void KeyUpEvent(KeyCode Key)
{
   gKeyStates[Key] = NotPressed;
}

// Event called to indicate that the mouse has been moved
void MouseMoveEvent(int X, int Y)
{
    gMousePosition += { X, Y };
}


/*-----------------------------------------------------------------------------------------
    Input functions
-----------------------------------------------------------------------------------------*/

// When the given key or button is pressed down the first call to this function during the keypress will return true,
// otherwise returns false. Poll frequently and use for one-off actions or toggles that trigger once per key-press.
// Example key codes: Key_A or Mouse_LButton, see Input.h for a full list.
bool KeyHit(KeyCode eKeyCode)
{
    if (gKeyStates[eKeyCode] == Pressed)
    {
        gKeyStates[eKeyCode] = Held;
        return true;
    }
    return false;
}


// Returns true if as a given key or button is pressed down, otherwise false
// Example key codes: Key_A or Mouse_LButton, see Input.h for a full list.
bool KeyHeld(KeyCode eKeyCode)
{
    if (gKeyStates[eKeyCode] == NotPressed)
    {
        return false;
    }
    gKeyStates[eKeyCode] = Held;
    return true;
}

    
// When any key is pressed down the first call to this function during the keypress will return true, otherwise false.
bool AnyKeyHit()
{
    bool hit = false;
    for (auto& keyState : gKeyStates)
    {
        if (keyState == Pressed)
        {
            keyState = Held;
            hit = true;
        }
    }
    return hit;
}


// Returns true if any key or button is pressed down, otherwise false
bool AnyKeyHeld()
{
    bool hit = false;
    for (auto& keyState : gKeyStates)
    {
        if (keyState != NotPressed)
        {
            keyState = Held;
            hit = true;
        }
    }
    return hit;
}


// Returns current position of mouse as a Vector2i
Vector2i GetMouse()
{
    return gMousePosition;
}
