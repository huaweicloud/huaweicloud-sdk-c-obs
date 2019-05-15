/*
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat
 * not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.  Red
 * Hat makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef	_SYNAPTICS_H_
#define _SYNAPTICS_H_

#include <X11/Xdefs.h>

/******************************************************************************
 *		Public definitions.
 *			Used by driver and the shared memory configurator
 *****************************************************************************/
typedef enum {
    RT_TAP = 0,				    /* Right top corner */
    RB_TAP,				    /* Right bottom corner */
    LT_TAP,				    /* Left top corner */
    LB_TAP,				    /* Left bottom corner */
    F1_TAP,				    /* Non-corner tap, one finger */
    F2_TAP,				    /* Non-corner tap, two fingers */
    F3_TAP,				    /* Non-corner tap, three fingers */
    MAX_TAP
} TapEvent;

typedef enum {
    F1_CLICK1 = 0,			    /* Click left, one finger */
    F2_CLICK1,				    /* Click left, two fingers */
    F3_CLICK1,				    /* Click left, three fingers */
    MAX_CLICK
} ClickFingerEvent;

#define SYN_MAX_BUTTONS 12		    /* Max number of mouse buttons */

struct SynapticsHwInfo {
    unsigned int model_id;		    /* Model-ID */
    unsigned int capabilities;		    /* Capabilities */
    unsigned int ext_cap;		    /* Extended Capabilities */
    unsigned int identity;		    /* Identification */
    Bool hasGuest;			    /* Has a guest mouse */
};


#define SHM_SYNAPTICS 23947
typedef struct _SynapticsSHM
{
    int version;			    /* Driver version */

    /* Current device state */
    int x, y;				    /* actual x, y coordinates */
    int z;				    /* pressure value */
    int numFingers;			    /* number of fingers */
    int fingerWidth;			    /* finger width value */
    int left, right, up, down;		    /* left/right/up/down buttons */
    Bool multi[8];
    Bool middle;
    int guest_left, guest_mid, guest_right; /* guest device buttons */
    int guest_dx, guest_dy; 		    /* guest device movement */

    /* Probed hardware properties */
    struct SynapticsHwInfo synhw;

    /* Parameter data */
    int left_edge, right_edge, top_edge, bottom_edge; /* edge coordinates absolute */
    int finger_low, finger_high, finger_press;	      /* finger detection values in Z-values */
    int tap_time;
    int tap_move;			    /* max. tapping time and movement in packets and coord. */
    int single_tap_timeout;		    /* timeout to recognize a single tap */
    int tap_time_2;			    /* max. tapping time for double taps */
    int click_time;			    /* The duration of a single click */
    Bool fast_taps;			    /* Faster reaction to single taps */
    int emulate_mid_button_time;	    /* Max time between left and right button presses to
					       emulate a middle button press. */
    int emulate_twofinger_z;		    /* pressure threshold to emulate two finger touch (for Alps) */
    int emulate_twofinger_w;		    /* Finger width threshold to emulate two finger touch */
    int scroll_dist_vert;		    /* Scrolling distance in absolute coordinates */
    int scroll_dist_horiz;		    /* Scrolling distance in absolute coordinates */
    Bool scroll_edge_vert;		    /* Enable/disable vertical scrolling on right edge */
    Bool scroll_edge_horiz;		    /* Enable/disable horizontal scrolling on left edge */
    Bool scroll_edge_corner;		    /* Enable/disable continuous edge scrolling when in the corner */
    Bool scroll_twofinger_vert;		    /* Enable/disable vertical two-finger scrolling */
    Bool scroll_twofinger_horiz;	    /* Enable/disable horizontal two-finger scrolling */
    Bool special_scroll_area_right;         /* Enable/disable autodetection right special scroll area */
    double min_speed, max_speed, accl;	    /* movement parameters */
    double trackstick_speed;		    /* trackstick mode speed */
    int edge_motion_min_z;		    /* finger pressure at which minimum edge motion speed is set */
    int edge_motion_max_z;		    /* finger pressure at which maximum edge motion speed is set */
    int edge_motion_min_speed;		    /* slowest setting for edge motion speed */
    int edge_motion_max_speed;		    /* fastest setting for edge motion speed */
    Bool edge_motion_use_always;	    /* If false, egde motion is used only when dragging */

    Bool updown_button_scrolling;	    /* Up/Down-Button scrolling or middle/double-click */
    Bool leftright_button_scrolling;	    /* Left/right-button scrolling, or two lots of middle button */
    Bool updown_button_repeat;		    /* If up/down button being used to scroll, auto-repeat?*/
    Bool leftright_button_repeat;	    /* If left/right button being used to scroll, auto-repeat? */
    int scroll_button_repeat;		    /* time, in milliseconds, between scroll events being
					     * sent when holding down scroll buttons */
    int touchpad_off;			    /* Switches the touchpad off
					     * 0 : Not off
					     * 1 : Off
					     * 2 : Only tapping and scrolling off
					     */
    Bool guestmouse_off;		    /* Switches the guest mouse off */
    Bool locked_drags;			    /* Enable locked drags */
    int locked_drag_time;		    /* timeout for locked drags */
    int tap_action[MAX_TAP];		    /* Button to report on tap events */
    int click_action[MAX_CLICK];	    /* Button to report on click with fingers */
    Bool circular_scrolling;		    /* Enable circular scrolling */
    double scroll_dist_circ;		    /* Scrolling angle radians */
    int circular_trigger;		    /* Trigger area for circular scrolling */
    Bool circular_pad;			    /* Edge has an oval or circular shape */
    Bool palm_detect;			    /* Enable Palm Detection */
    int palm_min_width;			    /* Palm detection width */
    int palm_min_z;			    /* Palm detection depth */
    double coasting_speed;		    /* Coasting threshold scrolling speed */
    int press_motion_min_z;		    /* finger pressure at which minimum pressure motion factor is applied */
    int press_motion_max_z;		    /* finger pressure at which maximum pressure motion factor is applied */
    double press_motion_min_factor;	    /* factor applied on speed when finger pressure is at minimum */
    double press_motion_max_factor; 	    /* factor applied on speed when finger pressure is at minimum */
    Bool grab_event_device;		    /* grab event device for exclusive use? */
    int led_double_tap;			    /* double-tap period in ms for touchpad LED control */
    int touch_button_area;                  /* touch button (clickpad) area in percent */
    Bool touch_button_sense;                /* touch button (clickpad) area sensing */
} SynapticsSHM;

/*
 * Minimum and maximum values for scroll_button_repeat
 */
#define SBR_MIN 10
#define SBR_MAX 1000

/*
 * The x/y limits are taken from the Synaptics TouchPad interfacing Guide,
 * section 2.3.2, which says that they should be valid regardless of the
 * actual size of the sensor.
 */
#define XMIN_NOMINAL 1472
#define XMAX_NOMINAL 5472
#define YMIN_NOMINAL 1408
#define YMAX_NOMINAL 4448

#define XMAX_VALID 6143

#endif /* _SYNAPTICS_H_ */
