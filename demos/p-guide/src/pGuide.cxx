/*
     P-GUIDE - PUI-based Graphical User Interface Designer
     Copyright (C) 2002  John F. Fay

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License as
     published by the Free Software Foundation; either version 2 of
     the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

// Program to allow the user to build a PUI Graphical User Interface

#include <plib/pu.h>

#include "WidgetList.h"

WidgetList *widgets = (WidgetList *)NULL ;
int max_layer = 1 ;

puObject *active_object = (puObject *)NULL ;
WidgetList *active_widget = (WidgetList *)NULL ;
short activity_flag = 0 ;  // 0 - inactive; 1 - moving; 2 - resizing xmin, 3 - resizing ymin, 4 - resizing xmax, 5 - resizing ymax
int resize_symmetric = 1 ;

int mouse_x_position_in_object = 0 ;
int mouse_y_position_in_object = 0 ;

// Main window parameters
int main_window = 0 ;  // Main window handle
int main_window_width  = 600 ;
int main_window_height = 600 ;
char main_window_name [ PUSTRING_MAX ] ;
int main_window_x = 0, main_window_y = 0 ;

float main_window_color_r = 1.0, main_window_color_g = 1.0,
      main_window_color_b = 1.0, main_window_color_a = 1.0 ;

bool main_window_changed = false ;

static int ctrl_key_down = 0 ;

static int mouse_x = 0 ;
static int mouse_y = 0 ;

// Widget count
static int widget_number = 0 ;

// From the status window:

extern void setStatusWidgets ( WidgetList *wid ) ;

// GLUT Main Window Callbacks

static void process_key ( int key )
{
  extern int selected_object_type ;
  extern puButton *active_button ;

  ctrl_key_down = ( glutGetModifiers () & GLUT_ACTIVE_CTRL ) ;
#define NUDGE_DISTANCE    1
#define LARGE_NUDGE_DISTANCE    10

  if ( active_object )  // Active object; check the keys
  {
    main_window_changed = true ;

    int xo, yo ;
    active_object->getPosition ( &xo, &yo ) ;
    int dist = ctrl_key_down ? LARGE_NUDGE_DISTANCE : NUDGE_DISTANCE ;

    if ( key == GLUT_KEY_LEFT )
      active_object->setPosition ( xo-dist, yo ) ;
    else if ( key == GLUT_KEY_RIGHT )
      active_object->setPosition ( xo+dist, yo ) ;
    else if ( key == GLUT_KEY_UP )
      active_object->setPosition ( xo, yo+dist ) ;
    else if ( key == GLUT_KEY_DOWN )
      active_object->setPosition ( xo, yo-dist ) ;
    else if ( ( key == 127 ) || ( key == 8 ) )  // Delete or Backspace
    {
      WidgetList *wid = widgets ;
      WidgetList *prv = (WidgetList *)NULL ;
      while ( wid )
      {
        if ( wid->obj == active_object )
        {
          if ( prv )  // Remove the widget from the linked list
            prv->next = wid->next ;
          else
            widgets = wid->next ;

          delete wid->object_type_name ;  // Delete the widget
          delete wid->legend_text ;
          delete wid->label_text ;
          delete wid->obj ;
          delete wid ;
          active_widget = (WidgetList *)NULL ;
          active_object = (puObject *)NULL ;
          wid = (WidgetList *)NULL ;
        }
        else
        {
          prv = wid ;
          wid = wid->next ;
        }
      }
    }
    else if ( key == 27 )  // Escape key, deactivate the object
    {
      active_widget = (WidgetList *)NULL ;
      active_object = (puObject *)NULL ;
    }

    setStatusWidgets ( active_widget ) ;
  }

  if ( selected_object_type )
  {
    if ( key == 27 )  // Escape key, deactivate the selected button
    {
      selected_object_type = 0 ;
      active_button->setValue ( 0 ) ;
    }
  }

  glutPostRedisplay () ;
}

static void main_window_specialfn ( int key, int, int )
{
  process_key ( key ) ;
}

static void main_window_keyfn ( unsigned char key, int, int )
{
  process_key ( key ) ;
}

static void main_window_motionfn ( int x, int yy )
{
  int y = main_window_height - yy ;

  mouse_x = x ;
  mouse_y = y ;

  // Dragging the mouse:  If there is an active object, slide it around
  if ( active_object )
  {
    extern puInput *object_position_x ;
    extern puInput *object_position_y ;
    extern puInput *object_size_x ;
    extern puInput *object_size_y ;

    main_window_changed = true ;

    int dist ;
    int xo, yo ;
    active_object->getPosition ( &xo, &yo ) ;
    int w, h ;
    active_object->getSize ( &w, &h ) ;

    switch ( activity_flag )
    {
    case 1 :  // Moving the object
      active_object->setPosition ( x - mouse_x_position_in_object,
                                   y - mouse_y_position_in_object ) ;
      break ;

    case 2 :  // Resizing on x-min
      dist = x - xo ;
      if ( resize_symmetric * dist > w ) dist = w / resize_symmetric - 1 ;
      active_object->setPosition ( x, yo ) ;
      active_object->setSize ( w - resize_symmetric * dist, h ) ;
      break ;

    case 3 :  // Resizing on y-min
      dist = y - yo ;
      if ( resize_symmetric * dist > h ) dist = h / resize_symmetric - 1 ;
      active_object->setPosition ( xo, y ) ;
      active_object->setSize ( w, h - resize_symmetric * dist ) ;
      break ;

    case 4 :  // Resizing on x-max
      dist = x - xo - w ;
      if ( resize_symmetric * dist < -w ) dist = 1 - w / resize_symmetric ;
      active_object->setSize ( w + resize_symmetric * dist, h ) ;
      if ( resize_symmetric == 2 ) active_object->setPosition ( xo - dist, yo ) ;
      break ;

    case 5 :  // Resizing on y-max
      dist = y - yo - h ;
      if ( resize_symmetric * dist < -h ) dist = 1 - h / resize_symmetric ;
      active_object->setSize ( w, h + resize_symmetric * dist ) ;
      if ( resize_symmetric == 2 ) active_object->setPosition ( xo, yo - dist ) ;
      break ;
    }

    int a, b ;
    active_object->getPosition ( &a, &b ) ;
    object_position_x->setValue ( a ) ;
    object_position_y->setValue ( b ) ;

    active_object->getSize ( &a, &b ) ;
    object_size_x->setValue ( a ) ;
    object_size_y->setValue ( b ) ;
  }

  glutPostRedisplay () ;
}

static void main_window_passivefn ( int x, int yy )
{
  int y = main_window_height - yy ;

  // Dragging the mouse without a button down:  save the mouse coordinates
  mouse_x = x ;
  mouse_y = y ;
}

static void main_window_mousefn ( int button, int updown, int x, int yy )
{
  int y = main_window_height - yy ;

  mouse_x = x ;
  mouse_y = y ;

  ctrl_key_down = ( glutGetModifiers () & GLUT_ACTIVE_CTRL ) ;

  // Downclick:  Place a new widget, activate an existing widget, or deactivate widget
  if ( updown == GLUT_DOWN )
  {
    // If there is a selected object, create a new one of that type.
//    extern puObject *createWidget ( int type ) ;
    extern int selected_object_type ;

    if ( selected_object_type )
    {
      extern bool selected_object_sticky ;
      extern char *selected_type_string ;
      extern puButton *active_button ;

      main_window_changed = true ;

//      puObject *new_obj = createWidget ( selected_object->getType () ) ;
      puObject *new_obj = new puFrame ( 0, 0, 90, 20 ) ;
      char *object_type_name = new char [ PUSTRING_MAX ] ;
      strcpy ( object_type_name, selected_type_string ) ;
      new_obj->setLegend ( object_type_name ) ;

      // Add the new object to the list of widgets
      WidgetList *new_wid = new WidgetList ;
      new_wid->obj = new_obj ;
      new_wid->label_text = (char *)NULL ;
      new_wid->legend_text = (char *)NULL ;
      new_wid->object_type_name = object_type_name ;
      new_wid->object_type = selected_object_type ;
      new_wid->callbacks = 1 ;  // Default:  up callback enabled, others disabled
      sprintf ( new_wid->object_name, "widget%d", widget_number++ ) ;
      new_wid->visible = true ;
      new_wid->layer = max_layer - 1 ;
      new_wid->next = widgets ;
      widgets = new_wid ;

      // Set the new widget's position
      new_obj->setPosition ( x, y ) ;

      mouse_x_position_in_object = 0 ;
      mouse_y_position_in_object = 0 ;

      // Make the new widget the active widget
      active_object = new_obj ;

      // If the "sticky" flag is not set, reset the selected object
      if ( !selected_object_sticky )
      {
        selected_object_type = 0 ;
        active_button->setValue ( 0 ) ;
      }
    }
    else
    {
      // Clicking on a widget in the main widget list activates it
      WidgetList *wid = widgets ;
      while ( wid )
      {
        puBox *box = wid->obj->getABox () ;
        if ( wid->visible && ( x >= box->min[0] ) && ( x <= box->max[0] ) &&
                             ( y >= box->min[1] ) && ( y <= box->max[1] ) )
        {
#define RESIZING_BORDER_WIDTH   5
          active_widget = wid ;
          active_object = wid->obj ;
          if ( abs ( x - box->min[0] ) < RESIZING_BORDER_WIDTH )
            activity_flag = 2 ;  // Resizing on x-min
          else if ( abs ( x - box->max[0] ) < RESIZING_BORDER_WIDTH )
            activity_flag = 4 ;  // Resizing on x-max
          else if ( abs ( y - box->min[1] ) < RESIZING_BORDER_WIDTH )
            activity_flag = 3 ;  // Resizing on y-min
          else if ( abs ( y - box->max[1] ) < RESIZING_BORDER_WIDTH )
            activity_flag = 5 ;  // Resizing on y-max
          else
            activity_flag = 1 ;  // Away from edges, we're moving it

          // TO DO:  If the user clicks on a corner, let him resize in both directions at once

          resize_symmetric = ctrl_key_down ? 2 : 1 ;

          int object_x, object_y ;
          active_object->getPosition ( &object_x, &object_y ) ;
          mouse_x_position_in_object = x - object_x ;
          mouse_y_position_in_object = y - object_y ;

          break ;
        }

        wid = wid->next ;
      }

      if ( !wid )  // Ran through the entire list, deactivate any active widget
      {
        active_widget = (WidgetList *)NULL ;
        active_object = (puObject *)NULL ;
      }
    }

    setStatusWidgets ( active_widget ) ;
  }

  glutPostRedisplay () ;
}


static void main_window_reshapefn ( int w, int h )
{
  extern puInput *window_size_x ;
  extern puInput *window_size_y ;

  main_window_changed = true ;

  if ( ( mouse_x < main_window_width/2 ) ||  // Grabbed the left edge ...
       ( mouse_y < main_window_height/2 ) )  // or the bottom edge, move the widgets
  {
    WidgetList *wid = widgets ;
    int deltax = 0 ;
    int deltay = 0 ;

    if ( mouse_x < main_window_width/2  ) deltax = w - main_window_width ;
    if ( mouse_y < main_window_height/2 ) deltay = h - main_window_height ;

    while ( wid )
    {
      int x, y ;
      wid->obj->getPosition ( &x, &y ) ;
      wid->obj->setPosition ( x + deltax, y + deltay ) ;

      wid = wid->next ;
    }
  }

  main_window_width = w ;
  main_window_height = h ;

  window_size_x->setValue ( w ) ;
  window_size_y->setValue ( h ) ;
}

static void main_window_displayfn ( void )
{
  /* Clear the screen */

  glClearColor ( main_window_color_r, main_window_color_g, main_window_color_b, main_window_color_a ) ;
  glClear      ( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) ;

  // Set the OpenGL to draw PUI widgets

  int w = puGetWindowWidth  () ;
  int h = puGetWindowHeight () ;

  glPushAttrib   ( GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_LIGHTING_BIT ) ;

  glDisable      ( GL_LIGHTING   ) ;
  glDisable      ( GL_FOG        ) ;
  glDisable      ( GL_TEXTURE_2D ) ;
  glDisable      ( GL_DEPTH_TEST ) ;
  glDisable      ( GL_CULL_FACE  ) ;
 
  glViewport ( 0, 0, w, h ) ;
 
  glMatrixMode   ( GL_PROJECTION ) ;
  glPushMatrix   () ;
  glLoadIdentity () ;
  gluOrtho2D     ( 0, w, 0, h ) ;
  glMatrixMode   ( GL_MODELVIEW ) ;
  glPushMatrix   () ;
  glLoadIdentity () ;

  // Draw the widgets
  int layer ;
  for ( layer = 0; layer < max_layer; layer++ )
  {
    WidgetList *wid = widgets ;
    while ( wid )
    {
      if ( wid->visible && ( wid->layer == layer ) ) wid->obj->draw ( 0, 0 ) ;
      wid = wid->next ;
    }
  }

  if ( active_object )
  {
    puBox *box = active_object->getBBox () ;
    glColor4f ( 0.0f, 0.0f, 0.0f, 1.0f ) ;
    glLineWidth ( 3.0f ) ;
    glBegin ( GL_LINES ) ;
    glVertex2i ( box->min[0]-1, box->min[1]-1 ) ;
    glVertex2i ( box->max[0]+1, box->min[1]-1 ) ;
    glVertex2i ( box->max[0]+1, box->max[1]+1 ) ;
    glVertex2i ( box->min[0]-1, box->max[1]+1 ) ;
    glVertex2i ( box->min[0]-1, box->min[1]-1 ) ;
    glEnd () ;
  }

  /* Update GLUT */

  glutSwapBuffers   () ;
  glutPostRedisplay () ;
}

