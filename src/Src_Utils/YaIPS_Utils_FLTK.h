/**********************************************************************************
 * YaIPS_Utils_FLTK.h

 Interface to YaIPS_Utils_FLTK.cpp

 20.02.2017 RR: * First edition of this file

*/

#ifndef YaIPS_Util_FLTK_H
#define YaIPS_Util_FLTK_H

#include <stdlib.h>

#include <FL/Fl.H>

/************************************************************************************
 * IqeFl_Float_Input
 *
 * A input class for 'Fl_Float_Input' with additional support for
 * the mouse wheel.
 *
 * Is like 'Fl_Float_Input' with the additional function
 * SetModifyData() with this arguments.
 *   Min, Max:   If Min < Max, clip value to this values.
 *               Default on creation: both are 0
 *   Step:       Must be > 0 to be used by the mouse wheel.
 *               Default on creation: 0
 *   Step2:      Must be > 0 to be used by the mouse wheel with pressed control key.
 *               Default on creation: 0
 *   WrapAround: Handle wrap around for circular number ranges (like angles).
 */

#include <FL/Fl_Float_Input.H>

class FL_EXPORT IqeFl_Float_Input : public Fl_Float_Input {

  // Values

public:

  float Step, Step2;       // Step in floats
  float Min, Max;          // If Min < Max, clip to this values
  int WrapAround;          // Wrap around handling for mouse wheel
  char Format[ 64];        // Format string

  /**
    Creates a new IqeFl_Float_Input widget using the given position,
    size, and label string. The default boxtype is FL_DOWN_BOX.

    Inherited destructor destroys the widget and any value associated with it.
  */
  IqeFl_Float_Input(int X,int Y,int W,int H,const char *L = 0) : Fl_Float_Input(X,Y,W,H,L) {

    Step = 0;                 // default NO step is set
    Step2 = 0;
    Min  = 0;                 // Default clipping is off
    Max  = 0;
    WrapAround = false;       // Wrap around is off

    memset( Format, 0, sizeof( Format)); // Zero all
    strcpy( Format, "%.1f");  // Set default format
  }

  // Set modify data

  void SetModifyData( float mi, float ma, float s = 1.0, float s2 = 0.0, int wa = false) {

    Min = mi;
    Max = ma;
    Step = s;
    Step2 = s2;
    WrapAround = wa;
  }

  // Set new format string

  void SetFormat( const char *pNewFormat) {

    strncpy( Format, pNewFormat, sizeof( Format) - 1);
  }

  // Set new value

  void SetValue( float Value) {

    char TempBuffer[ 256];

    if( Min < Max) {                          // Range is set

      if( Value < Min) Value = Min;           // Clip to range
      if( Value > Max) Value = Max;
    }

    sprintf( TempBuffer, Format, Value);      // Format number

    if( strcmp( TempBuffer, value()) != 0) {  // Is different

      value( TempBuffer);                     // Is different on GUI
    }
  }

  // Get new value

  float GetValue() {

    float Value;

    Value  = atof( value());                  // Get the value

    return( Value);
  }

protected:

  int handle(int event);   // overwrite event handler
};

/************************************************************************************
 * IqeFl_Int_Input
 *
 * A input class for 'Fl_Int_Input' with additional support for
 * the mouse wheel.
 *
 * Is like 'Fl_Int_Input' with the additional function
 * SetModifyData() with this arguments.
 *   Min, Max:   If Min < Max, clip value to this values.
 *               Default on creation: both are 0
 *   Step:       Must be > 0 to be used by the mouse wheel.
 *               Default on creation: 0
 *   WrapAround: Handle wrap around for circular number ranges (like angles).
 *   Step2:      Must be > 0 to be used by the mouse wheel with pressed control key.
 *               Default on creation: 0
 */

#include <FL/Fl_Int_Input.H>

class FL_EXPORT IqeFl_Int_Input : public Fl_Int_Input {

  // Values

public:

  int Step, Step2;         // Step in integers
  int Min, Max;            // If Min < Max, clip to this values
  int WrapAround;          // Wrap around handling for mouse wheel

  /**
    Creates a new IqeFl_Float_Input widget using the given position,
    size, and label string. The default boxtype is FL_DOWN_BOX.

    Inherited destructor destroys the widget and any value associated with it.
  */
  IqeFl_Int_Input(int X,int Y,int W,int H,const char *L = 0) : Fl_Int_Input(X,Y,W,H,L) {

    Step = 0;     // default NO step is set
    Step2 = 0;
    Min  = 0;     // Default clipping is off
    Max  = 0;
    WrapAround = false;  // Wrap around is off
  }

