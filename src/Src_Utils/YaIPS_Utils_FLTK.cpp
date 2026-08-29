/****************************************************************************

  YaIPS_Util_FLTK.cpp

  FLKT utilities.

 20.02.2017 RR: * First edition of this file

*****************************************************************************
*/

#include "YaIPS_Utils_FLTK.h"

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

int IqeFl_Float_Input::handle( int event)
{

  switch( event) {
    case FL_MOUSEWHEEL: // The user has moved the mouse wheel.

      if( ! Fl::event_inside( this)) {  // If mouse is not inside this widget

        return( 0);                     // Is not handled from this
      }

      if( Step > 0) {                   // Step is set

        float ValueIn, ValueOut, StepUse;
        char TempString[ 256];
        int State;

        ValueIn = atof( value());      // get the current value

        if( Step2 == 0 &&              // Have no second step value
            ((int)(Step * 100.0)) % 100 == 0) {  // Have no after point digits

          if( ValueIn >= 0.0) ValueIn = (int)( ValueIn + 0.5);  // Round to nearest integer
          else                ValueIn = (int)( ValueIn - 0.5);
        }

        ValueOut = ValueIn;

        State = Fl::event_state();
        if( (State & FL_CTRL) &&       // Control key pressed
            Step2 != 0.0) {            // and second step value set

          StepUse = Step2;

        } else {

          StepUse = Step;
        }

        if( Fl::event_dy() > 0) {         // positive value

          ValueOut = ValueIn - StepUse;

          if( WrapAround && Min < Max && ValueOut < Min) {

            ValueOut = ValueOut - Min + Max;
          }
        } else if( Fl::event_dy() < 0) {  // Negative

          ValueOut = ValueIn + StepUse;

          if( WrapAround && Min < Max && ValueOut > Max) {

            ValueOut = ValueOut - Max + Min;
          }
        }

        if( Min < Max) {                  // Range is set

          if( ValueOut < Min) ValueOut = Min;   // Clip to range
          if( ValueOut > Max) ValueOut = Max;
        }

        if( ValueOut != ValueIn) {         // Value has changed

          sprintf( TempString, Format, ValueOut);
          value( TempString);             // Set the new value

          do_callback();
        }
      }

      return( 1);
    break;
  }

  return( Fl_Float_Input::handle( event));     // do parent handle function
}

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
 *   Step2:      Must be > 0 to be used by the mouse wheel with pressed control key.
 *               Default on creation: 0
 *   WrapAround: Handle wrap around for circular number ranges (like angles).
 */

int FL_EXPORT IqeFl_Int_Input::handle( int event)
{

  switch( event) {
    case FL_MOUSEWHEEL: // The user has moved the mouse wheel.

      if( ! Fl::event_inside( this)) {  // If mouse is not inside this widget

        return( 0);                     // Is not handled from this
      }

      if( Step > 0) {                   // Step is set

        int ValueIn, ValueOut, StepUse, State;
        char TempString[ 256];

        ValueIn = atoi( value());       // get the current value

        ValueOut = ValueIn;

        State = Fl::event_state();
        if( (State & FL_CTRL) &&       // Control key pressed
            Step2 != 0) {              // and second step value set

          StepUse = Step2;

        } else {

          StepUse = Step;
        }

        if( Fl::event_dy() > 0) {         // positive value

          ValueOut = ValueIn - StepUse;

          if( WrapAround && Min < Max && ValueOut < Min) {

            ValueOut = ValueOut - Min + Max;
          }
        } else if( Fl::event_dy() < 0) {  // Negative

          ValueOut = ValueIn + StepUse;

          if( WrapAround && Min < Max && ValueOut > Max) {

            ValueOut = ValueOut - Max + Min;
          }
        }

        if( Min < Max) {                  // Range is set

          if( ValueOut < Min) ValueOut = Min;   // Clip to range
          if( ValueOut > Max) ValueOut = Max;
        }

        if( ValueOut != ValueIn) {         // Value has changed

          sprintf( TempString, "%d", ValueOut);
          value( TempString);             // Set the new value

          do_callback();
        }
      }

      return( 1);
    break;
  }

  return( Fl_Int_Input::handle( event));     // do parent handle function
}

/************************************************************************************
 * IqeFl_Tabs
 *
 * Extends the Fl_Tabs widget to allow the current tab group to be
 * returned as a number or to select a tab group with a number.
 *
 */

/************************************************************************************
 * int GetTabGroup()
 *
 * Gets the currently visible tab.
 * Return: >= 0 Number of current visible tab.
 *              First one has the number 0.
 *           -1 The tab group is empty.
*/

int FL_EXPORT IqeFl_Tabs::GetTabGroup()
{
  int i, n, iSel;
  Fl_Widget* o;

  iSel = -1;                                  // Return default value

  n = children();                             // Get number of tab groups

  Fl_Widget*const* a = array();

  for( i = n; i--; ) {

    o = *a++;

    if( iSel >= 0) {

      o->hide();

    } else if( o->visible()) {

      iSel = n - i - 1;

    } else if( !i) {

      o->show();
      iSel = n - i - 1;
    }
  }

  return( iSel);
}