int main ( int argc, char **argv )
{
  extern int define_widget_window () ;
  extern int define_status_window () ;

  strcpy ( main_window_name, "PUI GUI Builder" ) ;

  glutInitWindowPosition( 100,   0 ) ;
  glutInitWindowSize    ( main_window_width, main_window_height ) ;
  glutInit              ( &argc, argv ) ;
  glutInitDisplayMode   ( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH ) ;
  main_window = glutCreateWindow      ( main_window_name ) ;
  glutDisplayFunc       ( main_window_displayfn ) ;
  glutKeyboardFunc      ( main_window_keyfn     ) ;
  glutSpecialFunc       ( main_window_specialfn ) ;
  glutMouseFunc         ( main_window_mousefn   ) ;
  glutMotionFunc        ( main_window_motionfn  ) ;
  glutPassiveMotionFunc ( main_window_passivefn ) ;
  glutReshapeFunc       ( main_window_reshapefn ) ;
  glutIdleFunc          ( main_window_displayfn ) ;

  puInit () ;

#ifdef VOODOO
  puShowCursor () ;
#endif

  puSetDefaultStyle        ( PUSTYLE_SMALL_SHADED ) ;
  puSetDefaultColourScheme ( 0.8, 0.8, 0.8, 1.0 ) ;

  // Set up the other windows
  define_widget_window () ;
  define_status_window () ;

  glutMainLoop () ;
  return 0 ;
}