  // Initial set modify data
  void SetModifyData( int mi, int ma, int s = 1, int s2 = 0, int wa = false) {
    Min = mi;
    Max = ma;
    Step = s;
    Step2 = s2;
    WrapAround = wa;
  }

  // Change min max values
  void ChangeMinMax( int mi, int ma) {

    if( Min == mi && Max == ma) {  // Values will noch change

      return;
    }

    Min = mi;       // Update values
    Max = ma;

    int ValueIn, ValueOut;
    char TempString[ 256];

    ValueIn = atoi( value());       // get the current value

    ValueOut = ValueIn;

    if( Min < Max) {                  // Range is set

      if( ValueOut < Min) ValueOut = Min;   // Clip to new range
      if( ValueOut > Max) ValueOut = Max;
    }

    if( ValueOut != ValueIn) {         // Value has changed

      sprintf( TempString, "%d", ValueOut);
      value( TempString);             // Set the new value

      //x/do_callback();
    }
  }

  // Set new value

  void SetValue( int Value) {

    char TempBuffer[ 256];

    if( Min < Max) {                          // Range is set

      if( Value < Min) Value = Min;           // Clip to range
      if( Value > Max) Value = Max;
    }

    sprintf( TempBuffer, "%d", Value);        // Format number

    if( strcmp( TempBuffer, value()) != 0) {  // Is different

      value( TempBuffer);                     // Is different on GUI
    }
  }

  // Get new value

  int GetValue() {

    int Value;

    Value  = atoi( value());                  // Get the value

    return( Value);
  }

protected:

  int handle(int event);   // overwrite event handler
};

/************************************************************************************
 * IqeFl_Tabs
 *
 * Extends the Fl_Tabs widget to allow the current tab group to be
 * returned as a number or to select a tab group with a number.
 *
 */

#include <FL/Fl_Tabs.H>

class FL_EXPORT IqeFl_Tabs : public Fl_Tabs {

  // Values

public:

  /**
    Creates a new IqeFl_Tabs widget using the given position,
    size, and label string.

    Inherited destructor destroys the widget and any value associated with it.
  */
  IqeFl_Tabs( int X,int Y,int W,int H,const char *L = 0) : Fl_Tabs(X,Y,W,H,L) {

  }

  // Gets the currently visible tab.
  // Return: >= 0 Number of current visible tab.
  //              First one has the number 0.
  //           -1 The tab group is empty.
  int GetTabGroup();

  // Sets the tab to become the current visible tab.
  // Return:  1 if a different tab was chosen
  //          0 if there was no change (new value already set)
  //         -1 error
  int SetTabGroup( int TabNr);

};

/************************************************************************************
 * IqeFl_Menu_Button
 *
 * This is a rewrite of the Fl_Menu_Button class.
 * Only the down arrow is NOT drawn.
 *
 */

#include <FL/Fl_Menu_.H>

class FL_EXPORT IqeFl_Menu_Button : public Fl_Menu_ {

protected:
  void draw() FL_OVERRIDE;
  static IqeFl_Menu_Button* pressed_menu_button_;

public:
  /**
   \brief indicate what mouse buttons pop up the menu.

   Values for type() used to indicate what mouse buttons pop up the menu.
   IqeFl_Menu_Button::POPUP3 is usually what you want.
   */
  enum popup_buttons {POPUP1 = 1, /**< pops up with the mouse 1st button. */
    POPUP2,  /**< pops up with the mouse 2nd button. */
    POPUP12, /**< pops up with the mouse 1st or 2nd buttons. */
    POPUP3,   /**< pops up with the mouse 3rd button. */
    POPUP13,  /**< pops up with the mouse 1st or 3rd buttons. */
    POPUP23,  /**< pops up with the mouse 2nd or 3rd buttons. */
    POPUP123 /**< pops up with any mouse button. */
  };
  int handle(int) FL_OVERRIDE;
  const Fl_Menu_Item* popup();
  IqeFl_Menu_Button( int X,int Y,int W,int H,const char *L = 0);
};

/************************************************************************************
 * IqeFl_Check_Bit
 *
 * This is a rewrite of the Fl_Check_Button class.
 *
 * On creation the pointer to an integer variable and a bit mask is set.
 * The check-button state reflects the bit state.
 *
 * * int GetValue()
 *   Call this function inside a callback function to update the variable.
 *   Return: true  if bit value has changed by a changed check-button state.
 * * void UpdateValue()
 *   Update check-button
 *   Update value of check-button.
 *   Use this after change of variable.
 * * IsSet()
 *   Test bit is set in variable.
 */