/************************************************************************************
 * int SetTabGroup( int TabNr)
 *
 * Sets the tab to become the current visible tab.
 * Return:  1 if a different tab was chosen
 *          0 if there was no change (new value already set)
 *         -1 error
*/
int FL_EXPORT IqeFl_Tabs::SetTabGroup( int TabNr)
{
  int n;
  Fl_Widget* o;

  n = children();                             // Get number of tab groups

  if( n <= 0 || TabNr >= n) {                 // No tab groups or out of range

    return( -1);
  }

  Fl_Widget*const* a = array();

  o = a[ TabNr];

  n = value( o);

  return( n);
}

/************************************************************************************
 * IqeFl_Menu_Button
 *
 * This is a rewrite of the IqeFl_Menu_Button class.
 * Only the down arrow is NOT drawn.
 *
 */

IqeFl_Menu_Button* IqeFl_Menu_Button::pressed_menu_button_ = NULL;

void IqeFl_Menu_Button::draw() {
  if (!box() || type()) return;

  // calculate position and size of virtual "arrow box" (choice button)

#ifdef use_again   // Don't draw the arrow
  int ah = h() - Fl::box_dh(box());
  int aw = ah > 20 ? 20 : ah; // limit width: don't waste space for button
  int ay = y() + (h() - ah) / 2;
  int ax = x() + w() - Fl::box_dx(box()) - aw;
#else
  int aw = 0;
#endif

  // the remaining space is used to draw the label

  draw_box(pressed_menu_button_ == this ? fl_down(box()) : box(), color());
  draw_label(x() + Fl::box_dx(box()), y(), w() - Fl::box_dw(box()) - aw, h());
  if (Fl::focus() == this) draw_focus();

#ifdef use_again   // Don't draw the arrow
  // draw the arrow (choice button)

  Fl_Color arrow_color = active_r() ? labelcolor() : fl_inactive(labelcolor());
  fl_draw_arrow(Fl_Rect(ax, ay, aw, ah), FL_ARROW_SINGLE, FL_ORIENT_DOWN, arrow_color);
#endif
}


/**
  Act exactly as though the user clicked the button or typed the
  shortcut key.  The menu appears, it waits for the user to pick an item,
  and if they pick one it sets value() and does the callback or
  sets changed() as described above.  The menu item is returned
  or NULL if the user dismisses the menu.

  \note Since FLTK 1.4.0 Fl_Menu_::menu_end() is called before the menu
    pops up to make sure the menu array is located in private storage.

  \see Fl_Menu_::menu_end()
*/
const Fl_Menu_Item* IqeFl_Menu_Button::popup() {
  menu_end();
  const Fl_Menu_Item* m;
  pressed_menu_button_ = this;
  redraw();
  Fl_Widget_Tracker mb(this);
  if (!box() || type()) {
    m = menu()->popup(Fl::event_x(), Fl::event_y(), label(), mvalue(), this);
  } else {
    m = menu()->pulldown(x(), y(), w(), h(), 0, this);
  }
  picked(m);
  pressed_menu_button_ = 0;
  if (mb.exists()) redraw();
  return m;
}

int IqeFl_Menu_Button::handle(int e) {
  if (!menu() || !menu()->text) return 0;
  switch (e) {
  case FL_ENTER: /* FALLTHROUGH */
  case FL_LEAVE:
    return (box() && !type()) ? 1 : 0;
  case FL_PUSH:
    if (!box()) {
      if (Fl::event_button() != 3) return 0;
    } else if (type()) {
      if (!(type() & (1 << (Fl::event_button()-1)))) return 0;
    }
    if (Fl::visible_focus()) Fl::focus(this);
    popup();
    return 1;
  case FL_KEYBOARD:
    if (!box()) return 0;
    if (Fl::event_key() == ' ' &&
        !(Fl::event_state() & (FL_SHIFT | FL_CTRL | FL_ALT | FL_META))) {
      popup();
      return 1;
    } else return 0;
  case FL_SHORTCUT:
    if (Fl_Widget::test_shortcut()) {popup(); return 1;}
    return test_shortcut() != 0;
  case FL_FOCUS: /* FALLTHROUGH */
  case FL_UNFOCUS:
    if (box() && Fl::visible_focus()) {
      redraw();
      return 1;
    }
    break;
  default:
    break;
  }
  return 0;
}

/**
  Creates a new IqeFl_Menu_Button widget using the given position,
  size, and label string. The default boxtype is FL_UP_BOX.
  <P>The constructor sets menu() to NULL.  See
  Fl_Menu_ for the methods to set or change the menu.
*/
IqeFl_Menu_Button::IqeFl_Menu_Button(int X,int Y,int W,int H,const char *l)
: Fl_Menu_(X,Y,W,H,l) {
  down_box(FL_NO_BOX);
}

/********************************** End Of File **********************************/