#include <FL/Fl_Check_Button.H>

class FL_EXPORT IqeFl_Check_Bit : public Fl_Check_Button {

  // Values

public:

  int *pValue;           // Point to integer variable
  int BitMask;           // A single bit must be set here.

  /**
    Creates a new IqeFl_Check_Button widget using the given position,
    size, and label string.
  */
  IqeFl_Check_Bit( int X,int Y,int W,int H,                          // Box
                   int *pValueArg, int BitMaskArg,                   // Point to value, bitmask
                   const char *L = 0) : Fl_Check_Button(X,Y,W,H,L) {

    pValue  = pValueArg;
    BitMask = BitMaskArg;

    // Initial set state of check-button.
    value( (*pValue & BitMask) != 0);
  }

  // Get check-button setting to variable.
  // Use this in callback function of variable.
  int GetValue() {

    int HasChanged = false;

    if( value()) {                    // Check-button is set

      if( (*pValue & BitMask) == 0) { // Was not set before

        *pValue |= BitMask;           // Set bit

        HasChanged = true;            // Value has changed
      }

    } else {                          // Check-button is not set

      if( (*pValue & BitMask) != 0) { // Was set before

        *pValue &= ~BitMask;          // Reset bit

        HasChanged = true;            // Value has changed
      }
    }

    return( HasChanged);
  }

  // Update value of check-button.
  // Use this after change of variable.
  void UpdateValue() {

    value( (*pValue & BitMask) != 0);
  }

  // Test bit is set in variable.
  int IsSet() {

    return( (*pValue & BitMask) != 0);
  }
};

/************************************************************************************
 * IqeFl_Radio_Button_Bits
 *
 * This is a rewrite of the Fl_Radio_Round_Button class.
 *
 * On creation the pointer to an integer variable with some bits. The bits
 * together are used the values of multiple radi-buttons. Each of the
 * radio-bottons is associated with one of the values in the bits.
 *
 * * int GetValue()
 *   Call this function inside a callback function to update the variable.
 *   Return: true  if radio-button value has changed.
 * * void UpdateValue()
 *   Update radio-button
 *   Update value of the radio-button.
 *   Use this after change of variable.
 * * IsSet()
 *   Test if this radio button is set.
 *  int GetRadioValue() {
 *   Get value of the radio-button variable bits.
 */

#include <FL/Fl_Radio_Round_Button.H>

class FL_EXPORT IqeFl_Radio_Button_Bits : public Fl_Radio_Round_Button {

  // Values

public:

  int *pValue;           // Point to integer variable
  int BitMask;           // Some bits to mask the bits of the radio button value.
  int BitsThis;          // Bits for this radio button.

  /**
    Creates a new IqeFl_Check_Button widget using the given position,
    size, and label string.
  */
  IqeFl_Radio_Button_Bits( int X,int Y,int W,int H,                          // Box
                           int *pValueArg, int BitMaskArg, int BitsThisArg,  // Point to value, bitmask
                           const char *L = 0) : Fl_Radio_Round_Button(X,Y,W,H,L) {

    pValue   = pValueArg;
    BitMask  = BitMaskArg;
    BitsThis = BitsThisArg & BitMask;

    // Initial set state of check-button.
    value( (*pValue & BitMask) == BitsThis);
  }

protected:

  // Get check-button setting to variable.
  // Use this in callback function of variable.
  int GetValue() {

    int HasChanged = false;

    if( value()) {                           // Radio-button is set

      if( (*pValue & BitMask) != BitsThis) { // Was not set before

        *pValue &= BitMask;                  // Clear bits
        *pValue |= BitsThis;                 // Set bits for this radio-button

        HasChanged = true;                   // Value has changed
      }

    } else {                                 // Radio-button is not set

      // This should not happen
    }

    return( HasChanged);
  }

  // Update value of check-button.
  // Use this after change of variable.
  void UpdateValue() {

    value( (*pValue & BitMask) == BitsThis);
  }

  // Test bit is set in variable.
  int IsSet() {

    return( (*pValue & BitMask) == BitsThis);
  }

  // Get value of the radio-button variable bits.
  int GetRadioValue() {

    return( *pValue & BitMask);
  }
};

#endif // YaIPS_Util_FLTK_H

/****************************** End Of File ******************************/
